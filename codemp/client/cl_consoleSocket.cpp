/*
===========================================================================
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// cl_consoleSocket.cpp -- loopback TCP server that streams console output to
// external apps and executes lines they send back as console commands.
// Enabled by setting cl_consoleSocket to a port number (0 = disabled).

#include "client.h"

#ifdef _WIN32
	#include <winsock.h>

	typedef int socklen_t;

	#define socketError WSAGetLastError( )
	#define NET_WOULDBLOCK(e) ((e) == WSAEWOULDBLOCK)
#else
	#include <arpa/inet.h>
	#include <errno.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <sys/ioctl.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <unistd.h>

	typedef int SOCKET;
	#define INVALID_SOCKET	-1
	#define SOCKET_ERROR	-1
	#define closesocket		close
	#define ioctlsocket		ioctl
	#define socketError		errno
	#define NET_WOULDBLOCK(e) ((e) == EWOULDBLOCK || (e) == EAGAIN)
#endif

#include <mutex>
#include <string>

#define MAX_SOCKET_CLIENTS	4
#define MAX_PENDING_TEXT	(128*1024)	// output buffered between frames; dropped beyond this
#define MAX_CLIENT_OUTBUF	(256*1024)	// per-client send backlog; client dropped beyond this
#define MAX_COMMAND_LINE	1024

typedef struct socketClient_s {
	qboolean	active;
	qboolean	authed;		// has sent the correct password (or none is required)
	SOCKET		sock;
	std::string	outbuf;
	char		line[MAX_COMMAND_LINE];
	size_t		lineLen;
	qboolean	lineOverflow;
} socketClient_t;

static cvar_t			*cl_consoleSocket = NULL;
static cvar_t			*cl_consoleSocketPassword = NULL;
static SOCKET			listenSocket = INVALID_SOCKET;
static socketClient_t	socketClients[MAX_SOCKET_CLIENTS];

// CL_ConsoleSocket_Print can be hit from any thread that calls Com_Printf,
// so output is staged here and flushed to the sockets once per client frame.
static std::mutex		pendingLock;
static std::string		pendingText;
static bool				haveClients = false;

static qboolean ConsoleSocket_SetNonBlocking( SOCKET sock ) {
	u_long _true = 1;

	if ( ioctlsocket( sock, FIONBIO, &_true ) == SOCKET_ERROR ) {
		return qfalse;
	}
	return qtrue;
}

static void ConsoleSocket_DropClient( socketClient_t *cl, const char *reason ) {
	closesocket( cl->sock );
	cl->sock = INVALID_SOCKET;
	cl->active = qfalse;
	cl->authed = qfalse;
	cl->outbuf.clear();
	cl->lineLen = 0;
	cl->lineOverflow = qfalse;
	Com_Printf( "Console socket: client dropped (%s)\n", reason );
}

static void ConsoleSocket_CloseAll( void ) {
	int i;

	if ( listenSocket != INVALID_SOCKET ) {
		closesocket( listenSocket );
		listenSocket = INVALID_SOCKET;
	}

	for ( i = 0; i < MAX_SOCKET_CLIENTS; i++ ) {
		if ( socketClients[i].active ) {
			closesocket( socketClients[i].sock );
			socketClients[i].sock = INVALID_SOCKET;
			socketClients[i].active = qfalse;
			socketClients[i].outbuf.clear();
			socketClients[i].lineLen = 0;
			socketClients[i].lineOverflow = qfalse;
		}
	}

	std::lock_guard<std::mutex> l( pendingLock );
	pendingText.clear();
	haveClients = false;
}

static void ConsoleSocket_Open( int port ) {
	struct sockaddr_in	address;
	SOCKET				sock;

	sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( sock == INVALID_SOCKET ) {
		Com_Printf( "WARNING: Console socket: couldn't create socket (error %d)\n", socketError );
		return;
	}

	if ( !ConsoleSocket_SetNonBlocking( sock ) ) {
		Com_Printf( "WARNING: Console socket: ioctl FIONBIO failed (error %d)\n", socketError );
		closesocket( sock );
		return;
	}

#ifndef _WIN32
	{
		int reuse = 1;
		setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse) );
	}
#endif

	// only ever listen on loopback - this executes console commands, it must
	// not be reachable from the network
	memset( &address, 0, sizeof(address) );
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
	address.sin_port = htons( (unsigned short)port );

	if ( bind( sock, (struct sockaddr *)&address, sizeof(address) ) == SOCKET_ERROR ) {
		Com_Printf( "WARNING: Console socket: couldn't bind 127.0.0.1:%d (error %d)\n", port, socketError );
		closesocket( sock );
		return;
	}

	if ( listen( sock, 2 ) == SOCKET_ERROR ) {
		Com_Printf( "WARNING: Console socket: listen failed (error %d)\n", socketError );
		closesocket( sock );
		return;
	}

	listenSocket = sock;
	Com_Printf( "Console socket: listening on 127.0.0.1:%d\n", port );
}

static void ConsoleSocket_Accept( void ) {
	while ( 1 ) {
		struct sockaddr_in	from;
		socklen_t			fromlen = sizeof(from);
		SOCKET				sock;
		int					i;

		sock = accept( listenSocket, (struct sockaddr *)&from, &fromlen );
		if ( sock == INVALID_SOCKET ) {
			return;
		}

		for ( i = 0; i < MAX_SOCKET_CLIENTS; i++ ) {
			if ( !socketClients[i].active ) {
				break;
			}
		}

		if ( i == MAX_SOCKET_CLIENTS || !ConsoleSocket_SetNonBlocking( sock ) ) {
			closesocket( sock );
			continue;
		}

		{
			int nodelay = 1;
			setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&nodelay, sizeof(nodelay) );
		}

		socketClients[i].sock = sock;
		socketClients[i].active = qtrue;
		// with no password set, clients are trusted as soon as they connect
		socketClients[i].authed = (qboolean)( cl_consoleSocketPassword->string[0] == '\0' );
		socketClients[i].outbuf.clear();
		socketClients[i].lineLen = 0;
		socketClients[i].lineOverflow = qfalse;
		Com_Printf( "Console socket: client connected%s\n",
			socketClients[i].authed ? "" : " (waiting for password)" );
	}
}

static void ConsoleSocket_ReceiveFrom( socketClient_t *cl ) {
	char	buf[1024];
	int		r, i;

	while ( cl->active ) {
		r = recv( cl->sock, buf, sizeof(buf), 0 );
		if ( r == 0 ) {
			ConsoleSocket_DropClient( cl, "disconnected" );
			return;
		}
		if ( r < 0 ) {
			if ( !NET_WOULDBLOCK( socketError ) ) {
				ConsoleSocket_DropClient( cl, "receive error" );
			}
			return;
		}

		for ( i = 0; i < r; i++ ) {
			char c = buf[i];

			if ( c == '\r' ) {
				continue;
			}
			if ( c == '\n' ) {
				if ( !cl->authed ) {
					// first line must be the password, anything else
					// (including HTTP requests from a hostile browser tab)
					// gets the connection closed
					cl->line[cl->lineLen] = '\0';
					if ( !cl->lineOverflow && cl->lineLen > 0
						&& cl_consoleSocketPassword->string[0]
						&& !strcmp( cl->line, cl_consoleSocketPassword->string ) ) {
						cl->authed = qtrue;
						cl->outbuf.append( "cl_consoleSocket: authenticated\n" );
					}
					else {
						ConsoleSocket_DropClient( cl, "bad password" );
						return;
					}
				}
				else if ( !cl->lineOverflow && cl->lineLen > 0 ) {
					cl->line[cl->lineLen] = '\0';
					Cbuf_AddText( cl->line );
					Cbuf_AddText( "\n" );
				}
				cl->lineLen = 0;
				cl->lineOverflow = qfalse;
			}
			else if ( cl->lineLen < sizeof(cl->line) - 1 ) {
				cl->line[cl->lineLen++] = c;
			}
			else {
				cl->lineOverflow = qtrue;	// discard the whole oversized line
			}
		}
	}
}

static void ConsoleSocket_SendTo( socketClient_t *cl ) {
	int sent;

	while ( cl->active && !cl->outbuf.empty() ) {
		sent = send( cl->sock, cl->outbuf.data(), (int)cl->outbuf.size(), 0 );
		if ( sent > 0 ) {
			cl->outbuf.erase( 0, (size_t)sent );
			continue;
		}
		if ( sent < 0 && !NET_WOULDBLOCK( socketError ) ) {
			ConsoleSocket_DropClient( cl, "send error" );
		}
		return;
	}
}

/*
==================
CL_ConsoleSocketInfo_f

Player-facing explanation of the whole feature, shown by typing /consolesocket
==================
*/
static void CL_ConsoleSocketInfo_f( void ) {
	int i, connected = 0;

	for ( i = 0; i < MAX_SOCKET_CLIENTS; i++ ) {
		if ( socketClients[i].active ) {
			connected++;
		}
	}

	Com_Printf( "\n^5--- Console Socket: external app access ---\n" );
	Com_Printf( "^7Lets a program running on ^2this computer^7 read everything that\n" );
	Com_Printf( "^7appears in this console (chat included!) and send console\n" );
	Com_Printf( "^7commands back, over a local TCP connection.\n\n" );

	Com_Printf( "^5Status:\n" );
	if ( listenSocket != INVALID_SOCKET ) {
		Com_Printf( "  ^2enabled^7 - listening on 127.0.0.1:%d\n", cl_consoleSocket->integer );
		Com_Printf( "  connected apps: ^3%d^7 (max %d)\n", connected, MAX_SOCKET_CLIENTS );
	}
	else {
		Com_Printf( "  ^1disabled^7\n" );
	}
	Com_Printf( "  password: %s\n\n", cl_consoleSocketPassword->string[0] ? "^2set" : "^1not set" );

	Com_Printf( "^5Settings (both saved in your config):\n" );
	Com_Printf( "  ^3cl_consoleSocket <port>^7 - port to listen on, ^30^7 = off (default)\n" );
	Com_Printf( "  ^3cl_consoleSocketPassword <password>^7 - apps must send this as\n" );
	Com_Printf( "    their first line before anything works (recommended!)\n\n" );

	Com_Printf( "^5Quick start:\n" );
	Com_Printf( "  ^21.^7 ^3cl_consoleSocketPassword mySecret123\n" );
	Com_Printf( "  ^22.^7 ^3cl_consoleSocket 29071\n" );
	Com_Printf( "  ^23.^7 connect your app to ^3127.0.0.1:29071^7 and have it send the\n" );
	Com_Printf( "     password followed by a newline\n" );
	Com_Printf( "  ^24.^7 console lines stream to the app as they happen; any line the\n" );
	Com_Printf( "     app sends back runs as a console command (e.g. ^3say hi^7)\n\n" );

	Com_Printf( "^5Security:\n" );
	Com_Printf( "  ^7- Only programs on ^2your own computer^7 can connect, ever.\n" );
	Com_Printf( "    The socket is bound to 127.0.0.1 (loopback only).\n" );
	Com_Printf( "  ^1- A connected app can run ANY console command as you, so set a\n" );
	Com_Printf( "    password and only use apps you trust.\n\n" );

	Com_Printf( "^7Turn it off any time with ^3cl_consoleSocket 0^7.\n\n" );
}

void CL_ConsoleSocket_Init( void ) {
	cl_consoleSocket = Cvar_Get( "cl_consoleSocket", "0", CVAR_ARCHIVE_ND,
		"TCP port on 127.0.0.1 that streams console output to external apps and accepts console commands (0 = disabled)" );
	cl_consoleSocketPassword = Cvar_Get( "cl_consoleSocketPassword", "", CVAR_ARCHIVE_ND,
		"If set, apps connecting to cl_consoleSocket must send this as their first line" );

	Cmd_AddCommand( "consolesocket", CL_ConsoleSocketInfo_f, "Explain the console socket feature (external app access)" );
}

void CL_ConsoleSocket_Shutdown( void ) {
	Cmd_RemoveCommand( "consolesocket" );
	ConsoleSocket_CloseAll();
}

// Called for every console print (any thread). Just stage the text; the
// frame pump does all socket work on the main thread.
void CL_ConsoleSocket_Print( const char *text ) {
	if ( !text || !*text ) {
		return;
	}

	std::lock_guard<std::mutex> l( pendingLock );
	if ( !haveClients ) {
		return;
	}
	if ( pendingText.size() + strlen( text ) > MAX_PENDING_TEXT ) {
		return;	// reader has stalled, drop rather than grow unbounded
	}
	pendingText.append( text );
}

void CL_ConsoleSocket_Frame( void ) {
	static int	modCount = -1;
	std::string	out;
	bool		anyActive = false;
	int			i;

	if ( !cl_consoleSocket ) {
		return;
	}

	if ( cl_consoleSocket->modificationCount != modCount ) {
		modCount = cl_consoleSocket->modificationCount;
		ConsoleSocket_CloseAll();
		if ( cl_consoleSocket->integer > 0 && cl_consoleSocket->integer <= 65535 ) {
			ConsoleSocket_Open( cl_consoleSocket->integer );
		}
		else if ( cl_consoleSocket->integer != 0 ) {
			Com_Printf( "WARNING: Console socket: invalid port %d\n", cl_consoleSocket->integer );
		}
	}

	if ( listenSocket == INVALID_SOCKET ) {
		return;
	}

	ConsoleSocket_Accept();

	for ( i = 0; i < MAX_SOCKET_CLIENTS; i++ ) {
		if ( socketClients[i].active ) {
			ConsoleSocket_ReceiveFrom( &socketClients[i] );
		}
		if ( socketClients[i].active && socketClients[i].authed ) {
			anyActive = true;
		}
	}

	{
		std::lock_guard<std::mutex> l( pendingLock );
		out.swap( pendingText );
		haveClients = anyActive;
	}

	for ( i = 0; i < MAX_SOCKET_CLIENTS; i++ ) {
		if ( !socketClients[i].active ) {
			continue;
		}
		if ( !out.empty() && socketClients[i].authed ) {
			if ( socketClients[i].outbuf.size() + out.size() > MAX_CLIENT_OUTBUF ) {
				ConsoleSocket_DropClient( &socketClients[i], "send backlog overflow" );
				continue;
			}
			socketClients[i].outbuf.append( out );
		}
		ConsoleSocket_SendTo( &socketClients[i] );
	}
}
