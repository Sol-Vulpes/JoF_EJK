# jp_empowerEffect — trim the empower glow

An empowered player (`/amempower`) lights up with the same 8-orb body-blur "glow" as someone who is
being force-pushed. That is not a coincidence: JA+ reused the `EF_BODYPUSH` eFlag for empower, so the
two share the exact clientside code path (`CG_ForcePushBodyBlur` in `cg_players.c`). Admins wanted a
lighter empower glow — fewer orbs, less renderer load from a server full of empowered players — but
you cannot just trim the shared bone list without also changing what force push looks like for
everyone.

`jp_empowerEffect` is the server-side cvar that fixes this. The server now sets a **new**
`EF_EMPOWERED` flag *alongside* `EF_BODYPUSH` while a player is empowered, and ships the cvar value in
serverinfo. A patched client can finally tell empower and force-push apart, and draws a reduced bone
set for empower while leaving genuine force push at the full 8.

- Commit: [`265161c`](https://github.com/Sol-Vulpes/JoF_EJK/commit/265161c13e057e4f259108ab97b889196e17ef45)
- Same doc as a rendered page: [`jp-empower-effect.html`](jp-empower-effect.html) (open it locally — GitHub shows HTML as source)

## How to use

`jp_empowerEffect` is a **server** cvar (`CVAR_SERVERINFO`). Set it on the server; every patched
client reading that server's serverinfo picks it up.

```
jp_empowerEffect 0    // default — full 8-orb glow, identical to today
jp_empowerEffect 1    // arms only: both hands + both forearms (4 orbs)
jp_empowerEffect 2    // hands only: one bolt per hand (2 orbs)
```

No reconnect is needed on the client — the value arrives in the serverinfo configstring and the next
frame draws the new bone set. Force push is **never** affected by this cvar; it always draws the full
set.

## How it works

The empower/push glow is drawn entirely clientside. `CG_ForcePushBodyBlur` loops a hardcoded bone
list (`cg_pushBoneNames[]`) and spawns a blur sprite on each bolt — head, belly, both hands, both
forearms, both shins. That is the "8 orbs."

Because empower and push both raise `EF_BODYPUSH`, the client had no way to draw one differently from
the other. The fix adds a second flag:

| Flag | Meaning |
|---|---|
| `EF_BODYPUSH` (bit 19) | Set while being force-pushed **and** while empowered (unchanged). |
| `EF_EMPOWERED` (bit 18) | **New.** Set *in addition to* `EF_BODYPUSH` while empowered. |

Empower therefore sets **both** bits. The client checks `EF_EMPOWERED` first: if it is set, the player
is empowered and draws the `jp_empowerEffect` bone set; otherwise a raised `EF_BODYPUSH` means a
genuine push and draws the full set. The two are mutually exclusive, so the effects never stack.

## Where it lives

| File | What it does |
|---|---|
| `codemp/game/bg_public.h` | Claims the free bit 18 as `EF_EMPOWERED` (was `EF_NOT_USED_4`). Must stay `1<<18` — the server patch hardcodes it. |
| `codemp/cgame/cg_local.h` | Adds `cgs.empowerEffect`. |
| `codemp/cgame/cg_servercmds.c` | `CG_ParseServerinfo` reads `jp_empowerEffect` into `cgs.empowerEffect`. |
| `codemp/cgame/cg_players.c` | Two extra bone lists; `CG_ForcePushBodyBlur` factored into `CG_ForcePushBodyBlurBones(cent, boneNames)`; new `CG_EmpowerBodyBlur` picks the list from the cvar; the `CG_Player` call site branches on `EF_EMPOWERED` first. |

The `CG_Player` empower branch sits **inside** the existing `cg_hideForcePushFX` / zoom / third-person
guards, so empower obeys the same FX-hide toggle as push — empower spam is exactly what overloads the
renderer, so hiding push FX hides empower FX too.

## Backwards compatibility

The server sets **both** flags rather than swapping one for the other, which is what keeps old clients
working:

| Client | Sees | Result |
|---|---|---|
| Old / vanilla JA+ / unpatched JoF | `EF_BODYPUSH` only (bit 18 is meaningless to it) | Renders the full 8 orbs — exactly today's behavior. `jp_empowerEffect` is silently ignored. |
| Patched JoF | `EF_BODYPUSH` + `EF_EMPOWERED` | The `EF_EMPOWERED` branch wins and draws the `jp_empowerEffect` bone set. |

The server can be deployed on its own; clients update on their own schedule. The cvar simply has no
visible effect until a player is on a patched client.

## Known limits

- **The mutual exclusivity is load-bearing.** Empower raises `EF_BODYPUSH` too, so if the client ran
  both branches an empowered player would draw the trimmed set *on top of* the full push set and you'd
  see all 8 regardless of the cvar. The empower branch checks `EF_EMPOWERED` first and the push branch
  is the `else`, so exactly one runs.
- **Empowered *and* pushed at the same time:** the empower branch wins, so you see the trimmed set for
  the duration of the push. Reversing that (push overrides empower) is a one-line swap; it is a
  cosmetic call with no correctness impact.
- **`EF_EMPOWERED` must stay `1<<18`.** The server patch flips exactly that bit. Renumber it and
  empower renders as a plain push on new clients.
- **No netcode change.** `eFlags` is already networked in `entityState_t` — the existing
  `EF_BODYPUSH` check reads `cent->currentState.eFlags`. Nothing new crosses the wire.
- Non-humanoid models (`cent->localAnimIndex > 1`) already bail out of the blur; unchanged.
- `cgs.empowerEffect` defaults to `0` when the key is absent (e.g. a non-JoF server), which is the
  current look — a safe fallback.

## Testing notes

1. `jp_empowerEffect 0` → `/amempower` → 8 orbs, identical to today.
2. `jp_empowerEffect 1` → 4 orbs: both hands + both forearms.
3. `jp_empowerEffect 2` → 2 orbs: hands only.
4. **Regression — push:** with the cvar at `1` or `2`, force-push someone who is **not** empowered →
   must still show the full 8. If push changed, the branches got crossed.
5. **Regression — old client:** connect an unpatched client to the patched server with the cvar at `1`
   or `2` and look at an empowered player → must show the full 8 (it can't see bit 18).
6. **Stacking:** on a patched client, empower a player *and* push them → exactly one effect (the
   trimmed empower set), never both overlaid.

Headless mode (`com_headless 1`) can't load a map, so verify in a real windowed client.
