# No fake tells

In JKA, private messages (tells) are the only chat drawn in pink (`^6`). That makes pink a trust
signal — and players abuse it: they type `^6` into public or team chat so their line looks like a
private message, usually to impersonate an admin or bait someone into a reply.

The `cg_noFakeTells` cvar repaints that pink back into the channel's normal color. Real tells are
never touched.

The cvar is **archived and defaults to on**.

- Commit: [`102eb33`](https://github.com/Sol-Vulpes/JoF_EJK/commit/102eb335a2d75a1fea363b386d0d6d98c9565900)
- Same doc as a rendered page: [`no-fake-tells.html`](no-fake-tells.html) (open it locally — GitHub shows HTML as source)

## How to use

```
/cg_noFakeTells 1    // default — pink in public/team chat is recolored
/cg_noFakeTells 0    // show chat exactly as sent
```

Client-side, takes effect on the next message, saved to config. Listed in `helpUsSol`.

| Channel | What a `^6` in the message becomes |
|---|---|
| Public chat | `^2` green |
| Team chat (with or without location) | `^5` cyan |
| Real tell | untouched |

## How it tells a real DM from a fake one

The server builds every chat line as *sender prefix + color + message*. For a tell, the prefix is
`\x19[name^7\x19]\x19: ` and the color is `^6`. For public chat it's `name^7\x19: ` then `^2`.

`\x19` is an invisible marker the client strips before drawing. Player names are cleaned of control
characters on the server, and it sits outside the message, so **no player can forge a prefix that
starts with `\x19[`**. A name like `[Admin` still produces `[Admin^7\x19: ` — no leading marker, so
it's treated as public chat.

For anything that isn't a tell, every `^6` after the server's `\x19: ` separator is rewritten to the
channel color. Only the message body is scanned, never the sender's name, so a pink name stays
pink.

## Where it lives

| File | What it does |
|---|---|
| `codemp/cgame/cg_xcvar.h` | Declares `cg_noFakeTells`, `CVAR_ARCHIVE`, default `"1"`. |
| `codemp/cgame/cg_servercmds.c` | `CG_RecolorFakeTell` / `CG_RecolorFakeTellInLine`, called from `CG_Chat_f` for `chat`, `tchat`, `lchat` and `ltchat` — *before* `CG_RemoveChatEscapeChar`, which deletes the `\x19` markers the check relies on. |
| `codemp/cgame/cg_consolecmds.c` | Listed in `helpUsSol` (Sol branch only). |

Because the rewrite happens before the text is handed on, the chat box, the WoW chat window, the
console and the chat log all see the corrected color. It also runs before the
`cg_chatSounds 2` check, so a faked `]: ^6` no longer plays the private-message sound either.

## Known limits

- **Only `^6` is caught.** JKA's color codes are `^0`–`^9`, and `^6` is the only magenta one, so
  this covers the exact color the game uses for tells. Nothing stops someone using another color —
  but no other color reads as a DM.
- **All pink in the body goes, not just the leading one.** Someone who uses `^6` innocently in a
  rainbow message will see that bit turn green. That's deliberate: faking a DM mid-line
  (`hey ^7[Admin^7]: ^6...`) is just as easy as at the start, and a partial rule would miss it.
- **Relies on the standard server chat format.** Base JKA builds chat lines this way, and so do
  the mods derived from its `G_Say` (including this repo's own game module). A server mod that sends chat without the `\x19: ` separator is left alone — the filter never
  guesses where the message starts.
- **Your view only.** Other players without this client still see the fake pink.

## Testing notes

Two clients (or a client plus a second one on the same listen server):

1. `say ^6hello` — should arrive green.
2. `say_team ^6hello` — should arrive cyan.
3. `tell <id> hello` — should still arrive pink, in brackets.
4. `cg_noFakeTells 0` then repeat 1 — pink again.
