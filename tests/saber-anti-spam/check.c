#include "cgame/cg_local.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];
vmCvar_t cg_saberAntiSpam;
static snapshot_t snapshot;
static cgameImport_t imports;
cgameImport_t *trap = &imports;

static int started;
static qboolean staffShutdownSounded;
static int ghoul2;

static void StartSound(const vec3_t origin, int entityNum, int channel, sfxHandle_t sfx) {
	(void)origin; (void)entityNum; (void)channel; (void)sfx;
	++started;
}

// The local view's weapon model is the weapon number, so a switch is a new instance.
void *CG_G2WeaponInstance(centity_t *cent, int weapon) { (void)cent; return (void *)(intptr_t)(weapon + 1); }
void CG_CopyG2WeaponInstance(centity_t *cent, int weaponNum, void *toGhoul2) { (void)cent; (void)weaponNum; (void)toGhoul2; }
void BG_SI_SetDesiredLength(saberInfo_t *saber, float len, int bladeNum) { (void)saber; (void)len; (void)bladeNum; }
qboolean CG_StaffSwapShutdownSounded(int clientNum) { (void)clientNum; return staffShutdownSounded; }
qboolean CG_StaffSwapHoldIgnitionSound(int clientNum, sfxHandle_t sound) { (void)clientNum; (void)sound; return qfalse; }

#include "actual.h"

static void Setup(void) {
	memset(&cg, 0, sizeof(cg));
	memset(&cgs, 0, sizeof(cgs));
	memset(cg_entities, 0, sizeof(cg_entities));
	memset(cg_saberToggleBudget, 0, sizeof(cg_saberToggleBudget));
	memset(&snapshot, 0, sizeof(snapshot));
	cg.snap = &snapshot;
	cg_saberAntiSpam.integer = 1;
	imports.S_StartSound = StartSound;
	started = 0;
	staffShutdownSounded = qfalse;
}

// One toggle sound asked for at render time `time`, from the snapshot of `serverTime`.
static qboolean Allowed(int client, int time, int serverTime) {
	cg.time = time;
	snapshot.serverTime = serverTime;
	return (qboolean)!CG_SaberToggleThrottled(client);
}

static void BurstThenOnePerInterval(void) {
	int i, played = 0;
	Setup();
	for (i = 0; i < 40; ++i)
		played += Allowed(0, 1000 + i * 50, 1000 + i * 50);
	// Two seconds of spam: the burst, then one every 500 ms.
	CHECK(played == SABER_TOGGLE_BURST + 3);
	// Two quiet seconds later the whole burst is back.
	for (i = 0; i < SABER_TOGGLE_BURST; ++i)
		CHECK(Allowed(0, 5000 + i * 50, 5000 + i * 50));
	CHECK(!Allowed(0, 5200, 5200));
}

static void OneSnapshotsHiltsShareAnAnswer(void) {
	int i;
	Setup();
	for (i = 0; i < SABER_TOGGLE_BURST; ++i)
		CHECK(Allowed(3, 1000 + i * 50, 1000 + i * 50));
	// Both hilts of the next toggle are held back together.
	CHECK(!Allowed(3, 1200, 1200));
	CHECK(!Allowed(3, 1200, 1200));
	// The two hilts of an allowed toggle play together and pay once.
	Setup();
	CHECK(Allowed(3, 1000, 1000));
	CHECK(Allowed(3, 1000, 1000));
	for (i = 1; i < SABER_TOGGLE_BURST; ++i)
		CHECK(Allowed(3, 1000 + i * 50, 1000 + i * 50));
	CHECK(!Allowed(3, 1200, 1200));
}

static void SnapshotsRunInOneFramePayEach(void) {
	int i, played = 0;
	Setup();
	// A backlog: 20 snapshots' toggles all processed at one cg.time.
	for (i = 0; i < 20; ++i)
		played += Allowed(0, 1000, 1000 + i * 25);
	CHECK(played == SABER_TOGGLE_BURST);
}

static void PlayersAreApartAndNpcsUncapped(void) {
	int i;
	Setup();
	for (i = 0; i < SABER_TOGGLE_BURST; ++i)
		Allowed(3, 1000 + i * 50, 1000 + i * 50);
	CHECK(!Allowed(3, 1200, 1200));
	CHECK(Allowed(4, 1200, 1200));
	for (i = 0; i < 20; ++i)
		CHECK(Allowed(MAX_CLIENTS + 8, 1000 + i * 50, 1000 + i * 50));
	cg_saberAntiSpam.integer = 0;
	for (i = 0; i < 20; ++i)
		CHECK(Allowed(3, 1300 + i * 50, 1300 + i * 50));
}

static void TimeGoingBackStartsAfresh(void) {
	int i;
	Setup();
	for (i = 0; i < 10; ++i)
		Allowed(0, 50000 + i * 50, 50000 + i * 50);
	for (i = 0; i < SABER_TOGGLE_BURST; ++i)
		CHECK(Allowed(0, 1000 + i * 50, 1000 + i * 50));
}

// The local view switching from a lit saber to fists at `time`.
static void PutAway(playerState_t *ps, centity_t *cent, int time) {
	cg.time = time;
	snapshot.serverTime = time;
	ps->weapon = WP_MELEE;
	cent->weapon = WP_SABER;
	cent->ghoul2weapon = CG_G2WeaponInstance(cent, WP_SABER);
	CG_CheckPlayerG2Weapons(ps, cent);
}

static void SwitchPathPaysOnlyForASound(void) {
	playerState_t ps;
	centity_t *cent = &cg_entities[0];
	int i;

	Setup();
	memset(&ps, 0, sizeof(ps));
	cent->ghoul2 = &ghoul2;
	cgs.clientinfo[0].saber[0].soundOff = 11;
	cgs.clientinfo[0].saber[1].soundOff = 12;
	cgs.clientinfo[0].saber[1].model[0] = 'd';

	// Duals: both hilts sound together.
	PutAway(&ps, cent, 1000);
	CHECK(started == 2);

	// A staff whose shutdown already sounded as it went onto the back: the weapon change's
	// late copy is silent and costs nothing, so the full burst is still there.
	Setup();
	memset(&ps, 0, sizeof(ps));
	cent->ghoul2 = &ghoul2;
	cgs.clientinfo[0].saber[0].soundOff = 11;
	CHECK(Allowed(0, 1000, 1000));	// the shutdown on the back
	staffShutdownSounded = qtrue;
	PutAway(&ps, cent, 1200);
	CHECK(started == 0);
	for (i = 1; i < SABER_TOGGLE_BURST; ++i)
		CHECK(Allowed(0, 1200 + i * 50, 1200 + i * 50));
}

int main(void) {
	BurstThenOnePerInterval();
	OneSnapshotsHiltsShareAnAnswer();
	SnapshotsRunInOneFramePayEach();
	PlayersAreApartAndNpcsUncapped();
	TimeGoingBackStartsAfresh();
	SwitchPathPaysOnlyForASound();
	puts("saber anti-spam checks passed");
	return 0;
}
