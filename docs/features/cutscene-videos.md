# Cutscene videos

Play a fullscreen MP4 during a multiplayer game, without dropping the player. Maps can show a
pre-rendered cutscene when someone walks into a trigger, and a server can show one to a single
player or to everybody.

Stock JKA only decodes RoQ, and its `cinematic` command switches the client into a state where it
stops talking to the server: anyone who watches a video that way times out. The single-player
camera commands exist in MP ICARUS but do nothing. This adds real video decoding to the client and
keeps it connected the whole time.

- Commit: [`5db76ef`](https://github.com/Sol-Vulpes/JoF_EJK/commit/5db76efcdb5dc7597047212c5e14a0e94976c931)
- Trello: none
- Same doc as a rendered page: [`cutscene-videos.html`](cutscene-videos.html) (open it locally — GitHub shows HTML as source)

> **Client only, so far.** The client side is done: decoding, drawing, skipping, and the protocol
> below. The server side (a `target_cutscene` map entity, freezing the player, an admin command)
> lives in the server's game module and is not part of this commit. Until the server implements it,
> the only way to start a video is the local `cutscene` console command.

## Making a video

Use **H.264 video + AAC audio in an MP4**. Windows decodes that out of the box on every
supported version, and so does FFmpeg. HEVC, VP9 and AV1 only work on Windows machines that happen to
have the codec installed (HEVC is a paid Store extension), so don't use them.

```
ffmpeg -i source.mov -vf "scale=1280:-2" -r 30 \
       -c:v libx264 -profile:v high -pix_fmt yuv420p -crf 20 \
       -c:a aac -b:a 160k -ac 2 \
       -movflags +faststart video/intro.mp4
```

- **Resolution:** 1280×720 is the sweet spot. 1920×1080 works but costs twice the upload per frame
  (see [limits](#known-limits)). Anything above 2048 in either dimension is refused.
- **Frame rate:** 24–30 fps. Higher just burns CPU.
- **Aspect:** anything. The video is letterboxed or pillarboxed to the player's window, never
  stretched.
- **Audio:** stereo is best. 5.1 is folded down to stereo, keeping the centre (dialogue) channel.
- **Size:** the whole file is loaded into memory while it plays, and it is downloaded along with the
  map. Keep cutscenes short and the bitrate sensible (`-crf 20`–`23`).

## Shipping it with a map

Put the file anywhere in the map's pk3; `video/` is the convention:

```
mymap.pk3
├── maps/mymap.bsp
└── video/mymap_intro.mp4
```

Players who auto-download the map get the video with it. Paths are relative to the game
directory like any other asset, and `.mp4` is assumed if no extension is given.

## Testing a video locally

```
/cutscene video/mymap_intro.mp4    play it now, on this client only
/cutscene stop                     end it
```

Loose (non-pk3) files need `sv_pure 0`. Space, Enter, Escape or a mouse click skips.

| Cvar | Meaning |
|---|---|
| `cl_videoVolume` | Video volume relative to `s_volume` (0–2, default 1). Archived. |
| `cl_video` | Read-only. `1` if this client can play videos, `0` if not. Sent in userinfo so the server can tell. |
| `cl_videoBackend` | Read-only. `mediafoundation`, `ffmpeg`, or `none`. |

## Triggering it from a map

*Needs the server side — see the note at the top.* The intended setup, which the server prompt
describes, is the usual trigger chain:

```
trigger_multiple  (brush)          target      intro_cs
target_cutscene   (point entity)   targetname  intro_cs
                                   video       video/mymap_intro.mp4
                                   spawnflags  2        (skippable)
                                   target      after_cs (fired when the video ends)
```

| Key | Meaning |
|---|---|
| `video` | Path of the video in the pk3. Required. |
| `spawnflags 1` | Play for **everyone** on the server, not just the player who triggered it. |
| `spawnflags 2` | The player may skip it. |
| `spawnflags 4` | Don't freeze the player (by default they are frozen and cannot be hurt while watching). |
| `spawnflags 8` | Play only once per player per map. |
| `timeout` | Seconds the server waits for the client to report the end before releasing the player anyway. Default 120. |
| `target` | Fired, with the player as activator, when their video ends, is skipped, or times out. |

Anything that fires targets can start a cutscene: buttons, doors, `target_delay`, `target_relay`,
ICARUS scripts.

## The protocol

For whoever writes the server side. All text commands, same style as `jof_dialogue`.

**Server → client** (server commands):

```
jof_video play <serial> "<path>" <flags>     flags: 1 = skippable
jof_video stop <serial>
```

`serial` is any number the server picks to recognise the reply. A new `play` replaces a video that
is still running.

**Client → server** (client command), sent exactly once per `play` the client receives:

```
videodone <serial> <reason>
```

| Reason | When |
|---|---|
| `end` | The video played to the end. |
| `skip` | The player skipped it. |
| `stopped` | The server sent `stop`, a newer `play` replaced it, or the player typed `cutscene stop`. |
| `error` | The file is missing, corrupt, or in a format this machine cannot decode. |
| `unsupported` | The client has no video decoder (e.g. Windows N without the Media Feature Pack). |

Clients advertise support in userinfo: `cl_video` is `1` when they can play videos. Stock and older
clients don't send the key at all and would never reply, so the server should not freeze them or
wait for them.

**No reply is sent** if the client's cgame shuts down mid-video (`vid_restart`, disconnect, map
change) — so the server always needs a timeout.

## macOS and Linux

Windows needs nothing: Media Foundation ships with it. On macOS and Linux the client uses the
system's FFmpeg, which is **not** bundled with the game:

- **Players** install FFmpeg (`brew install ffmpeg`, `sudo apt install ffmpeg`, …).
- **Building:** CMake looks for FFmpeg's development libraries through pkg-config
  (`libavformat-dev libavcodec-dev libswscale-dev libswresample-dev` on Debian/Ubuntu). If they are
  found, video support is compiled in and the client **needs** FFmpeg's libraries installed to
  start. If not, the client builds without video and reports `cl_video 0`. Turn it off explicitly with
  `-DUseFFmpegVideo=OFF`.

FFmpeg 4.4 through 9.x are supported.

## Where it lives

| File | What it does |
|---|---|
| `codemp/client/cl_video.cpp` | Playback: loading the file, the clock, audio scheduling, the power-of-two texture, the `cl_video*` cvars. |
| `codemp/client/cl_video_mf.cpp` | Windows decoder (Media Foundation, loaded at runtime). |
| `codemp/client/cl_video_ffmpeg.cpp` | macOS/Linux decoder (FFmpeg), compiled only with `USE_FFMPEG`. |
| `codemp/client/cl_video.h` | The decoder interface both implement. |
| `codemp/cgame/cg_video.c` | `jof_video`, `videodone`, the `cutscene` command, skipping, letterboxing, the skip hint. |
| `codemp/cgame/cg_public.h` | `trap->ext.Video_*`, appended to the import table, and `CGAME_EVENT_VIDEO`. |

How it fits together:

- The engine decodes, cgame draws. cgame calls `trap->ext.Video_Draw` at the end of each frame, so
  the video covers the world, HUD and chat. The menus and the console still draw on top.
- The video runs on a wall clock. Each client frame shows the newest decoded picture that is due and
  keeps about 0.2 s of sound queued ahead. Sound is scheduled against that clock, not against the
  mixer, so a muted or unfocused window can't make the decoder race ahead.
- Every renderer's `DrawStretchRaw`, including the prebuilt Vulkan one, only accepts power-of-two
  images. Rather than resample, each frame is copied into the corner of a black power-of-two texture
  and the quad is stretched so the padding lands off the video's rectangle, on the black background.
- cgame only touches `trap->ext.Video_*` when `cl_video` is set, because an older engine's import
  table ends before those entries.
- Media Foundation's DLLs are loaded with `LoadLibrary`, not linked, so the client still starts on
  Windows N editions; there `cl_video` is simply 0.

## Known limits

- **The server keeps running.** Other players, monsters and projectiles carry on while someone
  watches. Freezing and protecting the viewer is the server's job; without the server side, a
  local `cutscene` is purely visual.
- **Clients without this build see nothing.** Stock clients ignore `jof_video` and never reply.
- **Pre-rendered only.** There is no in-engine camera (SP-style ICARUS `camera` commands are still
  stubs in MP). A cutscene cannot show the player's own model or what is actually happening in the
  game.
- **Only the level music is paused.** Game sounds, and voice chat, carry on under the video. Voice
  chat shares the raw sound buffer and may briefly stutter the video's audio.
- **Upload cost.** A 1080p frame pads to a 2048×2048 texture (16 MB per frame); 720p pads to
  2048×1024. Fine on current hardware, but 720p is the safer choice.
- **No pause, no seeking, no subtitles.** Burn subtitles into the video if you need them.
- **`vid_restart` or a map change ends the video** without telling the server (see the protocol).
- **The macOS/Linux decoder is compile-checked, not play-tested.** It compiles against FFmpeg 4.4
  and 9.0 headers, but nobody has run it on a Mac or Linux box yet.

## Testing notes

- Tested on Windows 11 with both the OpenGL renderer and the Vulkan renderer: a 1920×1080
  H.264/AAC stereo clip and a 1000×750 (4:3, odd height) H.264/AAC 5.1 clip. Both played in sync,
  letterboxed/pillarboxed correctly, ended on their own, and the level music came back.
- `cutscene video/missing` reports the missing file and plays nothing.
- Not yet tested: the server path (`jof_video` / `videodone`), because no server sends it yet, and
  skipping by key, which needs someone at the keyboard.
- Headless mode (`com_headless 1`) cannot load a map, so it cannot test this. Use a real windowed
  client.
