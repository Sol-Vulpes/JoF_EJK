# Task: implement opt-in "extended snapshots" on a JKA (Quake3-engine) server

You are working on a Jedi Academy multiplayer **server** codebase (idTech3 /
OpenJK lineage). A companion **modified client** already exists and speaks the
protocol described below — your job is to implement the server half so that
modified clients can receive more than the vanilla 256 entities per snapshot,
while **unmodified (vanilla) clients keep working exactly as today**.

File names and line numbers below refer to standard OpenJK layout
(`codemp/server/...`); if this codebase diverges, locate the equivalent code by
the function names — they are original Quake3 names and almost certainly exist
unchanged.

---

## 1. The problem being solved

In large fights, more than 256 entities (players, projectiles, items) can be
in a player's PVS at once. The per-client snapshot is hard-capped:

```c
// qcommon/qcommon.h
#define MAX_SNAPSHOT_ENTITIES 256
```

```c
// server/sv_snapshot.cpp — SV_AddEntToSnapshot
// if we are full, silently discard entities
if ( eNums->numSnapshotEntities == MAX_SNAPSHOT_ENTITIES ) {
    return;
}
```

The overflow is silently dropped each frame, and because the survivor set
shifts frame to frame, projectiles visibly flicker in and out. The fix is to
let clients that *can* hold more (the modified client buffers 512) opt in to
receiving more, per client.

## 2. Non-negotiable safety invariant

> **A client that has not explicitly opted in must NEVER be sent more than 256
> entities in a snapshot.** Vanilla clients only have buffers for 256; the cap
> must default to 256 for every connection and only rise after the explicit
> `extsnaps` opt-in described below. Opt-in state must not survive into a new
> connection that reuses the same client slot (verify the `client_t` is zeroed
> on connect — in stock code `SV_DirectConnect` memsets a temp and copies it,
> which is sufficient).

## 3. Wire protocol (already implemented client-side — must match exactly)

1. **Server advertises support** via a cvar named exactly `sv_extSnapshots`,
   registered with `CVAR_SYSTEMINFO` so it appears in the `CS_SYSTEMINFO`
   configstring. Value = max entities per snapshot the server is willing to
   send (e.g. `512`). Value `0` (the default) = feature disabled.
   - It must be in **systeminfo** (not just serverinfo): the client engine
     parses systeminfo in `CL_SystemInfoChanged` and that is where the
     companion client looks for it.
2. **Client opts in** by sending a reliable client command after parsing each
   gamestate (so it re-arrives after every map change / map_restart):
   ```
   extsnaps <N>
   ```
   where `<N>` is the client's compiled-in snapshot buffer capacity. The
   companion client sends `extsnaps 512`. The client only sends this when it
   saw `sv_extSnapshots` > 256 in systeminfo, so vanilla servers never see
   this command.
3. **Server grants** `min(N, sv_extSnapshots->integer, server compiled ceiling)`,
   and only if both `N` and `sv_extSnapshots` are > 256; otherwise the client
   stays at the vanilla 256. The grant is stored per client and used as that
   client's snapshot cap from then on.
4. There is no acknowledgment message; the client simply starts receiving
   larger snapshots. (Optional: log the grant server-side for debugging.)

## 4. Implementation steps

### 4.1 Constants (`qcommon/qcommon.h`)

```c
#define MAX_SNAPSHOT_ENTITIES         512  // new engine ceiling
#define MAX_SNAPSHOT_ENTITIES_VANILLA 256  // never exceeded without opt-in
```

Notes:
- 512 was chosen deliberately over 1024: the server snapshot ring is allocated
  as `sv_maxclients * PACKET_BACKUP * MAX_SNAPSHOT_ENTITIES * sizeof(entityState_t)`
  (see `SV_Startup` / `SV_ChangeMaxClients` in `sv_init.cpp`) and 1024
  quadruples a buffer that is already large; 512 also keeps worst-case
  snapshot size comfortably under `MAX_MSGLEN` (49152). **Do not grant more
  than 512 anyway — the companion client's buffers are 512.**
- Everything sized from `MAX_SNAPSHOT_ENTITIES` (the `svs.numSnapshotEntities`
  pool, `snapshotEntities[]` array) scales automatically from the define.
- If this server build shares headers with a client build, note the client-side
  derived constant `MAX_PARSE_ENTITIES = PACKET_BACKUP * MAX_SNAPSHOT_ENTITIES`
  must remain a power of two (it is used with `& (MAX_PARSE_ENTITIES-1)`
  masking). With PACKET_BACKUP 32 or 128 and the cap 512, it is.

### 4.2 Per-client state (`server/server.h`)

Add to `client_t`:

```c
int maxSnapshotEntities; // granted via "extsnaps" opt-in; 0 = vanilla 256
```

No explicit init needed if connections memset the struct (verify — see §2).

### 4.3 Advertise cvar (`server/sv_init.cpp` or wherever sv_ cvars register)

```c
sv_extSnapshots = Cvar_Get( "sv_extSnapshots", "0",
    CVAR_SYSTEMINFO | CVAR_ARCHIVE,
    "Entities per snapshot offered to clients that opt in via the extsnaps command (0: vanilla 256 for everyone)" );
Cvar_CheckRange( sv_extSnapshots, 0, MAX_SNAPSHOT_ENTITIES, qtrue );
```

Plus the usual `cvar_t *sv_extSnapshots;` definition and `extern` declaration.

### 4.4 Opt-in command handler (`server/sv_client.cpp`)

Register in the engine-level `ucmds[]` table (the one containing `"userinfo"`,
`"disconnect"`, `"download"`...). Being handled there means it is NOT forwarded
to the game module, so no mod compatibility concerns.

```c
static void SV_ExtSnapshots_f( client_t *cl ) {
    int limit = atoi( Cmd_Argv( 1 ) );

    if ( sv_extSnapshots->integer <= MAX_SNAPSHOT_ENTITIES_VANILLA ||
         limit <= MAX_SNAPSHOT_ENTITIES_VANILLA ) {
        cl->maxSnapshotEntities = 0; // stay vanilla
        return;
    }
    if ( limit > sv_extSnapshots->integer ) limit = sv_extSnapshots->integer;
    if ( limit > MAX_SNAPSHOT_ENTITIES )    limit = MAX_SNAPSHOT_ENTITIES;

    cl->maxSnapshotEntities = limit;
    Com_DPrintf( "Client %i opted in to extended snapshots (%i entities)\n",
                 (int)(cl - svs.clients), limit );
}
...
static ucmd_t ucmds[] = {
    {"userinfo", SV_UpdateUserinfo_f},
    {"disconnect", SV_Disconnect_f},
    {"extsnaps", SV_ExtSnapshots_f},   // <-- add
    ...
```

### 4.5 Per-client cap in the snapshot builder (`server/sv_snapshot.cpp`)

Add a cap field to the build-time struct:

```c
typedef struct snapshotEntityNumbers_s {
    int numSnapshotEntities;
    int maxSnapshotEntities;   // <-- add: per-client cap for this build
    int snapshotEntities[MAX_SNAPSHOT_ENTITIES];
} snapshotEntityNumbers_t;
```

In `SV_AddEntToSnapshot`, replace the constant check with the field:

```c
// if we are full, silently discard entities
if ( eNums->numSnapshotEntities >= eNums->maxSnapshotEntities ) {
    return;
}
```

In `SV_BuildClientSnapshot`, right where `entityNumbers.numSnapshotEntities`
is zeroed, set the cap from the client:

```c
entityNumbers.numSnapshotEntities = 0;
// never exceed the vanilla limit unless this client opted in via "extsnaps"
if ( client->maxSnapshotEntities > MAX_SNAPSHOT_ENTITIES_VANILLA ) {
    entityNumbers.maxSnapshotEntities = client->maxSnapshotEntities;
} else {
    entityNumbers.maxSnapshotEntities = MAX_SNAPSHOT_ENTITIES_VANILLA;
}
```

`snapshotEntityNumbers_t` is only ever instantiated in
`SV_BuildClientSnapshot` (portal recursion passes the same instance), so this
is the single choke point — but verify with a grep in this codebase.

### 4.6 Why nothing else needs to change

- The on-the-wire entity list is **sentinel-terminated**
  (`MSG_WriteBits(msg, (MAX_GENTITIES-1), GENTITYNUM_BITS)` ends it), not a
  fixed-size field, so sending more entities needs no message-format change
  and is invisible to clients that are never sent more than 256.
- Delta compression works per entity list; frames of different sizes delta
  against each other fine.
- Bots / local clients never send `extsnaps` → field stays 0 → vanilla cap.

## 5. Things to check in THIS codebase (don't assume)

1. **Slot reuse**: confirm `client_t` is fully zeroed for each new connection
   (stock `SV_DirectConnect` does `Com_Memset(&temp, 0, sizeof(client_t))`
   then `*newcl = temp;` at the `gotnewcl:` label). If this server has custom
   connect paths (e.g. for bots or split reconnect handling), make sure
   `maxSnapshotEntities` cannot leak between occupants of a slot.
2. **Custom snapshot logic**: JA+/japro-style servers sometimes add culling or
   per-client visibility hacks in `SV_AddEntitiesVisibleFromPoint`. Make sure
   no other code path writes into `snapshotEntities[]` without going through
   `SV_AddEntToSnapshot`.
3. **Memory**: the snapshot ring (`svs.snapshotEntities`) doubles. With the
   stock formula and a dedicated server this is
   `maxclients * PACKET_BACKUP * 512 * sizeof(entityState_t)` — check the
   allocation (`Z_Malloc` in `SV_Startup`/`SV_SpawnServer`) succeeds,
   especially on 32-bit builds.
4. **Rate limiting / command filtering**: if the server filters unknown or
   flood-y client commands before the `ucmds[]` dispatch, whitelist
   `extsnaps`.
5. **sizeof(entityState_t) mismatch**: this design does NOT change
   entityState_t or any message encoding — if you find yourself changing
   either, stop; something is wrong.

## 6. Acceptance tests

1. **Vanilla regression** (most important): connect with a stock/basejka
   client while `sv_extSnapshots 512` is set. Play a heavy scene. The client
   must never receive >256 entities (temporarily log
   `entityNumbers.numSnapshotEntities` per frame to verify ≤256) and must not
   crash or desync.
2. **Opt-in path**: connect with the companion modified client. Server log
   (developer 1) should show the opt-in grant. In a busy scene, that client's
   per-frame entity count should exceed 256 and projectile flicker should
   disappear.
3. **Map change**: `map_restart` and full map change — the companion client
   re-sends `extsnaps` on every gamestate; confirm the grant persists/renews.
4. **Slot reuse**: have a modified client disconnect, then a vanilla client
   take the same slot; verify the new occupant is capped at 256.
5. **Disabled state**: with `sv_extSnapshots 0`, an `extsnaps 512` command
   must leave the client at 256.
6. **Manual abuse**: from a stock client console, `/extsnaps 512` — with the
   feature enabled this grants 512 to a client that can only parse 256; stock
   clients truncate oversized snapshots in `CL_GetSnapshot` (graceful), but
   confirm no server-side assumption breaks. This is self-inflicted and
   acceptable, but it must not affect the server or other players.

## 7. Out of scope (do not do)

- Do not raise `MAX_GENTITIES`, `GENTITYNUM_BITS`, or any `entityState_t`
  field encoding — protocol-breaking and unnecessary.
- Do not send >512 to the companion client even if your ceiling is higher —
  its buffers are 512 (it requests its capacity in the command; respect it).
- Do not make the feature default-on for everyone or tie it to userinfo
  (userinfo is user-editable; a vanilla user could set a flag by hand and get
  over-sent. The explicit `extsnaps` client command is the opt-in for exactly
  this reason).
