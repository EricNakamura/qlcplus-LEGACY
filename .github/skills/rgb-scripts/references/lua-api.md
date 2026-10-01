# Lua RGB Script API Reference

Source of truth: `engine/src/rgblua.cpp` (`RGBLua`). This fork uses **LuaJIT (Lua 5.1)**.

## Globals read by `RGBLua::evaluate()`

| Global | Type | Notes |
|---|---|---|
| `apiVersion` | number | `evaluate()` returns `m_apiVersion > 0`; a script with `apiVersion = 0` is rejected. |
| `name` | string | Falls back to `"Unnamed Lua"`. Also parsed by `RGBScriptsCache::load()`. |
| `author` | string | Falls back to `"Unknown"`. |
| `properties` | table | Optional; see below. |

## Required functions

### `rgbMapStepCount(width, height) -> int`

Called with two integers, must return an integer step count. If the function is missing, `rgbMapStepCount()` returns `-1`.

### `rgbMap(width, height, rgb, step) -> table`

- `width`, `height`: integers.
- `rgb`: 24-bit packed color integer.
- `step`: a **float phase in `[0, 1)`** — see continuous phase in `SKILL.md`.
- Must return a 2D table `map[y][x]` (1-based) of packed RGB integers.

> **Verified in `RGBLua::rgbMap()`:** the C++ integer step index passed by `RGBMatrix` is **discarded**; the Lua `step` argument is `m_stepFloat`, set by `setStepFloat()`. Never treat `step` as an integer index.

The C++ reads the table with `lua_objlen` for the outer length, then `lua_rawgeti(..., y + 1)` and `lua_rawgeti(..., x + 1)`. Values are read with `lua_tointeger`.

## Optional functions (API v3+)

- `rgbMapSetColors(colors)` — receives a Lua table of color integers. `RGBLua::rgbMapSetColors()` returns early if `apiVersion < 3`.
- `rgbMapGetColors()` — must return a table of color integers; non-numbers are skipped.

## Property descriptors

`RGBLua::loadProperties()` iterates the `properties` table and reads these fields per entry:

| Field | Used for |
|---|---|
| `name` | Global variable name injected into the VM. Required — entries without it are skipped. |
| `display` | Label shown in the editor. |
| `type` | `"range"`, `"list"`, `"float"`, or anything else → `String`. |
| `min`, `max` | Read only when `type == "range"`. |
| `values` | Read only when `type == "list"`; comma-separated string, split on `,`. |
| `default` | Injected via `setProperty()` immediately after loading. |

`setProperty(name, value)` converts the value with `toDouble()`; if it parses as a number it is pushed as a Lua number, otherwise as a string. So `range`/`float` properties arrive as numbers and `list`/`string` properties as strings.

## Color helpers

```lua
-- unpack
local r = math.floor(rgb / 65536)
local g = math.floor((rgb % 65536) / 256)
local b = rgb % 256

-- pack
local packed = (r * 65536) + (g * 256) + b
```

## Minimal skeleton

```lua
apiVersion = 2
name = "My Effect"
author = "Me"

properties = {
    { name = "orientation", display = "Orientation", type = "list",
      values = "Horizontal,Vertical", default = "Horizontal" }
}

function rgbMapStepCount(width, height)
    return width
end

function rgbMap(width, height, rgb, step)
    local map = {}
    for y = 1, height do
        map[y] = {}
        for x = 1, width do
            map[y][x] = 0
        end
    end
    return map
end
```

## Reference scripts

- `resources/rgbscripts/sine.lua` — continuous-phase sine wave with `list` + `range` properties. **Use this as the reference.**
- `resources/rgbscripts/test_script.lua` — continuous-phase sweep with a fading tail.

For the full human-facing guide, see [`docs/RGB-SCRIPTS-LUA.md`](../../../docs/RGB-SCRIPTS-LUA.md).
