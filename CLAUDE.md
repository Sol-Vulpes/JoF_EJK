# JoF EternalJK — working agreements

This file lives on the `🦊Sol` branch only. It is never part of a feature commit, so it never
travels to `alpha` or `beta`.

## GitHub account

**Push only as `Sol-Vulpes`.** There is a second GitHub account on this machine — never use it here.
`gh` is already authenticated as `Sol-Vulpes`; if it isn't, stop and say so rather than pushing with
whatever account happens to be active.

## Remotes

| Remote | Repo | Role |
|---|---|---|
| `origin` | `Sol-Vulpes/JoF_EJK` | Sol's fork. `🦊Sol` and PR branches live here. |
| `upstream` | `JediofFreedom/JoF_EJK` | The project. `alpha` and `beta` live here. |

Other remotes (`daggolin`, `github-desktop-lumayaa`) are read-only references. Never push to them.

## How a change ships

Work happens on `🦊Sol`. From there a change reaches the project in two directions, and **both carry
the code commit only — never the docs commit.**

1. **`🦊Sol`** (`origin`) — the feature commit, then the docs commit as a *separate* commit. Push.
2. **`alpha`** (`upstream`) — rebase local `alpha` onto `upstream/alpha` first (it is usually behind),
   cherry-pick the feature commit, push straight to `upstream alpha`.
   Local `alpha` sometimes carries earlier un-pushed cherry-picks; pushing publishes those too, so
   say what is about to go out before pushing.
3. **`beta`** (`upstream`, via PR) — create a clean `🦊<feature>` branch off `upstream/beta`,
   cherry-pick the feature commit, push to `origin`, open the PR into `JediofFreedom:beta`.
   This matches how earlier work landed (e.g. PR #118 from `Sol-Vulpes/🦊autodownload_jof`).

**Build every branch you push.** `alpha`, `beta` and `🦊Sol` have genuinely different trees — a
cherry-pick that applies cleanly can still fail to compile. Note that `sol_one_script.bat` only
exists on `🦊Sol`; on the other branches build the VMs directly:

```
cmake --build build64temp --config Release --target cgamex86_64 uix86_64 jampgamex86_64
```

## Commit messages

Explain *why the change exists* — the problem in the game, not a restatement of the diff. If a Trello
card is given, put the raw URL on its own line at the end. Sign off with the `Co-Authored-By` trailer.

## Docs

Every non-trivial feature gets a doc in `docs/features/`, **on `🦊Sol` only**, as a separate commit:

- `<feature>.md` — what github.com actually renders when browsing the repo.
- `<feature>.html` — the same content as a standalone, self-contained page (no CDN, no build step;
  light and dark themes). GitHub shows HTML as source, so the `.md` links across to it.

Both open with the commit link (permalink to the feature commit on `Sol-Vulpes/JoF_EJK`) and the
Trello card, then cover: what it does, how to use it, where it lives in the code, and the known
limits — including anything deliberately *not* done, and why.

## Testing

`clientlistInfo` in the console prints each client's resolved model/skin — handy for confirming
client-side changes actually took effect.

Loose (non-pk3) test files need `sv_pure 0`.

Headless mode (`com_headless 1`) cannot load a map: the client crashes on cgame init. This is
pre-existing — stock code crashes identically — so don't chase it, and don't use headless to verify
anything that needs a map loaded. Verify in a real windowed client instead.
