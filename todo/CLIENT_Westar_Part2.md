# Client-side changes for JoF JA+ V101 westar

Companion notes for the V101 server binary. The server rewrites the westar
grant path so that any client can hold westar simultaneously; the client
mod currently gates westar rendering / weapon-list state on a single
"current westar player" variable, which breaks the moment a second player
gets granted. This doc describes exactly what the server puts on the wire
per player so the client can key off it per-player instead of globally.

Repo: https://github.com/JediofFreedom/JoF_EJK

## What the server writes into `playerState_t` (per client)

Every field below is **per-player**. Nothing is a global "westar owner"
anywhere on the server side — the client needs to mirror that.

### `ps.eFlags`

| bit                 | meaning                                                      | source                                        |
|---------------------|--------------------------------------------------------------|-----------------------------------------------|
| `0x80000000` (31)   | "This player has been granted westar"                        | Set on `ocgive westar <cn>`. Cleared on disconnect / kill (default `EF_*` cleanup). |
| `0x00001000` (12)   | "This player is currently using westar this frame"           | Set by the server's swap hook when `cmd.weapon == 19` AND grant is set. Cleared the same frame the player switches away or drops westar. |

* **Bit 31** is the durable grant marker — use it to decide "does this
  player's HUD show a westar icon at all, and does their weapon-select
  cycle include weapon 19".
* **Bit 12** is the transient "westar is active on this player right now"
  marker — use it to swap the rendered weapon model to dual pistols and
  swap the firing anim / muzzle flash logic. It's set/cleared every frame
  by the server, so no need to track it across snapshots.

Both bits arrive **independently per-player** in each snapshot. If your
current code has any file-scope variable like `westarActivePlayer`,
`cg.westarOwner`, or similar — that's what needs to change. Read the bit
directly off `cent->currentState.eFlags` (for other players) or
`cg.predictedPlayerState.eFlags` (for the local player).

### `ps.stats[STAT_WEAPONS]` (`[ps + 0xFC]`)

| bit          | weapon               |
|--------------|----------------------|
| `1 << 4`     | `WP_BRYAR_PISTOL`    |
| `1 << 19`    | `WP_BRYAR_OLD`       |

Both bits are set by `ocgive westar`. The server needs bit 4 present so
that the swap hook (which rewrites `cmd.weapon` from 19 to 4 before
`PM_Weapon` runs) has a valid weapon to transition to. **The client should
key HUD weapon-icon rendering off bit 19** — bit 4 might be there from
other reasons and shouldn't imply "show westar UI".

### `ps.stats[5]` (`[ps + 0x104]`)

Same bit layout as `STAT_WEAPONS` for `bits 0..19` (JoF-specific mirror),
plus:

| bit          | meaning                                                        |
|--------------|----------------------------------------------------------------|
| `1 << 20`    | Right/left pistol alternation flag (0 = next shot is right)    |

The alternation bit is the reason the "always right on paused" fix works
— every not-held frame the maintenance hook re-seeds this bit to 1, so
the first shot after a release is always from the right pistol, then it
toggles left/right per shot as normal.

If your client-side pistol-model selection currently reads this bit,
nothing changes. If it doesn't and you're calculating alternation
locally, please switch to reading `ps.stats[5] & (1 << 20)` — the server
is now authoritative on it.

### `ps.ammo[2]`

Westar consumes from **ammo slot 2** (the same slot WP_BLASTER uses in
vanilla). Costs:

| fire mode | cost per shot |
|-----------|---------------|
| primary   | 2 ammo        |
| alt       | 4 ammo        |

* If `ps.ammo[2] == -1`, the weapon has infinite ammo (the admin set
  `jp_weaponAmmo 0`, which the server maps to `-1`). Display it as
  infinite in the HUD.
* Otherwise display it as a plain integer, capped at 999.

Same slot as blaster rifle — that's intentional and matches how the
server wires it. **If you're currently rendering a separate westar ammo
counter, delete that state and read `ammo[2]`.**

## Concrete client changes to make

Wherever you have any of these patterns:

```c
if (cg.westarActive)              { ... }
if (ci->hasWestar)                { ... }
if (ent->westarPlayerNum == n)    { ... }
```

...replace with a **per-player** read of the eFlags bits described above.
The general pattern:

```c
qboolean CG_PlayerHasWestar( entityState_t *es ) {
    return (es->eFlags & EF_WESTAR_GRANTED) ? qtrue : qfalse;
}

qboolean CG_PlayerFiringWestar( entityState_t *es ) {
    return (es->eFlags & EF_WESTAR_ACTIVE) ? qtrue : qfalse;
}
```

Where `EF_WESTAR_GRANTED = (1u << 31)` and `EF_WESTAR_ACTIVE = (1 << 12)`.
(Whatever the current file-scope westar tracking variable is, replace it
with these two per-entity predicates.)

### Weapon list / HUD

* When rebuilding the visible weapon list for the local player (typically
  `CG_BuildWeaponListInfo` or wherever your weapon-cycle logic lives),
  include weapon 19 iff `EF_WESTAR_GRANTED` is set on `cg.predictedPlayerState`.
* When rendering another player's model with dual pistols, gate on
  `EF_WESTAR_ACTIVE` in **that entity's** state, not on a global.
* When drawing the local player's ammo, use `ps.ammo[2]` and treat `-1`
  as infinite (probably the same rendering path you already use for
  cheating/no-ammo modes — hide the number, show an infinity glyph or
  similar).

### Muzzle flash / attack anim

The server's swap hook rewrites `cmd.weapon` to 4 (`WP_BRYAR_PISTOL`) the
frame westar is fired, and sets `ps.torsoAnim = 0x3F6` (which corresponds
to the westar firing anim in your anim.md5mesh). If you have a client-side
prediction path that runs its own `PM_Weapon` and doesn't see `eFlags &
EF_WESTAR_ACTIVE`, it may re-derive an incorrect torso anim / muzzle
offset. Same guidance as above: any local westar state should read the
eFlag bit rather than a cached global.

### R/L alternation

The right/left pistol alternation is now server-authoritative via
`ps.stats[5] & (1 << 20)`. If your client currently maintains its own
`nextShotIsLeft` variable, remove it and read the stat bit. This matters
particularly for other-player rendering — otherwise everyone sees only
one hand fire.

## What the server does NOT rely on the client for

Just for clarity — these are all handled server-side now, no client work
needed:

* Ammo decrement (server subtracts 2 or 4 from `ammo[2]` per shot).
* Ammo cap (server caps at 999, treats -1 as infinite).
* Fire-rate cap (existing V94 code, unchanged).
* R/L alternation state itself (server toggles `bit 20`).
* `WP_BRYAR_PISTOL` grant when westar is granted (server does the OR).

## Test plan

The specific case that fails today with the current EJK client:

1. Empty server.
2. Player A connects (`cn = 0`).
3. `ocgive westar 0`. Player A sees the icon, can equip and fire — ✅.
4. Player B connects (`cn = 1`).
5. `ocgive westar 1`. Player B **sees the icon**, but selecting westar
   leaves them "stuck on the model" without being able to fire — ❌.

Expected after the client change: step 5 works the same as step 3
regardless of how many other players are also in westar. If player A is
actively firing while player B tries to equip, both should render
correctly and independently.

## Server-side reference for the wire format

If it helps for verification, these are the exact server actions on
`ocgive westar <cn>`:

```
jp_westarGrant[cn]      = 1               ; permanent until disconnect/kill
jp_westarFireTime[cn]   = 0
ps.eFlags[cn]          |= 0x80000000      ; grant marker (EF_WESTAR_GRANTED)
ps.stats[STAT_WEAPONS] |= (1 << 4) | (1 << 19)   ; bryar + bryar-old
ps.stats[5]            |= (1 << 4)                ; bryar in the JoF bitmap
ps.ammo[2]              = min(ps.ammo[2] + jp_weaponAmmo, 999)
                        ; (or -1 if jp_weaponAmmo == 0)
```

And on every frame during firing (server's swap hook, per-player):

```
if (ps.clientNum < 32
    AND cmd.weapon == 19
    AND jp_westarGrant[cn] != 0) {
    jp_westarActive[cn] = 1
    cmd.weapon          = 4
    ps.eFlags          |= 0x1000     ; EF_WESTAR_ACTIVE
} else {
    jp_westarActive[cn] = 0
    ps.eFlags          &= ~0x1000
}
```

Note both `jp_westarGrant[cn]` and `jp_westarActive[cn]` are server-only
arrays — the client never sees them. Everything the client needs is
already on `ps` per-player as described above.

Ping me on the server side if any of the bit assignments or the
`ammo[2]` slot need to change — it's cheaper to swap the server bits than
to rework two codebases in parallel.
