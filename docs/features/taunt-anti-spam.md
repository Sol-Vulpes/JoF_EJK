# Taunt anti-spam

Model voice lines carry no rate limit of their own. A player holding a taunt bind can fire the
same line as fast as the server sends the event, and a player bobbing at a water surface gasps on
every frame their head clears it.

`cg_tauntAntiSpam` throttles those lines per player, on your screen only. The taunt family —
taunt, flourish, gloat, bow, meditate — shares one timer, and **a player's next taunt is held back
until the last one has finished playing**, however long that model's line is.

The cvar is **archived and defaults to on**.

- Commit: [`86b0af4`](https://github.com/Sol-Vulpes/JoF_EJK/commit/86b0af485d80b569176454164228ecbb998e69fe) — taunts wait for the last line to finish
- Issue: https://github.com/JediofFreedom/JoF_EJK/issues/311
- Same doc as a rendered page: [`taunt-anti-spam.html`](taunt-anti-spam.html) (open it locally — GitHub shows HTML as source)

## How to use

```
/cg_tauntAntiSpam 1    // default — throttle repeated voice lines per player
/cg_tauntAntiSpam 0    // every voice line plays
```

Client-side, takes effect immediately, saved to config. Also in **Setup → Sound → Taunt
Anti-Spam**.

| Line | How long it holds that player's next one back |
|---|---|
| Taunt, flourish, gloat (and bow / meditate lines a server sends) | The length of the line that just played |
| Gasp | 2 s |
| Jump, roll, landing grunts | 1 s each, on separate timers |

Only the audio is suppressed — the taunt animation still plays.

## How the taunt hold works

Before, the taunt family used a fixed 2.5 s timer. Lines vary a lot by model and by variant
(`taunt1`…`taunt3`, `gloat1`…`gloat3`, …), so a short line still made the player wait out the full
2.5 s, while a line longer than 2.5 s got cut off mid-sentence: the next taunt lands on the same
player's `CHAN_VOICE`, and the mixer always replaces a sound from the same entity on that channel.

Now, when a taunt-family line gets through, cgame asks the engine for the length of that exact
sample and holds the player's taunt timer for that long. It's the same length the mixer uses to
decide when the channel is done, so the next taunt is allowed the moment the last one stops.

The engine exposes the length through a new cgame import, `trap->ext.S_GetSampleLengthMs`. It's
appended to the end of the `ext` block, and an older engine's import table simply ends before
that slot — reading it there would be reading past the table. So the engine also registers a
read-only cvar, `cl_soundLength 1`, and cgame calls the import only when that cvar is set. This is
the same guard `cl_video` provides for the video imports.

If the length can't be read — an older engine, no sound system (`s_initsound 0`), or a missing
file that registered as the placeholder sound — the taunt hold falls back to the old 2.5 s.

## Where it lives

| File | What it does |
|---|---|
| `codemp/cgame/cg_event.c` | `CG_VoiceLineThrottled` keeps a start time and a hold time per player per line class. `CG_SoundLengthMs` checks `cl_soundLength` before calling the import. `CG_ClassifyVoiceLine` maps sound names sent through generic sound events onto the same classes. The `EV_TAUNT`, `EV_TAUNT1`–`3`, `EV_GENERAL_SOUND` and `EV_ENTITY_SOUND` paths resolve the sample *before* the throttle check so it knows what's about to play. |
| `codemp/cgame/cg_public.h` | `ext.S_GetSampleLengthMs`, at the end of the import table. |
| `codemp/client/cl_cgameapi.cpp` | Fills the import and registers `cl_soundLength` (`CVAR_ROM`) in `CL_BindCGame`. |
| `codemp/client/snd_dma.cpp` | `S_GetSampleLengthMs` — wraps stock `S_GetSampleLengthInMilliSeconds`, returning 0 instead of its 512-second stand-in when there's no sound system, and 0 for the placeholder sound. |
| `codemp/cgame/cg_xcvar.h`, `codemp/ui/ui_xdocs.h` | The cvar and its in-game help text. |

## Known limits

- **Taunts only.** Gasp, jump, roll and landing grunts keep their fixed timers. Those were already
  sized to roughly the stock line, and they fire during ordinary movement, so they weren't part of
  this change.
- **The hold counts game time, the line plays in real time.** At normal speed the two match. With a
  server `timescale` below 1, the hold outlasts the line; above 1, it ends early and the next taunt
  can cut the last one off — the same as before this change.
- **A line cut short still holds for its full length.** If a player dies or takes a pain sound
  mid-taunt, `CHAN_VOICE` is taken over, but their taunt timer keeps running until the original
  line would have ended. Taunts are short, so this was left alone.
- **Needs the matching engine.** A new cgame on an old engine falls back to 2.5 s. The cgame and
  engine must come from the same branch: `beta` has no video imports, so on `beta` this import sits
  at a different position than on `alpha` / `🦊Sol`.
- **Players only.** NPCs (entity numbers above `MAX_CLIENTS`) aren't throttled — they have their own
  cooldowns.
- **Your view only.** Other players without this client still hear the spam.

## Testing notes

Needs both the new engine and the new cgame. Two clients, or a client plus a bot on a listen server:

1. `cl_soundLength` in the console — should print `1`.
2. Hammer a taunt bind (`taunt`, `flourish`, `gloat`) on the other client. Each line should play to
   the end, and the next one should start right after — never cut off and no long dead gap.
3. Try a model with long taunt lines — they should no longer be cut off at 2.5 s.
4. `cg_tauntAntiSpam 0` — lines cut each other off again.
5. With `sv_cheats 1`, `s_show 1` prints `...overrides <sound>` whenever a sound replaces another on
   the same channel (default mixer only, not `s_UseOpenAL 1`). With the cvar on, taunts should never
   show up there.
