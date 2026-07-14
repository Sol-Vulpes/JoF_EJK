# Hide force push/pull FX

The `cg_hideForcePushFX` cvar suppresses the force push/pull "cloud" effect that renders on
players. Admins handing out force powers with `/amempower` let people spam force push/pull, and
each hit spawns a cluster of puff sprites plus a refractive body pass. With several empowered
players going at once these stack into enough extra render passes that the renderer starts
dropping world surfaces — on tree-heavy maps, models flicker or stop drawing entirely.

The cvar is **archived and defaults to on**, so the effect is hidden out of the box.

- Commit: [`ce9128d`](https://github.com/Sol-Vulpes/JoF_EJK/commit/ce9128d88b53e31ef7eef2abd01a15dfdc76023e)
- Same doc as a rendered page: [`hide-force-push-fx.html`](hide-force-push-fx.html) (open it locally — GitHub shows HTML as source)

## How to use

```
/cg_hideForcePushFX 1    // default — push/pull cloud FX not drawn
/cg_hideForcePushFX 0    // restore the vanilla push/pull FX
```

It is a client-side display setting; it takes effect on the next frame, no reconnect needed, and
the value is saved to the config.

## What it hides

Force push and force pull both flag their *targets* with `EF_BODYPUSH` for 600 ms
(`w_force.c`, `push_list[x]->client->pushEffectTime`), and the caster's hand carries a refractive
blur. Two render paths draw from that, and the cvar gates both:

| Effect | Trigger | Renderer |
|---|---|---|
| Full-body puff "cloud" on everyone caught in a push/pull | `EF_BODYPUSH` eFlag | `CG_ForcePushBodyBlur` → `CG_ForcePushBlur` per push bone |
| Refractive blur on the caster's hand | `PW_DISINT_4` powerup, not gripping | `CG_ForcePushBlur( efOrg, cent )` |

Because the target flag is shared between push and pull, hiding it covers both.

## Where it lives

| File | What it does |
|---|---|
| `codemp/cgame/cg_xcvar.h` | Declares `cg_hideForcePushFX`, `CVAR_ARCHIVE`, default `"1"`. |
| `codemp/cgame/cg_players.c` | Two guards added in `CG_Player`: one on the `EF_BODYPUSH` body-blur block, one on the `else` branch that calls `CG_ForcePushBlur` for the hand. |

Both guards are added *alongside* the existing `forceFXVisible` and zoom checks, so the cvar only
ever removes work — it never changes any other condition.

## Known limits

- It hides the push/pull FX for **everyone**, including yourself and legitimate force users — this
  is a blunt display toggle, not an "only when /amempower is abused" filter. There is no reliable
  client-side signal that distinguishes an empowered player from any other force user, so the
  effect is keyed off the push/pull state itself.
- It does not touch the actual push/pull *gameplay* (knockback, damage, sounds) — only the visual
  sprite/refraction passes. The rendering-overload fix is the whole point; the mechanic is
  untouched.
- Other force-power visuals (rage electric body, protect/absorb shells, force sight aura, grip)
  are deliberately left alone — this cvar is scoped to the push/pull cloud only.

## Testing notes

Set `cg_hideForcePushFX 0`, have someone force-push a group of players near heavy foliage, and
watch the puff clouds and the world flicker; set it back to `1` and the clouds — and the
flickering — are gone. `sv_pure 0` is only relevant if you're loading loose test assets.

Headless mode (`com_headless 1`) can't load a map, so verify in a real windowed client.
