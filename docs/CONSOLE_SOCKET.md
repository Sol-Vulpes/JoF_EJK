# Console Socket — external app access to the in-game console

Lets a program running on the **same computer** as the game read everything
that appears in the console (chat included) and send console commands back,
over a plain TCP connection.

Typical use: an external app that watches chat and saves funny quotes, a
stream overlay, a chat relay, etc.

Type **`/consolesocket`** in the in-game console at any time for a summary of
everything on this page, including live status (port, connected apps,
password set or not).

## Enabling it

The feature is **disabled by default**. Both cvars are archived, so they are
saved in your config and survive restarts.

| Cvar | Default | Meaning |
|---|---|---|
| `cl_consoleSocket` | `0` | TCP port to listen on. `0` = off. |
| `cl_consoleSocketPassword` | *(empty)* | If set, an app must send this as its very first line before anything works. **Strongly recommended.** |

Quick start, in the in-game console:

```
cl_consoleSocketPassword mySecret123
cl_consoleSocket 29071
```

The console prints `Console socket: listening on 127.0.0.1:29071`. Set
`cl_consoleSocket 0` to turn it off again.

## Protocol

It is a raw text stream — no framing, no JSON, just lines. Any language that
can open a TCP socket works.

1. Connect to `127.0.0.1:<port>`.
2. If a password is set, send it followed by `\n` as your **first** line.
   - Correct: the game replies `cl_consoleSocket: authenticated\n` and the
     stream starts. Anything wrong (including not sending it): immediate
     disconnect. Nothing is streamed and nothing executes until then.
3. **Receiving:** every console line is sent to you as it prints, terminated
   by `\n`. Text still contains Quake color codes (`^1`–`^9`); strip them if
   you don't want them.
4. **Sending:** every line you send (terminated by `\n`; `\r` is ignored) is
   executed as a console command, exactly as if typed locally. For example
   `say hello there` chats in-game.

### Spotting chat lines

Chat messages contain the byte `0x19` right before the colon that separates
the player name from the message:

```
PlayerName<0x19>: the actual message
```

No other console output contains that byte, so `line.contains('\x19')` is a
reliable chat filter.

### Limits

- Up to **4** apps connected at once; further connections are refused.
- Command lines you send are capped at **1023 chars**; longer lines are
  discarded whole (never truncated into a different command).
- If your app stops reading, output is dropped (the game never blocks or
  freezes on a slow reader); an app with more than 256 KB of unread backlog
  is disconnected.

## Example client (PowerShell)

```powershell
$c = [Net.Sockets.TcpClient]::new("127.0.0.1", 29071)
$s = $c.GetStream()
$w = [IO.StreamWriter]::new($s); $w.AutoFlush = $true
$r = [IO.StreamReader]::new($s)

$w.WriteLine("mySecret123")           # password first (if set)
$w.WriteLine("say hello from outside")

while ($null -ne ($line = $r.ReadLine())) {
    if ($line.Contains([char]0x19)) { Write-Host "CHAT: $line" }
}
```

## Example client (Python)

```python
import socket

s = socket.create_connection(("127.0.0.1", 29071))
s.sendall(b"mySecret123\n")           # password first (if set)

buf = b""
while True:
    data = s.recv(4096)
    if not data:
        break
    buf += data
    while b"\n" in buf:
        line, buf = buf.split(b"\n", 1)
        if b"\x19" in line:           # it's a chat message
            print("CHAT:", line.decode("utf-8", "replace"))
```

## Security model

- The listener is bound to `127.0.0.1` (loopback) only. The OS network stack
  will not accept connections from other machines, period — this is not
  firewall-dependent.
- A connected app can execute **any** console command with your privileges
  (`bind`, `connect`, `quit`, ...). Treat it like handing your console to
  that app — only run apps you trust.
- **Set a password.** Even though only local programs can connect, a web page
  open in your browser can make HTTP requests to `localhost` ports. Without
  a password, the lines of such a request would be executed as console
  commands. With a password set, the first line of an HTTP request fails
  authentication and the connection is dropped before anything executes.
