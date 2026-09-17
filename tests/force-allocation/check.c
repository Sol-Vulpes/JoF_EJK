#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #c); exit(1); } } while (0)
typedef int qboolean;
typedef int menuDef_t;
typedef int fileHandle_t;
typedef float vec3_t[3];
typedef int uiMenuCommand_t;
enum { UIMENU_NONE, UIMENU_MAIN, UIMENU_TEAM, UIMENU_POSTGAME, UIMENU_INGAME,
    UIMENU_PLAYERCONFIG, UIMENU_PLAYERFORCE, UIMENU_SIEGEMESSAGE, UIMENU_SIEGEOBJECTIVES,
    UIMENU_VOICECHAT, UIMENU_CLOSEALL, UIMENU_CLASSSEL };
enum { qfalse, qtrue };
enum { FP_HEAL, FP_LEVITATION, FP_SPEED, FP_PUSH, FP_PULL, FP_TELEPATHY,
    FP_GRIP, FP_LIGHTNING, FP_RAGE, FP_PROTECT, FP_ABSORB, FP_TEAM_HEAL,
    FP_TEAM_FORCE, FP_DRAIN, FP_SEE, FP_SABER_OFFENSE, FP_SABER_DEFENSE,
    FP_SABERTHROW, NUM_FORCE_POWERS };
enum { FORCE_LEVEL_1 = 1, FORCE_LEVEL_3 = 3, NUM_FORCE_POWER_LEVELS = 4,
    NUM_FORCE_MASTERY_LEVELS = 8, MAX_FORCE_RANK = 7,
    FORCE_LIGHTSIDE = 1, FORCE_DARKSIDE = 2, GT_FFA = 0, GT_TEAM = 6,
    FORCE_NONJEDI = 0, FORCE_JEDI = 1, TEAM_RED = 1, TEAM_BLUE = 2,
    TEAM_SPECTATOR = 3, MAX_INFO_VALUE = 1024, CS_SERVERINFO = 0,
    EXEC_APPEND = 0, FS_READ = 0, KEYCATCH_UI = 2 };
#define DEFAULT_FORCEPOWERS "7-1-032330000000001333"
static struct { int integer; } ui_freeSaber, ui_rankChange, ui_drawTeamForces, ui_drawCursor;
static struct { int integer; } ui_singlePlayerActive, ui_isJAPro, ui_vgs;
static int uiForcePowersRank[NUM_FORCE_POWERS], uiForceUsed, uiForceAvailable;
static int uiForceRank = 7, uiMaxRank = 7, uiForceSide = 1, uiJediNonJedi;
static int gCustPowersRank[NUM_FORCE_POWERS], gCustRank, gCustSide;
static qboolean gTouchedForce;
static int engineFreeSaber, engineRankChange, paintMenu, savedWrites, commands;
static int gameType, basedTeams, myTeam = TEAM_SPECTATOR;
static char saved[128], templateData[128], parsedFPMessage[1024];
static int FPMessageTime;
static struct {
    struct { int frameTime, realTime, FPS; float cursorx, cursory, widthRatioCoef;
        struct { int cursor; } Assets; } uiDC;
    int newUIAPI, inGameLoad, forceConfigLightIndexBegin, forceConfigDarkIndexBegin, forceConfigCount;
    const char *forceConfigNames[2];
} uiInfo;
static void UI_ReadLegalForce(void);
static void UpdateForceUsed(void);
static void SetCvar(const char *key, const char *value) {
    if (!strcmp(key, "forcepowers")) { strcpy(saved, value); ++savedWrites; }
    else if (!strcmp(key, "ui_rankChange")) engineRankChange = atoi(value);
    else CHECK(0);
}
static float CvarValue(const char *key) { return myTeam; }
static void ExecuteText(int when, const char *command) { ++commands; }
static void ConfigString(int index, char *out, int size) { out[0] = 0; }
static int FileOpen(const char *path, fileHandle_t *handle, int mode) {
    *handle = 1; return (int)strlen(templateData);
}
static void FileRead(void *out, int size, fileHandle_t handle) { memcpy(out, templateData, size); }
static void FileClose(fileHandle_t handle) {}
static void SetTime(int time, int clock) {}
static int KeyCatcher(void) { return 0; }
static void SetKeyCatcher(int catcher) {}
static void ClearKeys(void) {}
static void CvarString(const char *name, char *out, int size) { out[0] = 0; }
static struct {
    void (*Cvar_Set)(const char *, const char *);
    float (*Cvar_VariableValue)(const char *);
    void (*Cmd_ExecuteText)(int, const char *);
    void (*GetConfigString)(int, char *, int);
    int (*FS_Open)(const char *, fileHandle_t *, int);
    void (*FS_Read)(void *, int, fileHandle_t);
    void (*FS_Close)(fileHandle_t);
    void (*G2API_SetTime)(int, int);
    int (*Key_GetCatcher)(void);
    void (*Key_SetCatcher)(int);
    void (*Key_ClearStates)(void);
    void (*Cvar_VariableStringBuffer)(const char *, char *, int);
} imports = { SetCvar, CvarValue, ExecuteText, ConfigString, FileOpen, FileRead,
    FileClose, SetTime, KeyCatcher, SetKeyCatcher, ClearKeys, CvarString }, *trap = &imports;
static const char *Info_ValueForKey(const char *info, const char *key) {
    static char value[32];
    sprintf(value, "%d", !strcmp(key, "g_gametype") ? gameType : basedTeams);
    return value;
}
static char *UI_Cvar_VariableString(const char *key) { return saved; }
static qboolean UI_TrueJediEnabled(void) { return qfalse; }
static const char *UI_TeamName(int team) { return "spectator"; }
static menuDef_t *Menus_FindByName(const char *name) { return NULL; }
static void Menu_ShowItemByName(menuDef_t *menu, const char *name, qboolean show) {}
static int Com_Clampi(int min, int max, int value) { return value < min ? min : value > max ? max : value; }
static void Q_strncpyz(char *out, const char *in, size_t size) { CHECK(strlen(in) < size); strcpy(out, in); }
static void Q_strcat(char *out, size_t size, const char *in) { CHECK(strlen(out) + strlen(in) < size); strcat(out, in); }
#define Com_sprintf snprintf
static char *va(const char *format, ...) {
    static char buffer[256]; va_list args;
    va_start(args, format); vsnprintf(buffer, sizeof(buffer), format, args); va_end(args); return buffer;
}
static void UI_UpdateCvars(void) {
    ui_freeSaber.integer = engineFreeSaber;
    ui_rankChange.integer = engineRankChange;
}
static void UI_BuildQ3Model_List_Process(void) {}
static void UI_LoadNonIngame(void) {}
static void UI_UpdateCurrentServerInfo(void) {}
static void UI_BuildPlayerList(void) {}
static void Menus_CloseAll(void) {}
static void Menus_ActivateByName(const char *name) {}
static int Menu_Count(void) { return paintMenu; }
/* The show-all-force ownerdraw performs this validation during painting. */
static void Menu_PaintAll(void) { UI_ReadLegalForce(); }
static void UI_DoServerRefresh(void) {}
static void UI_BuildServerStatus(qboolean force) {}
static void UI_BuildFindPlayerList(qboolean force) {}
static void UI_SetColor(const float *color) {}
static void UI_DrawHandlePic(float x, float y, float w, float h, int shader) {}
static const char *UI_GetStringEdString(const char *section, const char *key) { return "New rank"; }
qboolean BG_LegalizedForcePowers2(char *, size_t, int, qboolean, int, int, int, qboolean);
#define uiForcePowerDarkLight forcePowerDarkLight
#include "actual.h"

static void FullLoadout(void) {
    memset(uiForcePowersRank, 0, sizeof(uiForcePowersRank));
    uiForcePowersRank[FP_HEAL] = uiForcePowersRank[FP_LEVITATION] = 3;
    uiForcePowersRank[FP_TELEPATHY] = uiForcePowersRank[FP_PROTECT] = 3;
    uiForcePowersRank[FP_ABSORB] = uiForcePowersRank[FP_TEAM_HEAL] = 3;
    uiForcePowersRank[FP_SABER_OFFENSE] = uiForcePowersRank[FP_SABER_DEFENSE] = 3;
    uiForcePowersRank[FP_PUSH] = 1;
    uiForceSide = FORCE_LIGHTSIDE;
    uiForceRank = uiMaxRank = 7;
    ui_freeSaber.integer = engineFreeSaber = 1;
    ui_drawTeamForces.integer = 1;
    UpdateForceUsed();
    CHECK(uiForceAvailable == 0);
}
static void Costs(void) {
    unsigned seed = 184;
    int freeSaber, n, p, rank, used, exact = 0;
    char loadout[128], original[128];
    for (freeSaber = 0; freeSaber <= 1; ++freeSaber) {
        ui_freeSaber.integer = freeSaber;
        CHECK(UI_ForcePowerCost(FP_SABER_OFFENSE, 1) == !freeSaber);
        CHECK(UI_ForcePowerCost(FP_SABER_DEFENSE, 1) == !freeSaber);
        CHECK(UI_ForcePowerCost(FP_SABER_OFFENSE, 2) == 5);
        CHECK(UI_ForcePowerCost(FP_SABERTHROW, 1) == 4);
        for (n = 0; n < 10000; ++n) {
            strcpy(loadout, n & 1 ? "7-1-" : "7-2-");
            for (p = 0; p < NUM_FORCE_POWERS; ++p) {
                seed = seed * 1664525u + 1013904223u;
                loadout[4 + p] = '0' + ((seed >> 24) & 3);
            }
            loadout[4 + NUM_FORCE_POWERS] = 0;
            BG_LegalizedForcePowers2(loadout, sizeof(loadout), 7, freeSaber, 0, GT_FFA, 0, qtrue);
            strcpy(original, loadout);
            used = 0;
            for (p = 0; p < NUM_FORCE_POWERS; ++p) {
                uiForcePowersRank[p] = loadout[4 + p] - '0';
                for (rank = 1; rank <= uiForcePowersRank[p]; ++rank)
                    used += UI_ForcePowerCost(p, rank);
            }
            CHECK(used <= 100);
            if (used == 100) ++exact;
            UpdateForceUsed();
            CHECK(uiForceUsed == used && uiForceAvailable == 100 - used);
            for (p = 0; p < NUM_FORCE_POWERS; ++p)
                CHECK(uiForcePowersRank[p] == original[4 + p] - '0');
            CHECK(BG_LegalizedForcePowers2(loadout, sizeof(loadout), 7, freeSaber, 0, GT_FFA, 0, qtrue));
            CHECK(!strcmp(original, loadout));
        }
    }
    CHECK(exact > 0);
    CHECK(bgForcePowerCost[FP_SABER_OFFENSE][1] == 1);
    CHECK(bgForcePowerCost[FP_SABER_DEFENSE][1] == 1);
    printf("20000 allocations, %d fully spent\n", exact);
}
static void Toggle(void) {
    int ranks[NUM_FORCE_POWERS];
    FullLoadout();
    uiForcePowersRank[FP_PUSH] = 0;
    uiForcePowersRank[FP_HEAL] = 2;
    UpdateForceUsed();
    CHECK(uiForceAvailable == 7);
    memcpy(ranks, uiForcePowersRank, sizeof(ranks));
    engineFreeSaber = 0;
    UI_Refresh(10);
    CHECK(uiForceAvailable == 5);
    CHECK(!memcmp(ranks, uiForcePowersRank, sizeof(ranks)));
    engineFreeSaber = 1;
    UI_Refresh(20);
    CHECK(uiForceAvailable == 7);
    CHECK(!memcmp(ranks, uiForcePowersRank, sizeof(ranks)));
    FullLoadout();
    engineFreeSaber = 0;
    UI_Refresh(30);
    CHECK(uiForceAvailable >= 0 && uiForceUsed <= 100);
    /* An actual overspend must still be reduced. */
    CHECK(uiForcePowersRank[FP_SABER_DEFENSE] == 2);
}
static void Rank(void) {
    int ranks[NUM_FORCE_POWERS];
    FullLoadout();
    memcpy(ranks, uiForcePowersRank, sizeof(ranks));
    /* Removing freebies is affordable under the new 156-point budget.
       Painting against the old 100-point budget must not trim Heal first. */
    engineFreeSaber = 0;
    engineRankChange = 8;
    paintMenu = 1;
    UI_Refresh(10);
    CHECK(!memcmp(ranks, uiForcePowersRank, sizeof(ranks)));
    CHECK(uiForceAvailable == 54);
    CHECK(savedWrites == 0);
    UI_Refresh(20);
    CHECK(!memcmp(ranks, uiForcePowersRank, sizeof(ranks)));
}
static void Template(void) {
    int ranks[NUM_FORCE_POWERS];
    FullLoadout();
    memcpy(ranks, uiForcePowersRank, sizeof(ranks));
    UI_UpdateClientForcePowers(NULL);
    strcpy(templateData, saved);
    uiInfo.forceConfigCount = 2;
    uiInfo.forceConfigNames[1] = "test";
    UI_ForceConfigHandle(0, 1);
    CHECK(!memcmp(ranks, uiForcePowersRank, sizeof(ranks)));
    CHECK(uiForceAvailable == 0);
    UI_ForceConfigHandle(1, 0);
    CHECK(!memcmp(ranks, uiForcePowersRank, sizeof(ranks)));
    ui_drawTeamForces.integer = 0;
    UI_ForceConfigHandle(0, 1);
    CHECK(!memcmp(ranks, uiForcePowersRank, sizeof(ranks)));
    CHECK(uiForceAvailable == 0);
}
static void Reopen(void) {
    int ranks[NUM_FORCE_POWERS], n;
    FullLoadout();
    memcpy(ranks, uiForcePowersRank, sizeof(ranks));
    UI_UpdateClientForcePowers(NULL);
    savedWrites = 0;
    for (n = 0; n < 20; ++n) {
        memset(uiForcePowersRank, 0, sizeof(uiForcePowersRank));
        UI_UpdateForcePowers();
        UI_ReadLegalForce();
        CHECK(!memcmp(ranks, uiForcePowersRank, sizeof(ranks)));
        CHECK(uiForceAvailable == 0 && savedWrites == 0);
    }
}
static void Open(void) {
    int ranks[NUM_FORCE_POWERS], menu;
    for (menu = UIMENU_PLAYERCONFIG; menu <= UIMENU_PLAYERFORCE; ++menu) {
        FullLoadout();
        memcpy(ranks, uiForcePowersRank, sizeof(ranks));
        engineFreeSaber = 0;
        engineRankChange = 8;
        paintMenu = 1;
        /* The rank event can open this menu before UI_Refresh is called. */
        UI_SetActiveMenu(menu);
        CHECK(!memcmp(ranks, uiForcePowersRank, sizeof(ranks)));
        CHECK(uiForceAvailable == 54);
        CHECK(savedWrites == 0);
        UI_Refresh(10);
        CHECK(!memcmp(ranks, uiForcePowersRank, sizeof(ranks)));
    }
}
int main(int argc, char **argv) {
    CHECK(argc == 2);
    if (!strcmp(argv[1], "costs")) Costs();
    else if (!strcmp(argv[1], "toggle")) Toggle();
    else if (!strcmp(argv[1], "rank")) Rank();
    else if (!strcmp(argv[1], "template")) Template();
    else if (!strcmp(argv[1], "reopen")) Reopen();
    else if (!strcmp(argv[1], "open")) Open();
    else CHECK(0);
    printf("PASS %s\n", argv[1]);
    return 0;
}
