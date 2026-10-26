# Saber anti-spam

A bind or script can turn a saber on and off every server frame, or loop weapon switches onto it
(`weapon 1; weapon 3` on a key), and every ignition and shutdown plays. Everyone nearby hears a
wall of saber sounds.

`cg_saberAntiSpam` gives each player a small budget, on your screen only: **four toggles in a row
play as usual, then one toggle sounds every half second** for as long as the spam goes on, and the
budget refills at that pace. Quick toggling by hand is never cut.

The cvar is **archived and defaults to on**.

- Commit: [`5c2c7c0`](https://github.com/Sol-Vulpes/JoF_EJK/commit/5c2c7c0799b22a589757c651bbf44a54148a9df5) — saber on/off spam held to a burst, then one toggle every half second
- Replaces the March fix, [PR #74](https://github.com/JediofFreedom/JoF_EJK/pull/74) (closed)
- Same doc as a rendered page: [`saber-anti-spam.html`](saber-anti-spam.html) (open it locally — GitHub shows HTML as source)

## How to use

```
/cg_saberAntiSpam 1    // default — hold each player's saber toggle spam
/cg_saberAntiSpam 0    // every ignition and shutdown plays
```

Client-side, takes effect immediately, saved to config. Also in **Setup → Sound → Saber
Anti-Spam** (main menu and in game).

| Toggles from one player | What you hear |
|---|---|
| Up to 4 back to back | Every one |
| Kept up past that | One every 500 ms |
| After 2 s of quiet | The full 4 again |

Only the audio is suppressed — the saber still lights and goes out as the server says.

## Why not a debounce per sound

PR #74 kept one 800 ms timer per sound. Review found two problems with it:

- **Normal quick toggling lost sounds.** On, off, on within 800 ms skipped the second ignition.
- **Duals lost a hilt.** Each hilt's sound had its own timer, so one hilt of a toggle could play
  while the other was held back.

The budget fixes both. A burst plays untouched, and every sound of one toggle — both hilts of a
dual or a staff — arrives in the same frame and gets the same answer, so a toggle is heard whole or
not at all.

## Where it lives

| File | What it does |
|---|---|
| `codemp/cgame/cg_event.c` | `CG_SaberToggleThrottled` keeps a GCRA budget per player (`SABER_TOGGLE_BURST` 4, `SABER_TOGGLE_INTERVAL` 500 ms) and repeats its answer for the rest of a frame. `CG_SaberToggleSoundThrottled` finds the owner of a `G_Sound` toggle — the nearest player within 64 units whose hilt (or, for a forced hilt, the server's) plays that sound. Checked in `EV_GENERAL_SOUND` and `EV_SABER_UNHOLSTER`. |
| `codemp/cgame/cg_players.c` | The weapon switch path for other players (`CG_Player`), and the JA+ staff put-away shutdown. |
| `codemp/cgame/cg_weapons.c` | The weapon switch path for the local view (`CG_CheckPlayerG2Weapons`). |
| `codemp/cgame/cg_xcvar.h`, `codemp/ui/ui_xcvar.h`, `codemp/ui/ui_xdocs.h` | The cvar and its in-game help text. |
| `assets/japro/ui/jamp/setup.menu`, `ingame_setup.menu` | The Saber Anti-Spam row under Taunt Anti-Spam. On `🦊Sol` the in-game Text, Voice and Set Language controls move down a row for it. |

## Known limits

- **The numbers are fixed.** Four and 500 ms, no cvar for either. Easy to change in `cg_event.c` if
  testing says otherwise.
- **A toggle's sounds must land in one frame.** They do — both hilts come in the same snapshot, or
  the same weapon switch. If a lagging client takes several snapshots in one frame, those toggles
  share one answer, so a little more gets through, never less.
- **Owner by position.** `G_Sound` names no owner. Two players standing within a few units with the
  same hilt can be confused, and the cap then lands on the nearer one.
- **Players only.** NPCs aren't capped — no one binds their toggles.
- **Your view only.** Other players without this client still hear the spam.

## Testing notes

Two clients, or a client plus a second instance on a listen server:

1. On the other client, with the saber out, mash the saber key (pressing it again toggles the
   blade) next to you: four sounds, then about two a second.
2. Toggle normally — on, off, on, a second or so apart: every sound plays.
3. A `weapon 1; weapon 3` loop: capped the same way.
4. Dual sabers and a staff: both hilts sound together on every toggle that plays.
5. `cg_saberAntiSpam 0` — the spam is back.
