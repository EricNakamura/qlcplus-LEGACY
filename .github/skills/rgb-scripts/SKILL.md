---
name: rgb-scripts
description: 'Author, debug, or port QLC+ RGB Matrix scripts in this fork. Use when creating or editing .lua RGB scripts, adding script properties, porting legacy .js scripts to Lua, registering scripts in resources/rgbscripts/CMakeLists.txt, or troubleshooting why an RGB script does not appear or render.'
argument-hint: 'Describe the RGB effect you want (e.g. "a bouncing ball with a size property")'
---

# QLC+ RGB Scripts (Lua)

## When to Use

- Creating a new RGB Matrix effect script.
- Editing an existing `.lua` script or its properties.
- Porting a legacy `.js` script to Lua.
- Registering a script so it shows up in the RGB Matrix editor.
- Debugging a script that fails to load or renders nothing.

## Critical fork context

This fork **replaced the JavaScript RGB engine with LuaJIT**. Read this before touching anything:

- `RGBScriptsCache` (`engine/src/rgbscriptscache.cpp`) loads **only `.lua` files** and its `script()` returns `RGBLua*`, not `RGBScript*`. Do not reintroduce `.js` handling.
- `RGBLua` (`engine/src/rgblua.h/.cpp`) wraps a per-instance `lua_State*` and runs scripts with LuaJIT (Lua 5.1 semantics — use `lua_objlen`, not `lua_rawlen`).
- `resources/rgbscripts/CMakeLists.txt` has all `.js` files **commented out**; only `sine.lua` and `test_script.lua` are installed. A new script is invisible until you add it there.
- `RGBAlgorithm::setStepFloat(double)` is a fork-added virtual (base no-op). The RGB Matrix drives scripts with a **continuous float phase at 50 Hz**, not integer steps — see "Continuous phase" below.
- The `devtool.html` / `devtool/devtool.js` preview tool is **JavaScript-only** and does not run Lua. Use it only as a visual reference for legacy `.js` scripts.

## Procedure: add a new Lua RGB script

1. **Create the script** in `resources/rgbscripts/<name>.lua`. Start from [the template](./assets/template.lua) and follow the [Lua API reference](./references/lua-api.md).
2. **Register it** in `resources/rgbscripts/CMakeLists.txt` by adding `<name>.lua` to `SCRIPT_FILES` (the `.js` entries stay commented out).
3. **Verify it loads**: the cache parses the `name = "..."` line by splitting on `=` and stripping quotes, so the declaration must be exactly `name = "My Effect"` on its own line.
4. **Build and run** the RGB Matrix editor to confirm the script appears and renders:
   ```bash
   cmake --build build
   ninja -C build run
   ```
5. **Test** with the engine test suite: `./unittest.sh ui` (or `ninja -C build check`).

## Script contract

A Lua script must declare these globals at the top level:

| Global | Required | Purpose |
|---|---|---|
| `apiVersion` | yes | Must be `> 0` (use `2`). `RGBLua::evaluate()` returns false if it is 0. |
| `name` | yes | Display name; also used by the cache to index the script. |
| `author` | no | Shown in the editor. |
| `properties` | no | Table of property descriptors (see below). |
| `rgbMapStepCount(width, height)` | yes | Returns the number of steps in one cycle (an integer). |
| `rgbMap(width, height, rgb, step)` | yes | Returns a 2D table `map[y][x]` of packed RGB integers. **`step` is the continuous float phase (0..1), not an integer index** — see below. |
| `rgbMapSetColors(colors)` | no | API v3+; receives a table of colors. |
| `rgbMapGetColors()` | no | API v3+; returns a table of colors. |

**Indexing:** Lua tables are 1-based. The C++ side already compensates, so write `for y = 1, height` and `for x = 1, width`, and return `map[y][x]`.

**Color packing:** `rgb` is a 24-bit integer. Unpack with `math.floor(rgb / 65536)` (R), `math.floor((rgb % 65536) / 256)` (G), `rgb % 256` (B); repack with `(r * 65536) + (g * 256) + b`.

## Properties

Declare a `properties` table of descriptors. `RGBLua::loadProperties()` reads `name`, `display`, `type`, `values`/`min`/`max`, and `default`, then **injects each default as a global variable** in the Lua VM. Read them directly by name in `rgbMap` (e.g. `orientation`, `resolution`).

```lua
properties = {
    { name = "orientation", display = "Orientation", type = "list",
      values = "Horizontal,Vertical", default = "Horizontal" },
    { name = "resolution", display = "Steps per Beat", type = "range",
      min = 2, max = 64, default = 16 }
}
```

- `type = "list"` → dropdown; `values` is a comma-separated string.
- `type = "range"` → slider; requires `min` and `max`.
- `type = "float"` / `"string"` → free value.
- `setProperty()` pushes numbers as Lua numbers and everything else as strings, so a `range` property arrives as a number and a `list` property as a string.

## Continuous phase (fork behavior)

Verified in `engine/src/rgbmatrix.cpp` and `engine/src/rgblua.cpp`:

- `RGBMatrix::write()` calls `setStepFloat(m_continuousPhase)` with a wrapped `[0, 1)` phase each 50 Hz tick, then calls `rgbMap(..., currentStepIndex(), ...)`.
- `RGBLua::rgbMap()` **ignores the integer step index** and pushes `m_stepFloat` (the phase) as the Lua `step` argument: `lua_pushnumber(m_luaState, m_stepFloat)`.

So inside Lua, `step` is a **float in `[0, 1)`**, never an integer. Do not compare it with `==` against an index. Compute a phase from it:

```lua
local global_phase = (step / resolution) * (math.pi * 2)
```

`rgbMapStepCount()` still returns an integer and defines the cycle length used by the editor.

### Editor preview limitation

The editor preview path (`RGBMatrix::previewMap()` → `RGBAlgorithm::rgbMap()`) does **not** call `setStepFloat()`. Only playback (`RGBMatrix::write()`) does. As a result, a continuous-phase script renders **statically** in the editor preview (frozen at the last phase, usually `0.0`) but animates correctly during playback. Do not treat a static preview as a broken script.

### Reference scripts

- `resources/rgbscripts/sine.lua` — the correct reference: uses `step` as a continuous phase.
- `resources/rgbscripts/test_script.lua` — **misleading**: it treats `step` as an integer index (`(x - 1) == step`), which no longer matches the engine and will not animate. Do not copy its step handling.

## Porting a legacy `.js` script

1. Copy the effect logic; keep the same `name`/`author`/`apiVersion`.
2. Convert `algo.properties.push("name:x|type:list|display:...|values:...")` strings into the Lua `properties` table form.
3. Replace `algo.setX`/`algo.getX` accessors with direct global reads (the C++ injects the values).
4. Convert `new Array(height)` loops to 1-based Lua tables.
5. Save as `.lua`, register it, and remove/keep the `.js` file commented out in CMake.

## Troubleshooting

| Symptom | Cause |
|---|---|
| Script missing from the editor | Not added to `SCRIPT_FILES` in `resources/rgbscripts/CMakeLists.txt`, or the `name = "..."` line is malformed. |
| "não possui a função global rgbMap()" warning | `rgbMap` is not a global function (e.g. declared `local`). |
| Script loads but renders black | Wrong table indexing (0-based) or `rgb` unpacked incorrectly. |
| Script animates in playback but is static in the editor preview | Expected: the preview does not call `setStepFloat()` (see above). |
| Script never animates at all | `step` is being treated as an integer index instead of a float phase. |
| Properties ignored | Property `name` missing, or the script reads a different global than the declared `name`. |
| `apiVersion` 0 / script rejected | `apiVersion` not declared or not a number. |
