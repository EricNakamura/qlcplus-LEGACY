-- ==========================================================
-- Varredura Lua
-- Exemplo de efeito RGB para o motor LuaJIT desta fork.
--
-- IMPORTANTE: o argumento 'step' recebido por rgbMap() é uma
-- FASE contínua (float em [0, 1)), e NÃO um índice inteiro de
-- passo. Nunca compare 'step' com '==' contra um número de
-- passo. Consulte docs/RGB-SCRIPTS-LUA.md para mais detalhes.
-- ==========================================================
apiVersion = 2
name = "Varredura Lua"
author = "Eric Nakamura LEGACY Fork"

-- ==========================================================
-- Propriedades (o C++ injeta cada 'default' como variável global)
-- ==========================================================
properties = {
    {
        name = "orientation",
        display = "Orientação",
        type = "list",
        values = "Horizontal,Vertical",
        default = "Horizontal"
    },
    {
        name = "tail",
        display = "Tamanho da Cauda",
        type = "range",
        min = 0,
        max = 10,
        default = 4
    }
}

-- ==========================================================
-- rgbMapStepCount: número de passos de um ciclo completo.
-- Para uma varredura linear, um ciclo equivale a uma travessia.
-- ==========================================================
function rgbMapStepCount(width, height)
    if orientation == "Horizontal" then
        return width
    else
        return height
    end
end

-- ==========================================================
-- rgbMap: devolve a matriz map[y][x] com as cores empacotadas.
-- 'head = step * span' posiciona a cabeça da varredura de forma
-- contínua; a cauda decai em intensidade e dá wrap no fim.
-- ==========================================================
function rgbMap(width, height, rgb, step)
    local map = {}

    -- Separa os canais da cor base (rgb é um inteiro de 24 bits)
    local r_base = math.floor(rgb / 65536)
    local g_base = math.floor((rgb % 65536) / 256)
    local b_base = rgb % 256

    local horizontal = (orientation == "Horizontal")
    local span = horizontal and width or height
    local head = step * span

    for y = 1, height do
        map[y] = {}
        for x = 1, width do
            local i = horizontal and (x - 1) or (y - 1)

            -- Distância atrás da cabeça, com wrap para dar continuidade
            local dist = head - i
            if dist < 0 then
                dist = dist + span
            end

            local intensity
            if tail <= 0 then
                intensity = (dist < 1) and 1 or 0
            else
                intensity = 1 - (dist / (tail + 1))
                if intensity < 0 then
                    intensity = 0
                end
            end

            local p_r = math.floor(r_base * intensity)
            local p_g = math.floor(g_base * intensity)
            local p_b = math.floor(b_base * intensity)

            map[y][x] = (p_r * 65536) + (p_g * 256) + p_b
        end
    end

    return map
end
