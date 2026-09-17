#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #c); exit(1); } } while (0)
typedef int qboolean;
enum { qfalse, qtrue, CA_CONNECTED = 3, CA_ACTIVE = 8, CVAR_USERINFO = 2 };
#define S_COLOR_WHITE ""
#ifdef _MSC_VER
#define Q_stricmp _stricmp
#else
#include <strings.h>
#define Q_stricmp strcasecmp
#endif
static struct { int state; } cls;
static struct { int demoplaying; } clc;
static int cvar_modifiedFlags, paused, commandCount, argc;
static const char *argv[3], *args;
static char commands[4][256];
static char *Cmd_Argv(int i) { return (char *)argv[i]; }
static int Cmd_Argc(void) { return argc; }
static const char *Cmd_Args(void) { return args; }
static void Com_Printf(const char *format, ...) {}
static qboolean CL_CheckPaused(void) { return paused; }
static const char *Cvar_InfoString(int flags) { return "\\forcepowers\\7-1-332331000330003330"; }
static void CL_AddReliableCommand(const char *command, qboolean disconnect) {
    CHECK(commandCount < 4);
    strcpy(commands[commandCount++], command);
}
static char *va(const char *format, ...) {
    static char buffer[256]; va_list list;
    va_start(list, format); vsnprintf(buffer, sizeof(buffer), format, list); va_end(list); return buffer;
}
#include "protocol.h"
int main(void) {
    int direct;
    for (direct = 0; direct <= 1; ++direct) {
        cls.state = CA_ACTIVE;
        cvar_modifiedFlags = CVAR_USERINFO | 16;
        commandCount = 0;
        argv[0] = direct ? "forcechanged" : "cmd";
        argv[1] = direct ? "blue" : "forcechanged";
        argv[2] = "blue";
        argc = direct ? 2 : 3;
        args = "forcechanged blue";
        if (direct) CL_ForwardCommandToServer("forcechanged blue");
        else CL_ForwardToServer_f();
        /* A spectator's forcechanged handler immediately reads userinfo. */
        CHECK(commandCount == 2);
        CHECK(!strncmp(commands[0], "userinfo ", 9));
        CHECK(!strcmp(commands[1], "forcechanged blue"));
        CHECK(cvar_modifiedFlags == 16);
        CL_CheckUserinfo();
        CHECK(commandCount == 2);
        /* No duplicate userinfo for an unchanged allocation. */
        commandCount = 0;
        if (direct) CL_ForwardCommandToServer("forcechanged blue");
        else CL_ForwardToServer_f();
        CHECK(commandCount == 1);
        /* Ordinary commands keep their existing behavior. */
        argv[0] = direct ? "say" : "cmd";
        argv[1] = "say";
        args = "say hello";
        commandCount = 0;
        cvar_modifiedFlags = CVAR_USERINFO;
        if (direct) CL_ForwardCommandToServer("say hello");
        else CL_ForwardToServer_f();
        CHECK(commandCount == 1 && cvar_modifiedFlags == CVAR_USERINFO);
        CL_CheckUserinfo();
        CHECK(commandCount == 2);
    }
    puts("PASS userinfo precedes forcechanged for both forwarding paths");
    return 0;
}
