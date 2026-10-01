# Scripts RGB em Lua (LuaJIT)

Guia de uso do motor de efeitos RGB baseado em **LuaJIT** desta fork do QLC+.

Esta fork substituiu o motor JavaScript original (`QScriptEngine`) por uma
implementação própria em LuaJIT (`engine/src/rgblua.*`). Os efeitos da
**RGB Matrix** agora são arquivos `.lua`. O objetivo é desempenho: LuaJIT é
muito mais rápido que o motor JS anterior, o que permite animar matrizes
grandes com suavidade.

> Documentação interna de referência (para agentes): `.agents/skills/rgb-scripts/`.
> Este documento é a versão para pessoas e é a fonte canônica.

---

## Índice

1. [O que mudou em relação ao QLC+ original](#1-o-que-mudou-em-relação-ao-qlc-original)
2. [Onde os scripts ficam](#2-onde-os-scripts-ficam)
3. [Contrato do script](#3-contrato-do-script)
4. [Passo a passo: criando seu primeiro efeito](#4-passo-a-passo-criando-seu-primeiro-efeito)
5. [Propriedades](#5-propriedades)
6. [Fase contínua (o "pulo do gato")](#6-fase-contínua-o-pulo-do-gato)
7. [Cores: empacotar e desempacotar](#7-cores-empacotar-e-desempacotar)
8. [API v3: cores dinâmicas](#8-api-v3-cores-dinâmicas)
9. [Portando scripts `.js` para `.lua`](#9-portando-scripts-js-para-lua)
10. [Solução de problemas](#10-solução-de-problemas)
11. [Exemplo completo comentado](#11-exemplo-completo-comentado)

---

## 1. O que mudou em relação ao QLC+ original

| | QLC+ original | Esta fork |
|---|---|---|
| Linguagem | JavaScript (`QScriptEngine`) | **Lua 5.1 / LuaJIT** |
| Extensão | `.js` | **`.lua`** |
| Cache | `RGBScriptsCache` carrega `.js` | `RGBScriptsCache` carrega **apenas `.lua`** |
| Classe do motor | `RGBScript` | `RGBLua` |
| Avanço do efeito | passo inteiro | **fase contínua (float) a 50 Hz** |

Consequências práticas:

- Scripts `.js` **não são mais executados**. É preciso portá-los para `.lua`
  (veja a [seção 9](#9-portando-scripts-js-para-lua)).
- O argumento `step` de `rgbMap()` mudou de significado — leia a
  [seção 6](#6-fase-contínua-o-pulo-do-gato) antes de escrever qualquer coisa.

---

## 2. Onde os scripts ficam

**No repositório (scripts embutidos):**

```
resources/rgbscripts/*.lua
```

**Instalados / scripts do usuário:**

| Sistema | Diretório do usuário |
|---|---|
| macOS | `~/Library/Application Support/QLC+/RGBScripts` |
| Linux | `~/.qlcplus/rgbscripts` |
| Windows | `%APPDATA%\QLC+\RGBScripts` |

O cache varre esses diretórios e carrega **somente arquivos `.lua`**. Basta
colocar o arquivo lá (ou em `resources/rgbscripts/` antes de compilar) para o
efeito aparecer no editor da RGB Matrix.

---

## 3. Contrato do script

Todo script precisa declarar variáveis globais e duas funções globais.

### Globais

| Global | Obrigatório | Função |
|---|---|---|
| `apiVersion` | **sim** | Deve ser `> 0` (use `2`). O motor rejeita o script se for `0`. |
| `name` | **sim** | Nome exibido no editor; também é a chave usada pelo cache. |
| `author` | não | Autor exibido no editor. |
| `properties` | não | Tabela de descritores de propriedades (veja a [seção 5](#5-propriedades)). |

> **Regra do `name`:** o cache lê a linha do arquivo e a interpreta dividindo
> por `=`. Por isso declare exatamente `name = "Meu Efeito"` em **uma linha
> própria**. Formas criativas (concatenação, `local name = ...`) fazem o script
> não ser encontrado.

### Funções

| Função | Obrigatório | Descrição |
|---|---|---|
| `rgbMapStepCount(width, height)` | **sim** | Retorna o número de passos de um ciclo (inteiro). |
| `rgbMap(width, height, rgb, step)` | **sim** | Retorna a matriz `map[y][x]` com cores empacotadas. **`step` é uma fase float** — veja a [seção 6](#6-fase-contínua-o-pulo-do-gato). |
| `rgbMapSetColors(colors)` | não | API v3+: recebe uma tabela de cores. |
| `rgbMapGetColors()` | não | API v3+: retorna uma tabela de cores. |

### Indexação

Lua indexa tabelas a partir de **1**. O lado C++ já compensa isso, então
escreva naturalmente:

```lua
for y = 1, height do
    for x = 1, width do
        map[y][x] = 0
    end
end
```

---

## 4. Passo a passo: criando seu primeiro efeito

1. Crie o arquivo em `resources/rgbscripts/<nome>.lua`. Comece pelo esqueleto:

   ```lua
   apiVersion = 2
   name = "Meu Efeito"
   author = "Seu Nome"

   function rgbMapStepCount(width, height)
       return width
   end

   function rgbMap(width, height, rgb, step)
       local map = {}
       for y = 1, height do
           map[y] = {}
           for x = 1, width do
               map[y][x] = rgb   -- matriz toda acesa na cor escolhida
           end
       end
       return map
   end
   ```

2. **Registre o script** em `resources/rgbscripts/CMakeLists.txt`, adicionando
   o nome do arquivo à lista `SCRIPT_FILES`:

   ```cmake
   set(SCRIPT_FILES
       # ... (os .js continuam comentados)
       sine.lua
       test_script.lua
       meu_efeito.lua      # <-- adicione aqui
   )
   ```

3. **Compile e rode:**

   ```bash
   cmake --build build
   ninja -C build run
   ```

4. No QLC+, abra o **RGB Matrix** e selecione o efeito na lista de algoritmos.

5. Rode a suíte de testes antes de enviar qualquer coisa:

   ```bash
   ./unittest.sh ui       # ou: ninja -C build check
   ```

> Durante o desenvolvimento você também pode largar o `.lua` direto no
> diretório de usuário (seção 2) para testar sem recompilar.

---

## 5. Propriedades

Propriedades são controles que aparecem no editor (sliders, menus) e que o
usuário ajusta em tempo real. Declare uma tabela `properties`; o motor lê cada
descritor e **injeta o valor de `default` como uma variável global** na VM do
Lua.

```lua
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
        display = "Passos por Ciclo",
        type = "range",
        min = 2,
        max = 64,
        default = 16
    }
}
```

Depois é só ler a propriedade pelo `name` dentro de `rgbMap()`:

```lua
if orientation == "Horizontal" then
    -- ...
end
local phase = (step / resolution) * (math.pi * 2)
```

### Campos de um descritor

| Campo | Usado para |
|---|---|
| `name` | Nome da variável global injetada. **Obrigatório** — entradas sem `name` são ignoradas. |
| `display` | Rótulo mostrado no editor. |
| `type` | `"range"`, `"list"`, `"float"`, `"string"`. |
| `min`, `max` | Lidos apenas quando `type == "range"`. |
| `values` | Lido apenas quando `type == "list"`; string separada por vírgulas. |
| `default` | Valor inicial injetado na VM ao carregar o script. |

### Como cada tipo chega ao Lua

- `type = "list"` → dropdown; o valor chega como **string**.
- `type = "range"` → slider; o valor chega como **número**.
- `type = "float"` / `"string"` → valor livre (número ou texto).

Internamente, o motor tenta converter o valor com `toDouble()`: se parecer
número, vira número no Lua; caso contrário, vira string.

---

## 6. Fase contínua (o "pulo do gato")

**Esta é a diferença mais importante em relação ao QLC+ original.**

No QLC+ clássico, `rgbMap()` recebia `step` como um **índice inteiro** de passo.
Nesta fork, o RGB Matrix roda a **50 Hz** e avança uma **fase contínua** entre
`0.0` e `1.0` a cada tick. O motor entrega essa fase ao Lua:

```lua
function rgbMap(width, height, rgb, step)
    -- 'step' é um float em [0, 1). NUNCA compare com '==' a um índice!
```

Ou seja: **não faça** `if (x - 1) == step then ...`. Em vez disso, converta a
fase em posição/ângulo:

```lua
-- Para efeitos cíclicos (seno, ondas):
local global_phase = (step / resolution) * (math.pi * 2)

-- Para varredura linear (uma cabeça que percorre a matriz):
local head = step * span          -- span = width ou height
```

O `rgbMapStepCount()` continua devolvendo um inteiro, e define o tamanho do
ciclo mostrado no editor. O multiplicador de velocidade e a duração do efeito
(no editor) controlam a rapidez da fase — não o "tempo por passo".

### Limitação conhecida: o preview do editor

O caminho de preview do editor (`RGBMatrix::previewMap()`) **não** chama
`setStepFloat()`. Só a execução real (`RGBMatrix::write()`) chama. Por isso
um script de fase contínua aparece **parado (estático)** no preview do editor,
mas **anima corretamente** quando o efeito roda.

**Isso não é bug do seu script.** Valide a animação executando o efeito.

---

## 7. Cores: empacotar e desempacotar

`rgb` é um inteiro de 24 bits (`0xRRGGBB`).

```lua
-- Desempacotar
local r = math.floor(rgb / 65536)
local g = math.floor((rgb % 65536) / 256)
local b = rgb % 256

-- Empacotar de volta
local packed = (r * 65536) + (g * 256) + b
```

Para criar um tom com brilho proporcional (intensidade de `0.0` a `1.0`):

```lua
local intensity = 0.5
local p_r = math.floor(r * intensity)
local p_g = math.floor(g * intensity)
local p_b = math.floor(b * intensity)
map[y][x] = (p_r * 65536) + (p_g * 256) + p_b
```

---

## 8. API v3: cores dinâmicas

Se `apiVersion >= 3`, o script pode implementar duas funções extras para dar ao
efeito controle sobre a paleta de cores (em vez de usar a cor única escolhida
no editor):

```lua
function rgbMapSetColors(colors)
    -- 'colors' é uma tabela Lua de inteiros RGB vindos do editor
    myColors = colors
end

function rgbMapGetColors()
    -- Devolve uma tabela de inteiros RGB
    return { 0xFF0000, 0x00FF00, 0x0000FF }
end
```

- `rgbMapSetColors()` é chamada apenas quando `apiVersion >= 3`.
- Em `rgbMapGetColors()`, valores que não forem números são ignorados.
- Declare os descritores de cor correspondentes em `properties` (normalmente
  como `type = "list"`).

---

## 9. Portando scripts `.js` para `.lua`

1. Copie a lógica do efeito; mantenha `name`, `author` e `apiVersion`.
2. Converta as chamadas de propriedades:

   ```js
   // JavaScript
   algo.properties.push("name:orientation|type:list|display:Orientation|values:Horizontal,Vertical|write:setOrientation|read:getOrientation");
   ```

   ```lua
   -- Lua
   properties = {
       { name = "orientation", display = "Orientação", type = "list",
         values = "Horizontal,Vertical", default = "Horizontal" }
   }
   ```

3. Troque os acessores `algo.setX()` / `algo.getX()` por **leitura direta** do
   global injetado (ex.: `orientation`).
4. Converta laços baseados em 0 (`new Array(height)`) para tabelas Lua
   **1-based** (`for y = 1, height`).
5. **Reescreva o tratamento de `step`** para usar a fase contínua
   ([seção 6](#6-fase-contínua-o-pulo-do-gato)) — este é o ponto mais comum de
   erro ao portar.
6. Salve como `.lua`, registre no `CMakeLists.txt` e deixe o `.js` antigo
   comentado (ou remova-o).

---

## 10. Solução de problemas

| Sintoma | Causa provável |
|---|---|
| O efeito não aparece no editor | Não foi adicionado a `SCRIPT_FILES` no `CMakeLists.txt`, ou a linha `name = "..."` está malformada. |
| Aviso `não possui a função global rgbMap()` | `rgbMap` não é global (ex.: foi declarada como `local`, ou com erro de sintaxe antes). |
| O script carrega mas pinta tudo preto | Indexação errada (0-based) ou desempacotamento de `rgb` incorreto. |
| Anima na execução mas fica parado no preview | Esperado: o preview não chama `setStepFloat()` (veja a [seção 6](#6-fase-contínua-o-pulo-do-gato)). |
| Nunca anima em lugar nenhum | `step` está sendo tratado como índice inteiro em vez de fase float. |
| As propriedades são ignoradas | Falta o `name` no descritor, ou o script lê um global diferente do `name` declarado. |
| `apiVersion` 0 / script rejeitado | `apiVersion` ausente ou não numérico. |

---

## 11. Exemplo completo comentado

Os dois scripts de referência ficam em `resources/rgbscripts/`:

- **`sine.lua`** — onda senoidal com fase contínua e propriedades
  `list` + `range`. **Use este como referência principal.**
- **`test_script.lua`** — varredura contínua com cauda de brilho decrescente.

Trecho central de `sine.lua`:

```lua
function rgbMap(width, height, rgb, step)
    local map = {}

    local r_base = math.floor(rgb / 65536)
    local g_base = math.floor((rgb % 65536) / 256)
    local b_base = rgb % 256

    -- A fase global avança continuamente com o tempo
    local global_phase = (step / resolution) * (math.pi * 2)

    for y = 1, height do
        map[y] = {}
        for x = 1, width do
            local local_phase
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
```

---

## Veja também

- `engine/src/rgblua.h` / `engine/src/rgblua.cpp` — implementação do motor.
- `engine/src/rgbscriptscache.cpp` — carregamento dos scripts `.lua`.
- `engine/src/rgbmatrix.cpp` — fase contínua, velocidade e 50 Hz.
- `.agents/skills/rgb-scripts/` — referência interna para agentes de código.
