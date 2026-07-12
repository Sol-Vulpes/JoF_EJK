# Windows build scripts

Everything Windows runs through **`sol_one_script.bat`** in the repo root. The files in this folder are the pieces it calls; each one also works on its own if you want to run just that step.

## The one script

```batch
sol_one_script.bat              build + deploy + launch   (the usual one)
sol_one_script.bat build        build only (64-bit)
sol_one_script.bat build32      build only (32-bit)
sol_one_script.bat deploy       copy the existing build into GameData
sol_one_script.bat run          launch the game (no build, no deploy)
sol_one_script.bat headless     launch headless (rd-null renderer, console only)
sol_one_script.bat vulkan       launch with the Vulkan renderer
sol_one_script.bat vs           generate a Visual Studio solution in build\
sol_one_script.bat clean        delete build64temp\ and build32temp\
sol_one_script.bat help
```

Anything after the command is forwarded to the game:

```batch
sol_one_script.bat run +connect 1.2.3.4
sol_one_script.bat all +set cg_autoHeal 1
```

If the build or the deploy fails, the game is **not** launched, so you never end up testing a stale binary.

The game starts **detached** — it has no console attached, so you can keep using the terminal, or close it entirely, and the game keeps running.

## Builds are incremental

Only the sources you actually changed get recompiled. CMake is configured once, when the build directory is first created; after that it goes straight to an incremental MSBuild, the same as hitting Build in Visual Studio. A no-op rebuild takes about a second.

The one thing that always re-runs is packing `assets/` into the `.pk3` archives. The CMake zip step declares no input dependencies, so it would otherwise hand you a stale archive forever. It costs about 0.3s.

If a build ever goes strange, `sol_one_script.bat clean` deletes the build directories and the next build reconfigures from scratch.

## The pieces

| Script | What it does |
|---|---|
| `build.bat [x64\|Win32]` | Configures (first run only) and builds Release. Defaults to x64. |
| `copy_build_to_steam.bat` | Copies the engine, renderers, game modules and asset pk3s into GameData. Only copies files newer than what's already there. |
| `gamedata-path.bat` | Works out where your Jedi Academy `GameData` folder is. |
| `cmake-path.bat` | Works out which `cmake.exe` to use. |
| `detect-vs-generator.bat` | Picks the newest installed Visual Studio generator. |
| `start_headless.bat` | Launches the game headless (no GPU, console only). Optional argument: a server to auto-connect to. |
| `start_vulkan.bat` | Launches with the Vulkan renderer. Also un-sticks `cl_renderer` if a headless run left it on `rd-null`. |
| `CreateVisualStudio2022Projects.bat` | Generates a VS solution in `build\` for IDE work. Separate from the command-line build, so the two never clash. |

The three `*-path.bat` resolvers print the path they found on stdout and nothing else — all their chatter goes to stderr — so callers can capture them with `for /f`.

## Telling the scripts where things are

### Your Jedi Academy install

`gamedata-path.bat` looks, in order, at:

1. the `GAMEDATA` environment variable
2. `gamedata-path.txt` in this folder (first non-comment line)
3. the usual Steam install locations

Set it once and forget it:

```batch
setx GAMEDATA "D:\Games\Jedi Academy\GameData"
```

### Your CMake

`cmake-path.bat` looks, in order, at:

1. the `CMAKE_PATH` environment variable
2. `cmake-path.txt` in this folder (first non-comment line)
3. `cmake` on your `PATH`

Use this if CMake isn't on your `PATH`, or if you need a specific version. Our builds are tested with CMake 3.28; if a much newer CMake gives you generator or configure errors, install 3.28 and point `cmake-path.txt` at it:

```
# cmake-path.txt
C:\Program Files\CMake\bin\cmake.exe
```

Check it resolved correctly:

```batch
build-scripts\cmake-path.bat
build-scripts\gamedata-path.bat
```

Each prints the path it settled on, or an error explaining what to fix.

## Requirements

- **Visual Studio 2022** with the *Desktop development with C++* workload (2015 or later works; the newest installed one is picked automatically)
- **CMake 3.10+**
- A 64-bit **`SDL2.dll`** next to `eternaljk.x86_64.exe` in GameData, or the client won't start

## A note for anyone editing these

Don't echo an unquoted path inside a `( ... )` block. Batch expands `%VAR%` while it *parses* the block, so the `)` in `C:\Program Files (x86)\...` closes the block early and you get a baffling `\Steam\steamapps\... was unexpected at this time`. Use a `goto` label instead — that's why these scripts avoid blocks around paths.
