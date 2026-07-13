# Playermodel blacklist

Turn specific playermodels into Kyle. Some models are only meant for NPCs — they break
hitboxes, look absurd on a player, or were never built for MP. Before this, the only way to
keep players off a model was to not ship it, which also denied it to NPCs.

A blacklisted model falls back to `kyle/default` on every player wearing it, and disappears
from the model picker in the menus. **NPCs keep using it normally.**

- Commit: [`41eac85`](https://github.com/Sol-Vulpes/JoF_EJK/commit/41eac85584f8dd3cbc6dd840f27cc83b8fee9a24)
- Trello: [Create a blacklist for Playermodels](https://trello.com/c/rW73mB5L/168-create-a-blacklist-for-playermodels-let-us-turn-specific-models-of-our-choice-into-kyle-default-model)

## The three ways to blacklist a model

A model is off limits to players if **any** of these says so.

### 1. A blacklist file in a pk3 — `ext_data/modelblacklist/*.txt`

The one to use for shipping a model pack. One model name per line, `//` comments allowed:

```
// zz_jof_models_blacklist.txt
rancor
wampa
sand_creature   // huge, breaks hitboxes
```

The filesystem merges this **directory** across every loaded pk3, so a pk3 can carry both its
models and its own blacklist file, and it stacks with whatever else is installed. No repacking
of anyone else's pk3. This is the same mechanism the game already uses for
`ext_data/sabers/*.sab`.

> **Name the file after your pack.** Directory *listings* merge across pk3s, but a *file* at the
> same path does not — if two pk3s both contain `ext_data/modelblacklist/blacklist.txt`, only the
> higher-priority pk3's copy is read and the other is silently ignored. `jof_npconly.txt`,
> `mypack_blacklist.txt`, etc. all coexist fine.

### 2. A model declaring itself NPC-only — `models/players/<model>/settings.txt`

For when you control the model's pk3 and want the model itself to carry the rule:

```
notInMP 1
```

### 3. `cg_modelBlacklist` — the player's personal list

A local, archived cvar. Space or comma separated, for a player who just doesn't want to see
certain models:

```
/cg_modelBlacklist "rancor wampa"
```

Editing it takes effect immediately — it re-runs every player's clientinfo through the existing
`CG_ForceModelChange` callback, no reconnect needed.

## Why NPCs are unaffected

Nothing special was needed for this; it falls out of the engine's structure. Players and NPCs
take entirely separate model paths:

- **Players**: userinfo `model` → `ClientUserinfoChanged` → the `CS_PLAYERS+n` configstring →
  every client's `CG_NewClientInfo` → `CG_RegisterClientModelname`.
- **NPCs**: `ET_NPC` entities get their ghoul2 instance built server-side and never touch
  `clientinfo` or that configstring (`cg_players.c`, `cent->npcClient`).

So gating the player clientinfo path leaves NPCs untouched by construction.

Siege class `forcedModel` is deliberately also left alone — that's a class definition, not a
player's choice.

## Where it lives

| File | What it does |
|---|---|
| `codemp/game/bg_misc.c` | `BG_LoadModelBlacklist()` merges/parses the `ext_data` files; `BG_ModelIsNPCOnly()` answers the question (blacklist files first, then the model's own `settings.txt`); `BG_ModelInList()` matches a name against a list string. Compiled into cgame *and* ui, which is where both callers are. |
| `codemp/cgame/cg_players.c` | `CG_ModelIsBlacklisted()`, applied in `CG_NewClientInfo`. |
| `codemp/cgame/cg_xcvar.h` | `cg_modelBlacklist`, wired to `CG_ForceModelChange`. |
| `codemp/ui/ui_main.c` | `UI_BuildPlayerModel_List` skips blacklisted models. |

The enforcement point is `CG_NewClientInfo`, right after the model name is resolved from the
clientinfo configstring and *before* anything downstream reads it — so the swap carries into the
model icon, the skin and the scoreboard, not just the rendered mesh. It also catches the model
however it arrived: the player's own choice, `cg_forceModel`, or `cg_forceAllyModel` /
`cg_forceEnemyModel`.

Results are cached (the blacklist files, and each model's `settings.txt` answer), so repeated
clientinfo updates don't re-hit the filesystem. Limits are 512 blacklisted models and 16 KB per
blacklist file; exceeding either prints a console warning rather than truncating silently.

The list reloads lazily. In cgame that's effectively per-map, since the VM is recreated on each
map load, so a newly downloaded pk3 is picked up on the next map. The UI VM lives longer, so
`UI_BuildPlayerModel_List` forces a reload every time it rebuilds — the picker reflects pk3s
installed mid-session.

## Known limits

Enforcement is **client side**. That means:

- A player running an unmodified client still sees the model. This is cosmetic policy for a
  community where everyone runs our cgame, not an anti-cheat.
- A client that doesn't have the model's pk3 can't read its `settings.txt` — but such a client
  already falls back to Kyle anyway, via the existing missing-model path.

Making it authoritative would mean rewriting the `model` key server-side in
`ClientUserinfoChanged` (`codemp/game/g_client.c`), which would also require the *server* to have
the model's pk3 installed in order to read its blacklist. Deliberately not done.

## Testing notes

`clientlistInfo` in the console prints each client's resolved model name — the quickest way to
confirm a swap actually happened.

Loose (non-pk3) test files need `sv_pure 0`, otherwise the filesystem won't read them.

Headless mode (`com_headless 1`) is **not** usable for testing this: the client crashes on cgame
init when a map loads. That's pre-existing and unrelated — stock, unmodified code crashes there
identically.
