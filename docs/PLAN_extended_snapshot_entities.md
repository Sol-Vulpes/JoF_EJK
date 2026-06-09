# Plan: Extended Snapshot Entities (see more projectiles in busy fights)

## 1. Goal

In crowded firefights, projectiles and other entities **flicker / pop out of
existence**. They are real server entities, but the server can only put a
limited number of them into each per-client snapshot, so the overflow is
silently dropped every frame.

We want our enhanced client to be able to **see more entities per frame**,
**without breaking vanilla clients** connecting to the same server.

Core idea:

- Raise the hard ceiling (compile-time) on our **server** and our **enhanced
  client** so the buffers can hold more entities.
- Keep the *actual* per-client send count **gated by a handshake**: vanilla
  clients keep getting the safe 256; only confirmed-enhanced clients get more.

---

## 2. Root cause recap (verified in code)

The flicker is **NOT** the gentity pool (`MAX_GENTITIES = 1024`,
`q_shared.h:929`). That is the total world entity slots and is unrelated.

The real cap is the **per-client, per-frame snapshot cap**:

```c
#define MAX_SNAPSHOT_ENTITIES 256          // qcommon.h:128
```

Enforced server-side while building each client's snapshot:

```c
// SV_AddEntToSnapshot, sv_snapshot.cpp:350
// if we are full, silently discard entities
if ( eNums->numSnapshotEntities == MAX_SNAPSHOT_ENTITIES ) {
    return;
}
```

Mirrored client-side as the cgame-facing array + truncation:

```c
#define MAX_ENTITIES_IN_SNAPSHOT 256       // cg_public.h:35
// CL_GetSnapshot, cl_cgame.cpp:133 — anything past 256 is thrown away
if ( count > MAX_ENTITIES_IN_SNAPSHOT ) { count = MAX_ENTITIES_IN_SNAPSHOT; }
```

Buffer sizing derived from the cap:

```c
#define MAX_PARSE_ENTITIES (PACKET_BACKUP * MAX_SNAPSHOT_ENTITIES)  // client.h:104
// server pool: svs.numSnapshotEntities = maxclients * PACKET_BACKUP
//              * MAX_SNAPSHOT_ENTITIES   (sv_init.cpp:274/280/366/369)
```

Ultimate client ceiling (far above 256, probably never our bottleneck):

```c
#define MAX_REFENTITIES ((1<<11) - 1)      // = 2047, tr_types.h:34
// REFENTITYNUM_BITS 11 — "can't be increased without changing drawsurf bit packing"
```

---

## 3. Why this is safe to do per-client

- `SV_BuildClientSnapshot(client_t *client)` builds a **fresh snapshot for each
  client** every frame. The `client` pointer is in scope.
- The on-the-wire entity format is a **sentinel-terminated stream** (terminated
  by the `MAX_GENTITIES-1` marker), not a fixed-width count field tied to
  `MAX_SNAPSHOT_ENTITIES`. So the number of entities in a packet is free-form;
  a vanilla client simply must never be *sent* more than its 256 buffers hold.
- Therefore: **enlarge the storage to the new ceiling for everyone, but cap the
  per-client FILL based on a negotiated flag.** Vanilla = 256, enhanced = more.

**Invariant (do not violate):** the server must NEVER place more than 256
entities in a snapshot for a client that has not positively completed the
enhanced handshake. Default off. Opt-in only.

---

## 4. Design: capability negotiation (handshake)

We use an explicit, opt-in handshake so a vanilla client can never accidentally
be over-sent (which would overflow its 256 buffers and crash it).

1. **Server advertises support** via a serverinfo/systeminfo cvar, e.g.
   `sv_extSnapshots` = max entities the server is willing to send (e.g. `1024`).
   Vanilla clients ignore it.
2. **Our client detects** the cvar in the systeminfo and, after connect, sends a
   reliable client command, e.g. `cmd extsnaps 1`.
3. **Server confirms**: in the client-command handler it sets
   `client->extSnapshots = qtrue` and computes the negotiated cap
   `client->maxSnapshotEntities = min(serverMax, compiledMax)`.
   Optionally reply with a server command so the client knows it succeeded.
4. If the handshake never arrives (vanilla, or our client connecting to a
   vanilla server), `client->maxSnapshotEntities` stays at the safe `256`.

> Use a **client command** for the opt-in, not just a spoofable userinfo cvar.
> A userinfo flag alone could be set by a vanilla user by hand and then the
> server would over-send and crash them. A dedicated `cmd extsnaps` is something
> only our client knows to send.

---

## 5. Shared / compile-time changes

These live in headers compiled into multiple binaries. Pick the new ceiling
(suggest `1024`, the natural max since you can't have more live entities than
`MAX_GENTITIES`). Memory cost scales linearly; 1024 is fine.

- `qcommon/qcommon.h:128` — raise `MAX_SNAPSHOT_ENTITIES` (e.g. 256 -> 1024).
  - Auto-scales `MAX_PARSE_ENTITIES` (client) and `svs.numSnapshotEntities`
    pool (server). No other edits needed for those.
- `cgame/cg_public.h:35` — raise `MAX_ENTITIES_IN_SNAPSHOT` to match.
  - This is the **engine <-> cgame ABI** (`snapshot_t.entities[]`). The engine
    binary AND the cgame module MUST be compiled with the same value or the
    struct size mismatches -> memory corruption. Rebuild both together.

> Keep the two constants equal. They represent the same ceiling from two sides.

---

## 6. Server-side changes

### 6.1 Per-client cap state
- `server/server.h` — add to `client_t`:
  ```c
  qboolean extSnapshots;          // completed enhanced handshake
  int      maxSnapshotEntities;   // negotiated cap; default 256
  ```
- Initialize `maxSnapshotEntities = 256`, `extSnapshots = qfalse` on connect
  (SV_ClientConnect / SV_DirectConnect reset path).

### 6.2 Thread the cap into snapshot building
- `server/sv_snapshot.cpp:309` — add a field to `snapshotEntityNumbers_t`:
  ```c
  typedef struct snapshotEntityNumbers_s {
      int numSnapshotEntities;
      int maxSnapshotEntities;                 // NEW: per-build cap
      int snapshotEntities[MAX_SNAPSHOT_ENTITIES]; // array now sized to new ceiling
  } snapshotEntityNumbers_t;
  ```
- `server/sv_snapshot.cpp:350` — change the discard check from the constant to
  the per-build cap:
  ```c
  if ( eNums->numSnapshotEntities >= eNums->maxSnapshotEntities ) {
      return;
  }
  ```
- `server/sv_snapshot.cpp:607` (`SV_BuildClientSnapshot`) — set the cap before
  the visibility walk:
  ```c
  entityNumbers.numSnapshotEntities = 0;
  entityNumbers.maxSnapshotEntities =
      client->extSnapshots ? client->maxSnapshotEntities : MAX_ENTITIES_IN_SNAPSHOT_VANILLA; // 256
  ```
  (Define `MAX_ENTITIES_IN_SNAPSHOT_VANILLA = 256` so the vanilla limit is
  explicit and never accidentally raised.)

### 6.3 (Recommended) Prioritize projectiles before the cap
Even raised, a cap can still clip. So entities that matter most for gameplay
(missiles/projectiles) should be added to the snapshot **first**, so they
survive even if the cap is hit. Options:
- In `SV_AddEntitiesVisibleFromPoint`, do a first pass that adds
  projectile-type entities (e.g. `ET_MISSILE`) before the general pass, or
- Sort/score entities by importance before truncating.
This benefits even vanilla clients (fewer dropped bolts within their 256).

### 6.4 Handshake handler
- `server/sv_ccmds.cpp` / `server/sv_client.cpp` — register `extsnaps` in the
  client-command dispatch (`SV_ExecuteClientCommand`):
  ```c
  // pseudo
  if (!Q_stricmp(cmd, "extsnaps")) {
      int want = atoi(Cmd_Argv(1));
      cl->extSnapshots = qtrue;
      cl->maxSnapshotEntities = Com_Clamp(256, MAX_SNAPSHOT_ENTITIES,
                                          want ? sv_extSnapshots->integer : 256);
      // optional: reply with server command to confirm
  }
  ```
- `server/sv_init.cpp` (or sv_main) — register the advertise cvar:
  ```c
  sv_extSnapshots = Cvar_Get("sv_extSnapshots", "1024",
                             CVAR_SERVERINFO | CVAR_ARCHIVE);
  ```
  Clamp to compiled `MAX_SNAPSHOT_ENTITIES`.

### 6.5 Storage sizing
- No change needed beyond the header bump: `svs.numSnapshotEntities`
  (`sv_init.cpp:274/280/366/369`) already scales with `MAX_SNAPSHOT_ENTITIES`.
  Confirm the larger allocation succeeds (memory = maxclients * PACKET_BACKUP *
  ceiling * sizeof(entityState_t)).

---

## 7. Client-side changes (enhanced client)

- Header bumps in section 5 cover the buffers (parse ring + cgame array +
  truncation point). After those, `CL_GetSnapshot` no longer truncates below
  the new ceiling.
- `client/cl_main.cpp` (or wherever systeminfo is processed,
  `CL_SystemInfoChanged`) — detect `sv_extSnapshots` and, once connected
  (CS_ACTIVE / first snapshot), send `cmd extsnaps 1` once.
- Register a client cvar like `cl_extSnapshots` (default 1) so the user can
  toggle the opt-in.
- (Optional) HUD/debug print showing negotiated cap and current
  `cg.snap->numEntities` so we can see it working.

---

## 8. Risks & edge cases

1. **Vanilla overflow (critical):** never send vanilla >256. Guaranteed by the
   default-off handshake + explicit `MAX_ENTITIES_IN_SNAPSHOT_VANILLA` cap.
2. **Spoofing:** a malicious user could send `cmd extsnaps` from a vanilla
   client without real buffers and crash *themselves*. Their problem, but the
   client-command handshake makes it deliberate, not accidental. Optionally
   gate on a version token.
3. **Packet size / fragmentation:** more entities = bigger snapshots. Verify
   `MAX_MSGLEN` and the netchan fragmentation path handle the larger messages,
   and that client `rate` / `snaps` are high enough. Watch for "rate" throttling
   clipping large snapshots.
4. **Renderer ceiling:** `MAX_REFENTITIES = 2047` still caps simultaneously
   rendered refEnts. Far above 256, but if we push the snapshot cap very high in
   extreme scenes, drops can resume here (hard to raise — bit-packed sort key).
5. **Demo compatibility:** demos recorded by the enhanced client may contain
   >256-entity snapshots and will not play back correctly on vanilla. Note in
   release.
6. **Delta baseline on cap change:** decide the cap at connect/handshake time;
   avoid changing it mid-session, or force a non-delta snapshot when it changes.
7. **Mixed module versions:** engine and cgame MUST share the same
   `MAX_ENTITIES_IN_SNAPSHOT`. Ship them together.

---

## 9. Testing plan

1. **Vanilla regression:** stock JKA/base client connects to our server ->
   confirm normal play, no crash, capped at 256 (add a temp dev print of each
   client's negotiated cap).
2. **Enhanced handshake:** our client connects -> confirm server logs handshake
   and raises `client->maxSnapshotEntities`.
3. **Stress test:** spawn a heavy-projectile scenario (many bots firing rapid
   weapons). Compare `cg.snap->numEntities` and visible projectile count:
   vanilla (~256 ceiling, flicker) vs enhanced (higher, stable).
4. **Bandwidth:** watch `cl_debugMove` / netgraph / `showpackets` for fragment
   counts and dropped packets at the higher cap.
5. **Renderer cap probe:** run `developer 1`, watch for
   "Dropping refEntity, reached MAX_REFENTITIES".

---

## 10. Phased checklist

- [ ] Phase 0: add `MAX_ENTITIES_IN_SNAPSHOT_VANILLA (256)` constant; replace
      the literal `256` discard with the per-build cap field (still always 256).
      No behavior change. Verify vanilla unaffected.
- [ ] Phase 1: raise `MAX_SNAPSHOT_ENTITIES` + `MAX_ENTITIES_IN_SNAPSHOT` to the
      new ceiling. Rebuild engine + cgame. Verify normal play (cap still 256 per
      client via the gate).
- [ ] Phase 2: add `client_t.extSnapshots` / `maxSnapshotEntities`, the
      `sv_extSnapshots` cvar, and the `extsnaps` client-command handshake.
- [ ] Phase 3: client-side systeminfo detection + auto `cmd extsnaps`.
- [ ] Phase 4 (optional): projectile-priority pass in
      `SV_AddEntitiesVisibleFromPoint`.
- [ ] Phase 5: stress test, bandwidth check, demo/regression notes.

---

## 11. Files touched (summary)

| File | Change |
|------|--------|
| `qcommon/qcommon.h` | raise `MAX_SNAPSHOT_ENTITIES` |
| `cgame/cg_public.h` | raise `MAX_ENTITIES_IN_SNAPSHOT` (match) |
| `server/server.h` | `client_t`: `extSnapshots`, `maxSnapshotEntities` |
| `server/sv_snapshot.cpp` | per-build cap field; gate discard; set cap in build; (opt) projectile priority |
| `server/sv_client.cpp` / `sv_ccmds.cpp` | `extsnaps` handshake handler |
| `server/sv_init.cpp` | register `sv_extSnapshots`; confirm pool alloc |
| `client/cl_main.cpp` | detect `sv_extSnapshots`, send `cmd extsnaps`; `cl_extSnapshots` cvar |

> Note: header changes require rebuilding **engine + cgame** together. Server
> and enhanced client are separate binaries from this same source tree; vanilla
> clients are untouched and stay compatible as long as the per-client gate
> holds them at 256.
