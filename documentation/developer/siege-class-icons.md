# Custom Siege class-picker icons

The client now uses the active team's class definitions for the six category
buttons in the Siege class picker. Previously these buttons always loaded
`gfx/mp/c_icon_*`, so maps replaced globally shared images to customize them.
Selected-class details and scoreboard icons already use `class_shader`.

## Map setup

Give the map's `.scl` and `.team` files unique names, select those teams using
`UseTeam` in the map's `.siege` file, and put the images in a map-specific path.
Inside each `.scl` file's `ClassInfo` block, point the existing key at that path:

```text
class_shader "gfx/jof/moba/c_icon_infantry"
```

For example, include `gfx/jof/moba/c_icon_infantry.jpg` in the PK3. No new key,
menu override, shader remap, or server-code change is needed. Keep the shader
reference extensionless. The existing class parser determines the category from
the **end of the shader name**, so preserve these suffixes:

| Picker button | Shader-name suffix |
| --- | --- |
| 1 | `infantry` |
| 2 | `heavy_weapons` |
| 3 | `demolitionist` |
| 4 | `vanguard` |
| 5 | `support` |
| 6 | `jedi_general` |

When several classes share a category, the first one in the active `.team` file
provides the category-button icon; individual class details still show their own
icons. Missing categories or a zero shader handle fall back to the corresponding
stock path. The client refreshes the buttons whenever the selected team changes.

## BattleArena migration

`{JoF}BattleArena.pk3` already has unique `moba_*` classes and `Moba1_*` teams.
Move its eight `gfx/mp/c_icon_*.jpg` images into `gfx/jof/moba/` and update the
14 MOBA classes' `class_shader` references to match. Preserve the filenames,
including `c_icon_adept`: its existing fallback category is infantry, and each
MOBA team lists its actual infantry hero before this extra class.

Remove the old image entries from the **replacement** PK3. Do not install it
alongside the old PK3: the old archive would still override the stock icons.
Keep a backup outside the game's search paths. Servers and clients should use
the same updated map package; old clients still have hardcoded picker buttons
and will show stock category icons with the repacked map.

## In-game verification still required

1. With only the replacement MOBA package installed, open the Siege picker on
   `jof_moba`; check all six icons on both teams and the extra infantry class.
2. Switch teams and reopen the menu; check the selected-class and scoreboard
   icons too.
3. Load `siege_hoth` and `siege_korriban` without uninstalling MOBA. Check that
   their class icons come from the vanilla assets, not MOBA.
4. Return to MOBA and verify its icons return. Check a team missing a category
   to ensure a previous map's icon cannot remain on that button.
