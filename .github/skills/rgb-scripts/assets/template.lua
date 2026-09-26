-- ==========================================================
-- <Effect name>
-- ==========================================================
apiVersion = 2
name = "My Effect"
author = "Your Name"

-- ==========================================================
-- Properties (optional)
-- The C++ injects each 'default' as a global variable.
-- ==========================================================
properties = {
    {
        name = "orientation",
        display = "Orientation",
        type = "list",
        values = "Horizontal,Vertical",
        default = "Horizontal"
    },
    {
        name = "resolution",
        display = "Steps per Cycle",
        type = "range",
        min = 2,
        max = 64,
        default = 16
    }
}

-- ==========================================================
-- rgbMapStepCount: number of steps in one cycle
-- ==========================================================
function rgbMapStepCount(width, height)
    return resolution
end

-- ==========================================================
-- rgbMap: return a 2D table map[y][x] of packed RGB integers.
-- 'step' is a FLOAT phase in [0, 1) (set via setStepFloat), NOT an
-- integer index. Never compare it with '==' against a step number.
-- Note: the editor preview does not call setStepFloat, so this effect
-- looks static in the preview but animates during playback.
-- ==========================================================
function rgbMap(width, height, rgb, step)
    local map = {}

    -- Unpack the base color
    local r_base = math.floor(rgb / 65536)
    local g_base = math.floor((rgb % 65536) / 256)
    local b_base = rgb % 256

    -- Continuous phase in radians
    local global_phase = (step / resolution) * (math.pi * 2)

    for y = 1, height do
        map[y] = {}
        for x = 1, width do
            local local_phase = 0
            if orientation == "Horizontal" then
                local_phase = (x / width) * (math.pi * 2)
            else
                local_phase = (y / height) * (math.pi * 2)
            end

            local intensity = (math.sin(global_phase + local_phase) + 1) / 2

            local p_r = math.floor(r_base * intensity)
            local p_g = math.floor(g_base * intensity)
            local p_b = math.floor(b_base * intensity)

            map[y][x] = (p_r * 65536) + (p_g * 256) + p_b
        end
    end

    return map
end
