# Hide fire FX

The `cg_hideFireFX` cvar suppresses map-placed fire/flame effects. Some maps pack in dozens of
fire `fx_runner`s — torches, campfires, braziers, lava — which add visual clutter and rendering
cost. The only prior off switch was `cg_noFX`, which kills *every* effect and speaker on the map;
this is the scalpel for fire alone.

The cvar is **archived and defaults to off**, so maps keep their intended look unless you opt in.

- Commit: [`1c6b89a`](https://github.com/Sol-Vulpes/JoF_EJK/commit/1c6b89aac8352cfb75d4470dd1519aee81b54006)
- Same doc as a rendered page: [`hide-fire-fx.html`](hide-fire-fx.html) (open it locally — GitHub shows HTML as source)

## How to use

```
/cg_hideFireFX 0    // default — fire effects play normally
/cg_hideFireFX 1    // hide map fire/flame effects
```

Client-side display setting: takes effect on the next frame, no reconnect, saved to config. It's
listed in `helpUsSol` alongside `cg_hideForcePushFX`.

## What it hides

Map ambient effects are `fx_runner` entities that arrive on the client as `ET_FX` and play through
`CG_FX` (`cg_ents.c`). Each one names an effect file via the `CS_EFFECTS` configstring. When the
cvar is on, `CG_FX` looks up that name and returns early if it contains **"fire"** or **"flame"**
(case-insensitive) — so `env/fire`, `env/small_fire`, `env/fire_column`, a custom
`maps/foo/torch_flame`, etc. are all skipped.

Because the flame's looping sound is usually bundled *inside* the same `.efx`, skipping the effect
drops that sound with it.

## Where it lives

| File | What it does |
|---|---|
| `codemp/cgame/cg_xcvar.h` | Declares `cg_hideFireFX`, `CVAR_ARCHIVE`, default `"0"`. |
| `codemp/cgame/cg_ents.c` | Guard at the top of `CG_FX`: resolves the `CS_EFFECTS` name and returns early on a `fire`/`flame` substring match. |
| `codemp/cgame/cg_consolecmds.c` | Listed in `helpUsSol`. |

The guard sits after the "fx not active" check and before the effect is registered/played, so it
costs a cheap configstring pointer lookup and a substring test only while the cvar is on.

## Known limits

- **Name-based heuristic.** It matches on the effect *name* containing `fire`/`flame`. An effect
  that is visually fire but named something else (`env/torch`, `volcano/lava_spurt`) slips through;
  a non-fire effect that happens to contain the substring (a hypothetical `firework`) would be
  caught. In practice JKA/community maps name their fire effects with the word in them, so this
  covers the overwhelming majority. If a specific map slips through, the name can be added.
- **Effects only, not shaders or speakers.** Fire painted with an animated *shader* (a flame
  texture on a surface) is part of the world geometry, not an `fx_runner`, and is untouched — as is
  a fire *sound* coming from a separate `target_speaker` rather than the effect itself. For those,
  the blunt `cg_noFX` (`2` also silences speakers) is still the only lever.
- **Client-side and cosmetic.** It changes only what you see/hear locally; gameplay (fire that
  actually damages, e.g. a `trigger_hurt`) is unaffected.

## Testing notes

Load a fire-heavy map (or drop an `fx_runner` pointing at `env/fire` with `sv_pure 0`), toggle
`cg_hideFireFX 1`, and the flames — and their crackle — should vanish on the next frame; back to
`0` restores them.

Headless mode (`com_headless 1`) can't load a map, so verify in a real windowed client.
