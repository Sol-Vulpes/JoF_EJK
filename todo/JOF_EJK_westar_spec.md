# JoF EJK — Westar Dual-Pistol Client Implementation Spec

**Target repo:** `JediofFreedom/JoF_EJK`
**Server counterpart:** `JoF_JA__V78 westar.so` (v13, `md5 377b82aa353c7ef8e0ae9ac084aa9b73`)
**Status:** server side complete and shipping. Client side pending.

---

## 1. What the server sends and why

The server introduces a second pistol slot (WP_WESTAR, index 19) that fires two blaster bolts — alternating right/left on primary, both simultaneously on alt-fire — at 3× vanilla rate. It shares the FireBryarPistol firing routine so behaviour on the wire is a normal blaster-pistol projectile.

The server does **not** send weapon 19 in `ps.weapon` / `es.weapon`. Every frame that a westar-owning player has selected slot 19, the server:

- forces the outgoing `cmd.weapon` to 4 (blaster pistol) before PmoveWeapon runs, so PM_Weapon treats it as a blaster fire
- broadcasts a distinguishing bit in `eFlags` so *your* cgame can render/sound the westar variant

Everything the client needs to distinguish "this player is currently firing westar" from "this player is firing regular blaster" travels in **one bit** of eFlags.

**Vanilla-client safety:** anyone connecting with a stock EternalJK / OpenJK cgame ignores the bit and sees a normal blaster pistol with normal fire sound. No silence, no crash, no visual glitch.

---

## 2. The wire signal: `EF_WESTAR_MODE`

Add to `codemp/game/bg_public.h`, redefining an already-unused slot:

```c
// bit 12 — was EF_NOT_USED_1
#define EF_WESTAR_MODE          (1<<12)         // player is wielding the westar dual-pistol
```

Delete or comment the existing `#define EF_NOT_USED_1 (1<<12)` line at the same time so the two don't collide.

**Why bit 12:** the vanilla bg_public.h already labels this slot as unused; disassembly of the JA+ server confirms no direct eFlags immediate-mask op touches it in the shipped `.so`. There is no risk of collision with EF_JETPACK_FLAMING, EF_ALT_DIM, EF_GRAPPLE_SWING, sitting animations, or any other JA+ specific bit. (Sitting emotes are pure-anim in this codebase — no eFlags involvement — so there's nothing on that side to worry about either.)

**Field layout note:** the JA+ server uses `playerState_t.eFlags` at struct offset `0x88` and `entityState_t.eFlags` at struct offset `0xc`. `BG_PlayerStateToEntityState` copies ps→es unchanged — you don't need to touch either offset yourself, this is just for reference if you're binary-diffing.

**Prediction copy for the local player:** in the `#ifdef _CGAME` block that already carries `EF_ALT_DIM` and `EF_GRAPPLE_SWING`, add:

```c
#define EF_WESTAR_MODE_PRED     (1<<12)         // same value, exists so the cgame
                                                // prediction pmove has the symbol
```

(You can just reuse `EF_WESTAR_MODE` — the point is that whatever guards `EF_ALT_DIM` for prediction should also guard whatever pmove logic you add for westar.)

---

## 3. New weapon slot: `WP_WESTAR`

Add to `bg_public.h`'s `weapon_t` enum:

```c
typedef enum {
    WP_NONE = 0,
    ...
    WP_STUN_BATON,       // 1
    WP_MELEE,            // 2
    WP_SABER,            // 3
    WP_BRYAR_PISTOL,     // 4
    WP_BLASTER,          // 5
    ...
    WP_TURRET,           // 17
    WP_EMPLACED_GUN,     // 18
    WP_WESTAR,           // 19  ← NEW
    WP_NUM_WEAPONS
} weapon_t;
```

`WP_NUM_WEAPONS` will become 20. Server-side, stats bit 19 is set in `stats[STAT_WEAPONS]` when a player is granted westar via `ocgive westar`. Because vanilla clients have `WP_NUM_WEAPONS == 19`, their weapon-cycling code skips bit 19 entirely — they simply cannot select it, and they cannot see it in the wheel. That's the intended fall-through.

**Do not** put a WP_WESTAR icon or model registration inside any code path that vanilla clients need to run — put the whole path behind an `if (i < WP_NUM_WEAPONS)` bound. That's already the norm in cgame weapon-loading loops but worth double-checking.

---

## 4. Assets to register

| Path | Purpose |
|---|---|
| `models/weapons2/westar34/westar34.glm` | weapon model, attached to both `*weapon` and `*hand_l` tags |
| `sound/jof/westarfire.mp3` | primary fire sound |
| `sound/jof/westaraltfire.mp3` | alt-fire sound (both bolts simultaneously) |
| `gfx/jof/w_icon_westar.tga` | wheel/HUD icon |
| `gfx/jof/w_icon_westar_na.tga` | greyed-out variant when unowned |

Ship these in a `pk3` alongside the client release. Vanilla clients without the pk3 don't reference these paths, so nothing breaks for them.

---

## 5. Client changes by area

### 5.1 `cg_weapons.c` — `CG_RegisterWeapon`

Add the WP_WESTAR case. Follow the exact pattern of WP_BRYAR_PISTOL / WP_BLASTER but with your westar assets:

```c
case WP_WESTAR:
    MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
    weaponInfo->missileModel   = trap_R_RegisterModel("models/weapons2/blaster/bryarshot.md3");  // reuse bolt
    weaponInfo->missileSound   = trap_S_RegisterSound("sound/weapons/bryar/fire.wav", qfalse);  // travel loop
    weaponInfo->firingSound    = 0;
    weaponInfo->chargeSound    = 0;
    weaponInfo->flashSound[0]  = trap_S_RegisterSound("sound/jof/westarfire.mp3", qfalse);
    weaponInfo->altFlashSound[0] = trap_S_RegisterSound("sound/jof/westaraltfire.mp3", qfalse);
    weaponInfo->weaponModel    = trap_R_RegisterModel("models/weapons2/westar34/westar34.glm");
    weaponInfo->handsModel     = 0;  // no separate hands
    break;
```

Fall-back registration — if the pk3 wasn't loaded, `trap_S_RegisterSound` returns 0 and `trap_S_StartSound` silently no-ops. To avoid inaudible fires if a westar-owning player joins a client running mismatched pk3s, defensive-init:

```c
if ( !weaponInfo->flashSound[0] ) {
    weaponInfo->flashSound[0] = cgs.media.bryarPistolFireSound;  // fall back to regular
}
if ( !weaponInfo->altFlashSound[0] ) {
    weaponInfo->altFlashSound[0] = cgs.media.bryarPistolFireSound;
}
```

### 5.2 `cg_players.c` / `cg_weapons.c` — model rendering on other players

Where the code currently picks the weapon model for a player from `cent->currentState.weapon`, add a westar override:

```c
int renderWeapon = cent->currentState.weapon;
if ( cent->currentState.eFlags & EF_WESTAR_MODE ) {
    renderWeapon = WP_WESTAR;
}
CG_AddPlayerWeapon( ... , renderWeapon, ... );
```

The server sends `es.weapon = 4` (blaster pistol) even when the player is holding westar. The bit tells you to substitute WP_WESTAR for the render.

**Second pistol on left hand:** attach `westar34.glm` a second time to the `*hand_l` bolt. The simplest place is inside `CG_AddPlayerWeapon` under an `if ( weaponNum == WP_WESTAR )` guard. Use `trap_G2API_AttachG2Model` with the character's ghoul2 as parent and pass the tag name `"*hand_l"` — mirror the existing `*weapon` attach code. Set the same shader/skin used for the primary pistol so both hands match.

### 5.3 Fire event routing — `cg_event.c` / `cg_effects.c`

Both `EV_FIRE_WEAPON` and `EV_ALT_FIRE` need to check the eFlags bit on the entity that fired and select the westar sound / muzzle-flash effect if set.

The dispatch already reads `es.weapon` to key into `cg_weapons[]`. Add:

```c
case EV_FIRE_WEAPON:
    firingWeapon = es->weapon;
    if ( es->eFlags & EF_WESTAR_MODE ) {
        firingWeapon = WP_WESTAR;
    }
    CG_FireWeapon( cent, firingWeapon, qfalse );
    break;
```

Same treatment for `EV_ALT_FIRE`.

Inside `CG_FireWeapon`, the `weaponInfo->flashSound[rand()%N]` / `altFlashSound` lookup will now pull the westar sounds you registered in 5.1.

### 5.4 Weapon selection UI — `cg_weapons.c`, `cg_draw.c`

Anywhere a UI enumerates `1 .. WP_NUM_WEAPONS`, bit 19 will now be reachable. The wheel, the HUD ammo/current-weapon widget, and `CG_NextWeapon_f` / `CG_PrevWeapon_f` should already iterate up to `WP_NUM_WEAPONS`, so bumping the enum's tail is all that's needed for them to include westar.

Verify these three touch points:
- `CG_NextWeapon_f` cycles through `stats[STAT_WEAPONS]` bits — should include bit 19 now
- `CG_WeaponSelectable` returns qtrue for weapon 19 if bit 19 is set in stats
- Weapon wheel drawing loop iterates through weapons — should render the WP_WESTAR icon slot

If any of these hardcode `WP_NUM_WEAPONS - 1 = 18`, replace with the updated bound.

### 5.5 Prediction — `bg_pmove.c` / `PM_Weapon`

The server's `CODE_HELD` intercepts PM_Weapon entry and rewrites `pm->cmd.weapon = 4` whenever a player owning westar has selected weapon 19. To match server prediction *exactly*, mirror this in the client's pmove:

```c
// At the top of PM_Weapon, just after fetching pm->cmd.weapon:
if ( pm->cmd.weapon == WP_WESTAR &&
     (pm->ps->stats[STAT_WEAPONS] & (1 << WP_WESTAR)) ) {
    // Mirror server: fire mechanics use blaster pistol, but the eFlags bit
    // stays set so rendering still shows westar.
    pm->cmd.weapon = WP_BRYAR_PISTOL;
    pm->ps->eFlags |= EF_WESTAR_MODE;
} else {
    pm->ps->eFlags &= ~EF_WESTAR_MODE;
}
```

Without this mirror, the local player's own view will predict `ps.weapon == 19` on frames they select westar, then snap-back to 4 when the server ack arrives — visible as brief weapon flicker. With the mirror, prediction matches server and there's no snap.

The stats-bit gate matters here: don't force `cmd.weapon` to 4 if the player doesn't own westar (stat bit 19 clear). That preserves normal behavior for anyone using the pk3 but who hasn't been granted westar.

### 5.6 Weapon-owned check for cheat prevention

If your existing anti-cheat / weapon-validation code checks `stats[STAT_WEAPONS] & (1 << ps.weapon)` to verify the player owns what they're firing, it will now see `ps.weapon == 4` and check bit 4 — which passes as long as they own a regular pistol. That's fine. But if you want a stricter "you must own the mode you're using" check, add:

```c
if ( ps->eFlags & EF_WESTAR_MODE ) {
    if ( !(ps->stats[STAT_WEAPONS] & (1 << WP_WESTAR)) ) {
        // impossible under normal play — server would have cleared the bit
        return CHEAT_DETECTED;
    }
}
```

The server-side `CODE_HELD` gates the eFlags-set path on `OWNED[cn]` which is only true after `ocgive westar` for that clientNum, so this is defensive only.

---

## 6. Left-hand muzzle position (nice-to-have)

The server currently calculates the left-hand bolt origin using a mirror of `vright` (the player's right vector) rather than the actual `*hand_l` tag position. This works well from a distance but the bolt doesn't emerge from the exact left-hand muzzle location.

If you want visual perfection, the client can override this: intercept the incoming projectile in `CG_AddPacketEntities` when it's a fresh bryar bolt fired by a westar-mode player, and if the bolt origin is closer to the right hand than the left, snap it to the `*hand_l` tag position for the first frame.

Alternatively, we can revisit this server-side later — the `CODE_LEFT_MUZZLE` routine in the `.so` (currently dead code) is a stub for calling `G_GetBoltPosition` on the `*hand_l` tag, but it fails because of a `trap_G2API_AddBolt` handle-convention issue (see server session notes — probably needs `lea` instead of `mov` on `&ent->ghoul2`). Not blocking for this feature ship.

---

## 7. Testing checklist

| Test | Expected |
|---|---|
| Vanilla EternalJK client joins server, observes westar-owning player firing | Sees regular blaster model, hears regular blaster sound, no crash |
| Vanilla client, own player, no westar owned | Fires regular blaster normally |
| Patched EJK client, no westar owned | Fires regular blaster normally, bit 12 stays clear |
| Patched EJK client, granted westar via `ocgive westar`, still on weapon 4 | Regular blaster fire rate, single bolt, single-hand anim, no westar sound |
| Patched EJK client, switch to weapon 19 | Westar model on right AND left hand, dual-fire animation, 3× fire rate, alternating L/R bolts, westar sound |
| Patched EJK client, weapon 19, alt-fire | Both bolts simultaneous, alt-fire sound |
| Cycle weapon 4 → 19 → 4 → 19 rapidly | No prediction snap on own view, sound/model swaps cleanly |
| Two westar players in view of each other | Each sees the other's westar correctly |
| Kill a westar-mode player | Corpse drops with normal blaster (server-side handling — no client work needed) |
| `/cg_thirdperson 1` while in westar mode | Both pistols render correctly in third-person view |

---

## 8. Reference values

| Constant | Value | Notes |
|---|---|---|
| `EF_WESTAR_MODE` | `1 << 12` = `0x1000` | replaces `EF_NOT_USED_1` |
| `WP_WESTAR` | `19` | new weapon slot |
| `STAT_WEAPONS` bit for westar | `1 << 19` = `0x80000` | ownership indicator |
| Server fire-delay divisor | `/ 3` | rate is 3× vanilla blaster pistol |
| Server anims | idle `852` (BOTH_SABERDUAL_STANCE), fire `1014` (BOTH_GUNSIT1) | already handled server-side |
| Fire routine | reuses `FireBryarPistol` | sends `EV_FIRE_WEAPON` with `weapon=4` |

---

## 9. Points of contact / open questions

- Server-side dead code `CODE_LEFT_MUZZLE` at server VA `0x02290a00` awaits a fix (probably `lea` vs `mov` on the ghoul2 handle passed to `trap_G2API_AddBolt`). Not needed for shipping.
- If EJK's `WP_NUM_WEAPONS` exceeds 19 in the future for reasons unrelated to westar, verify bit 19 still round-trips through your snapshot serialization.
- Consider whether `stats[STAT_WEAPONS]` bit 19 should show anything in the scoreboard / player-info panel (e.g., a "has westar" badge). Currently no server-side signalling for that beyond the stats bit.
