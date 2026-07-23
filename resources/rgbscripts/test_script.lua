-- ==========================================================
-- Cabeçalho Obrigatório
-- ==========================================================
apiVersion = 2
name = "Varredura Lua"
author = "O Seu Fork do QLC+"

-- ==========================================================
-- Declaração das Propriedades (A Interface Gráfica lê isso)
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
        max = 5, 
        default = 0 
    }
}

-- ==========================================================
-- Função: rgbMapStepCount
-- Retorna o número total de passos para o efeito dar um ciclo
-- ==========================================================
function rgbMapStepCount(width, height)
    -- As variáveis 'orientation' e 'tail' já foram injetadas 
    -- automaticamente na memória global pelo nosso C++!
    if orientation == "Horizontal" then
        return width
    else
        return height
    end
end

-- ==========================================================
-- Função: rgbMap
-- Retorna a matriz bidimensional com as cores de cada pixel
-- ==========================================================
function rgbMap(width, height, rgb, step)
    local map = {}

    -- O Lua indexa tabelas a partir do 1 (o nosso C++ já compensa isso)
    for y = 1, height do
        map[y] = {}
        for x = 1, width do
            map[y][x] = 0 -- Inicia o pixel apagado (preto)
            
            if orientation == "Horizontal" then
                -- Lógica para varredura da Esquerda para a Direita
                if (x - 1) == step then
                    map[y][x] = rgb -- Pixel principal aceso
                elseif tail > 0 and (x - 1) >= (step - tail) and (x - 1) < step then
                    map[y][x] = rgb -- Acende a cauda se estiver configurada
                end
            else
                -- Lógica para varredura de Cima para Baixo
                if (y - 1) == step then
                    map[y][x] = rgb
                elseif tail > 0 and (y - 1) >= (step - tail) and (y - 1) < step then
                    map[y][x] = rgb
                end
            end
        end
    end

    return map
end