# Mega prompt: Sol's Archive companion app

Copy everything below the line into a fresh Claude Code session in an empty
project folder.

---

# Project: Sol's Archive — Jedi Academy console companion app

Build a cross-platform desktop app (Windows, macOS, Linux — all three must
work natively) called **Sol's Archive**. It connects to my Jedi Academy
client (a JoF_EJK / OpenJK fork) over a local TCP socket, shows the live game
console with colors, lets me send commands and chat from the app, lets me
save funny chat lines as "quotes", and runs an in-game chat bot that answers
`!so` commands by posting saved quotes back into the game chat.

## Tech stack (decided, don't change without strong reason)

- **Rust** (stable), **eframe/egui** for the GUI — single static binary per
  OS, no runtime dependencies, no platform-specific code.
- `serde` + `serde_json` for persistence, `directories` for the config dir,
  `rand` for random quote selection. Std `TcpStream` + threads + channels are
  enough for networking; no async runtime needed.
- Architecture: a background network thread owns the `TcpStream` (read +
  write; writes arrive over an `mpsc` channel), parses bytes into lines, and
  sends parsed lines to the UI thread over a channel. `egui` repaints on new
  data via `ctx.request_repaint()`. The network thread reconnects
  automatically with backoff (e.g. 1s, 2s, 5s, then every 5s) when the game
  isn't running.
- Include a **mock server** as `examples/mock_game.rs`: listens on the same
  port, requires the password line, emits fake console output including
  colored chat lines every few seconds, and prints any commands it receives.
  This lets us develop and test everything without the game running.

## The game-side protocol (already implemented in the game, do not redesign)

The game client has a cvar `cl_consoleSocket <port>` which opens a TCP
listener on `127.0.0.1:<port>` (loopback only), and an optional
`cl_consoleSocketPassword <pw>`. Protocol — raw text stream, no framing:

1. Connect to `127.0.0.1:<port>`.
2. If a password is configured, send it as the **first** line (`\n`
   terminated). The game replies `cl_consoleSocket: authenticated\n` and the
   stream starts. A wrong/missing password means immediate disconnect.
3. **Inbound:** every console line the game prints is streamed, terminated by
   `\n`. Lines contain Quake color codes (see below) and arbitrary bytes —
   treat them as bytes, convert to text with lossy UTF-8 for display.
4. **Outbound:** every `\n`-terminated line the app sends is executed as a
   console command in the game (e.g. `say hello` chats). `\r` is ignored.
5. Limits: max 4 simultaneous clients; outbound command lines max 1023 chars;
   if the app stops reading, the game silently drops output (it never
   blocks); over 256 KB of unread backlog the game disconnects the app.

### Color codes

`^` followed by a digit `0`–`9` switches color until the next code. Render
them in the console view; never display the `^N` itself. Exact game colors:

| Code | Color | RGB |
|---|---|---|
| `^0` | black | (0, 0, 0) |
| `^1` | red | (255, 0, 0) |
| `^2` | green | (0, 255, 0) |
| `^3` | yellow | (255, 255, 0) |
| `^4` | blue | (0, 0, 255) |
| `^5` | cyan | (0, 255, 255) |
| `^6` | magenta | (255, 0, 255) |
| `^7` | white (default) | (255, 255, 255) |
| `^8` | orange | (255, 128, 0) |
| `^9` | grey | (128, 128, 128) |

The console view should use a dark background so white/yellow text reads
well, and render `^0` black as a light grey instead so it's not invisible.
A "clean" version of any string = the string with all `^N` sequences removed.

### Recognizing chat lines

Chat lines (and only chat lines) contain the byte `0x19` before the colon
separating the speaker from the message:

```
PlayerName^7<0x19>: message text
```

Parse: split at the **first** `0x19`; speaker = everything before it
(color-stripped; team chat wraps the name in parentheses), message =
everything after the following `": "`. The message itself may contain color
codes. Detect chat on the raw bytes *before* UTF-8 conversion.

## App features

### 1. Console view

- Scrollable, monospace, color-rendered view of every console line.
- Auto-scroll to bottom, but pause auto-scroll while the user scrolls up
  (resume via a "jump to bottom" button or scrolling back down).
- A filter toggle/tabs: **All** output vs **Chat only** (0x19 lines).
- A text filter box to live-search visible lines.
- Keep a generous scrollback (e.g. 10,000 lines, oldest dropped).

### 2. Input bar

- One input box at the bottom with two modes, toggled by a small button or
  Tab: **Chat mode** (default — sends `say <text>`) and **Command mode**
  (sends the line verbatim as a console command, e.g. `amsit`).
- Up/Down arrow command history. Enter to send.
- Show connection state next to it (connected / connecting / authenticating /
  disconnected) with a colored dot.

### 3. Saving quotes (the core feature)

- Each line in the console view is **click-selectable** (click = select,
  Ctrl+click = add to selection, Shift+click = range). Selected lines get a
  highlighted background.
- A "save quote" action (button + keyboard shortcut, e.g. Ctrl+S) saves the
  selected line(s) as one quote. Double-clicking a chat line saves it
  instantly as a single-line quote.
- A **Quotes panel** (side panel or second tab): list of saved quotes showing
  id, speaker, clean text, and saved date; with text search, copy to
  clipboard, and delete. Color-render the quote text here too.
- Storage: `quotes.json` in the app's config directory
  (`directories::ProjectDirs`, e.g. qualifier "", org "Sol", app
  "SolsArchive"). Each quote: `id` (sequential integer, never reused),
  `saved_at` (ISO timestamp), `speaker` (clean), `message_clean`,
  `message_raw` (with color codes), `raw_line` (full original line(s)).
  Save the file on every change.

### 4. The in-game `!so` bot

While connected, watch incoming **chat** messages for bot commands. When my
app sees one, it answers by sending a `say` command back through the socket,
so the reply appears in game chat for everyone.

Commands (a chat message whose clean text starts with `!so`):

- `!so <number>` → look up quote by id. Reply: `so - id <id>: <message_clean>`
- `!so <text>` → case-insensitive substring search over `message_clean` and
  `speaker`; if multiple match, pick one at random. Reply same format.
- `!so random` (or `!so r`) → random quote, same reply format.
- `!so count` → reply `so - <N> quotes archived`
- `!so help` → reply `so - commands: !so <id> | !so <search> | !so random | !so count`
- No match → reply `so - nothing found for: <query>`
- Bare `!so` or unparseable → reply the help line.

Bot safety rules (all required):

- **Loop prevention:** never trigger on a chat message whose clean text
  starts with `so - ` (that's our own reply echoing back), and never trigger
  on messages we ourselves just sent.
- **Sanitization:** before embedding any quote text in a `say` line, strip
  `\n`, `\r`, `;` and the `0x19` byte (a `;` would let a quote inject a
  second console command), collapse whitespace, and truncate the final say
  text to 140 characters (the game caps chat length at 150).
- **Rate limiting:** minimum 1.5s between bot replies (queue them), and a
  global cooldown of ~2s per trigger so chat spam of `!so` can't flood; drop
  triggers while on cooldown.
- A master **bot on/off toggle** in the UI, default ON, state persisted.
- Log every bot trigger and reply to a small activity area or status line.

### 5. Settings

- Settings UI (small dialog or collapsible section): port (default 29071),
  password, bot on/off, reply prefix (default `so - `), trigger prefix
  (default `!so`). Persist to `config.json` next to `quotes.json`. Connect on
  launch with saved settings.

## Quality bar

- `cargo build` must succeed on all three platforms — zero `cfg(target_os)`
  platform-specific code, zero non-Rust dependencies.
- No `unwrap()`/`expect()` on I/O or network paths; failures surface in the
  UI status, the app never panics because the game closed.
- Handle partial lines (TCP is a byte stream — buffer until `\n`).
- Start by setting up the project, the mock server example, and the network
  thread; get the colored console view working against the mock server, then
  quotes, then the bot. Write a README covering build/run per platform and a
  user-facing feature guide.

## How I test

I run the game with `cl_consoleSocketPassword <pw>` and `cl_consoleSocket
29071` set, or I run `cargo run --example mock_game` and point the app at it.
