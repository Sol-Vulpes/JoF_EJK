# Olol Addon

This is a test addon for the JoF_EJK multiplayer game client addon system.

## Purpose

This addon demonstrates how to create addons that can add console commands to the game client. It adds a single command `test_olol` that prints "test success" to the console.

## Building

1. Make sure you have CMake installed
2. Create a build directory: `mkdir build && cd build`
3. Run CMake: `cmake ..`
4. Build the project: `cmake --build . --config Release`

The resulting `Olol.dll` file should be placed in the `addons/` directory of your game installation.

## Usage

Once the addon is loaded (automatically on client startup), you can use the command:

```
test_olol
```

This will print "test success" to the console.

## API

Addons use the addon API defined in `codemp/client/cl_addonapi.h`. The main functions are:

- `GetAddonAPI()` - Entry point that returns the addon interface
- `Init()` - Called when the addon is loaded
- `Shutdown()` - Called when the addon is unloaded

Addons can:
- Add console commands via `Cmd_AddCommand()`
- Access cvars via `Cvar_*` functions
- Use file system functions via `FS_*` functions
- Print messages via `Printf()`
- Allocate/free memory via `Z_Malloc()`/`Z_Free()`