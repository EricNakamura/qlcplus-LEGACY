-- ==========================================================
-- Cabeçalho
-- ==========================================================
apiVersion = 2
name = "Onda Sine Continua"
author = "O Seu Fork do QLC+"

-- ==========================================================
-- Propriedades
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
        name = "resolution", 
        display = "Ciclo (Steps per Beat)", 
        type = "range", 
        min = 2, 
        max = 64, 
        default = 16 
    }
}

-- ==========================================================
-- Função: rgbMapStepCount
-- Retorna a resolução do ciclo (quantas batidas formam a onda)
-- ==========================================================
function rgbMapStepCount(width, height)
    return resolution
end

-- ==========================================================
-- Função: rgbMap
-- O pulo do gato: 'step' agora é um Float (ex: 2.14, 2.50...)
-- ==========================================================
function rgbMap(width, height, rgb, step)
    local map = {}

    -- 1. Extração dos canais RGB da cor base
    local r_base = math.floor(rgb / 65536)
    local g_base = math.floor((rgb % 65536) / 256)
    local b_base = rgb % 256

    -- 2. A Mágica Contínua: 
    -- Como o 'step' avança quebrado a 50fps, a fase corre perfeitamente.
    local global_phase = (step / resolution) * (math.pi * 2)

    for y = 1, height do
        map[y] = {}
        for x = 1, width do
            local local_phase = 0
            
            -- 3. Distribuição espacial da onda
            if orientation == "Horizontal" then
                local_phase = (x / width) * (math.pi * 2)
            else
                local_phase = (y / height) * (math.pi * 2)
            end

            -- 4. O cálculo do Seno (-1 a 1 normalizado para 0 a 1)
            local intensity = (math.sin(global_phase + local_phase) + 1) / 2

            -- 5. Multiplica a intensidade pela cor escolhida
            local p_r = math.floor(r_base * intensity)
            local p_g = math.floor(g_base * intensity)
            local p_b = math.floor(b_base * intensity)

            -- 6. Remonta o valor bitwise para o C++
            map[y][x] = (p_r * 65536) + (p_g * 256) + p_b
        end
    end

    return map
end