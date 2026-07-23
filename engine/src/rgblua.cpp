/*
  Q Light Controller Plus
  rgbscript_lua.cpp

  Copyright (c) Seu Nome / Eric Nakamura LEGACY Fork
*/

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QFile>
#include <QTextStream>
#include <QDebug>

#include "rgblua.h"
#include "qlcconfig.h"

RGBLua::RGBLua(Doc* doc)
    : RGBAlgorithm(doc)
    , m_luaState(nullptr)
    , m_apiVersion(0)
    , m_rgbMapRef(LUA_NOREF)
    , m_rgbMapStepCountRef(LUA_NOREF)
    , m_rgbMapSetColorsRef(LUA_NOREF)
    , m_rgbMapGetColorsRef(LUA_NOREF)
{
}

RGBLua::RGBLua(const RGBLua& s)
    : RGBAlgorithm(s.doc())
    , m_luaState(nullptr)
    , m_fileName(s.m_fileName)
    , m_scriptContent(s.m_scriptContent) // Puxamos da RAM, não do disco!
    , m_apiVersion(s.m_apiVersion)
    , m_rgbMapRef(LUA_NOREF)
    , m_rgbMapStepCountRef(LUA_NOREF)
    , m_rgbMapSetColorsRef(LUA_NOREF)
    , m_rgbMapGetColorsRef(LUA_NOREF)
{
    // 1. Inicia a VM e roda o script a partir da RAM (muito mais rápido que luaL_dofile)
    initEngine();
    if (luaL_dostring(m_luaState, m_scriptContent.toUtf8().constData()) != LUA_OK) {
        lua_pop(m_luaState, 1);
    } else {
        // 2. AQUI ENTRA A CORREÇÃO DE ESTADO (Amnésia)
        // Precisamos copiar as propriedades configuradas no QLC+ para a nova VM
        /* 
        foreach (RGBScriptProperty cap, s.m_properties) {
            setProperty(cap.m_name, s.property(cap.m_name));
        }
        */
    }
}

RGBLua::~RGBLua()
{
    cleanupEngine();
}

RGBLua& RGBLua::operator=(const RGBLua& s)
{
    if (this != &s)
    {
        m_fileName = s.m_fileName;
        m_contents = s.m_contents;
        m_apiVersion = s.m_apiVersion;
        evaluate();
    }
    return *this;
}

bool RGBLua::operator==(const RGBLua& s) const
{
    return this->fileName().isEmpty() == false && this->fileName() == s.fileName();
}

RGBAlgorithm* RGBLua::clone() const
{
    return new RGBLua(*this);
}

bool RGBLua::load(const QString& fileName)
{
    m_fileName = fileName;
    
    // Lê do disco UMA vez e guarda na RAM
    QFile file(m_fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Erro ao carregar script Lua:" << m_fileName;
        return false;
    }

    QTextStream stream(&file);
    m_scriptContent = stream.readAll();
    file.close();

    return evaluate();
}
QString RGBLua::fileName() const
{
    return m_fileName;
}

void RGBLua::initEngine()
{
    if (m_luaState == nullptr)
    {
        // Cria uma nova máquina virtual LuaJIT isolada para este efeito
        m_luaState = luaL_newstate();
        // Abre as bibliotecas base do Lua (math, string, table)
        luaL_openlibs(m_luaState); 
    }
}

void RGBLua::cleanupEngine()
{
    if (m_luaState != nullptr)
    {
        lua_close(m_luaState);
        m_luaState = nullptr;
    }
}

bool RGBLua::evaluate()
{
    initEngine();

    // 1. O Lua lê e carrega o arquivo direto do disco (Redução extrema)
    if (luaL_dofile(m_luaState, m_fileName.toUtf8().constData()) != LUA_OK) {
        qWarning() << "Erro ao compilar Lua:" << lua_tostring(m_luaState, -1);
        lua_pop(m_luaState, 1);
        return false;
    }

    // 2. Validação de Sobrevivência (Cenário de Falha)
    // Se o script não declarou a função rgbMap globalmente, matamos o carregamento aqui!
    lua_getglobal(m_luaState, "rgbMap");
    if (!lua_isfunction(m_luaState, -1)) {
        qWarning() << m_fileName << "não possui a função global rgbMap()!";
        lua_pop(m_luaState, 1);
        return false;
    }
    lua_pop(m_luaState, 1); // Limpa a stack

    // 3. Cache das variáveis globais em 1 linha (usando if com atribuição)
    lua_getglobal(m_luaState, "name");
    m_name = lua_isstring(m_luaState, -1) ? QString::fromUtf8(lua_tostring(m_luaState, -1)) : "Unnamed Lua";
    
    lua_getglobal(m_luaState, "author");
    m_author = lua_isstring(m_luaState, -1) ? QString::fromUtf8(lua_tostring(m_luaState, -1)) : "Unknown";
    
    lua_getglobal(m_luaState, "apiVersion");
    m_apiVersion = lua_isnumber(m_luaState, -1) ? lua_tointeger(m_luaState, -1) : 0;

    if (m_rgbMapRef != LUA_NOREF) luaL_unref(m_luaState, LUA_REGISTRYINDEX, m_rgbMapRef);
    if (m_rgbMapStepCountRef != LUA_NOREF) luaL_unref(m_luaState, LUA_REGISTRYINDEX, m_rgbMapStepCountRef);
    if (m_rgbMapSetColorsRef != LUA_NOREF) luaL_unref(m_luaState, LUA_REGISTRYINDEX, m_rgbMapSetColorsRef);
    if (m_rgbMapGetColorsRef != LUA_NOREF) luaL_unref(m_luaState, LUA_REGISTRYINDEX, m_rgbMapGetColorsRef);

    // Salva a função rgbMap
    lua_getglobal(m_luaState, "rgbMap");
    if (lua_isfunction(m_luaState, -1)) {
        m_rgbMapRef = luaL_ref(m_luaState, LUA_REGISTRYINDEX);
    } else {
        lua_pop(m_luaState, 1);
        m_rgbMapRef = LUA_NOREF;
    }

    // Salva a função rgbMapStepCount
    lua_getglobal(m_luaState, "rgbMapStepCount");
    if (lua_isfunction(m_luaState, -1)) {
        m_rgbMapStepCountRef = luaL_ref(m_luaState, LUA_REGISTRYINDEX);
    } else {
        lua_pop(m_luaState, 1);
        m_rgbMapStepCountRef = LUA_NOREF;
    }

    // (Opcional) Salva a função rgbMapSetColors
    lua_getglobal(m_luaState, "rgbMapSetColors");
    if (lua_isfunction(m_luaState, -1)) m_rgbMapSetColorsRef = luaL_ref(m_luaState, LUA_REGISTRYINDEX);
    else lua_pop(m_luaState, 1);

    // (Opcional) Salva a função rgbMapGetColors
    lua_getglobal(m_luaState, "rgbMapGetColors");
    if (lua_isfunction(m_luaState, -1)) m_rgbMapGetColorsRef = luaL_ref(m_luaState, LUA_REGISTRYINDEX);
    else lua_pop(m_luaState, 1);
    
    lua_pop(m_luaState, 3); // Limpa as 3 consultas de uma vez só!

    loadProperties();

    return m_apiVersion > 0;
}

// --- Funções de Informação da API ---
QString RGBLua::name() const { return m_name; }
QString RGBLua::author() const { return m_author; }
int RGBLua::apiVersion() const { return m_apiVersion; }
RGBAlgorithm::Type RGBLua::type() const { return RGBAlgorithm::Script; }
int RGBLua::acceptColors() const { return 2; }

// --- O Coração da Matrix de Alta Performance ---

int RGBLua::rgbMapStepCount(const QSize& size)
{
    if (!m_luaState || m_rgbMapStepCountRef == LUA_NOREF) return -1;

    lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, m_rgbMapStepCountRef);

    lua_getglobal(m_luaState, "rgbMapStepCount");
    if (!lua_isfunction(m_luaState, -1)) {
        lua_pop(m_luaState, 1);
        return -1;
    }

    lua_pushinteger(m_luaState, size.width());
    lua_pushinteger(m_luaState, size.height());

    if (lua_pcall(m_luaState, 2, 1, 0) != LUA_OK) {
        qWarning() << "Lua rgbMapStepCount Error:" << lua_tostring(m_luaState, -1);
        lua_pop(m_luaState, 1);
        return -1;
    }

    int steps = lua_tointeger(m_luaState, -1);
    lua_pop(m_luaState, 1);
    return steps;
}

void RGBLua::rgbMap(const QSize& size, uint rgb, int step, RGBMap& map)
{
    if (!m_luaState || m_rgbMapRef == LUA_NOREF) return;

    // Pega a função DIRETO da memória usando o nosso ID de cache
    lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, m_rgbMapRef);

    // Busca a função rgbMap global do script
    lua_getglobal(m_luaState, "rgbMap");
    if (!lua_isfunction(m_luaState, -1)) {
        lua_pop(m_luaState, 1);
        return;
    }

    // Empilha os argumentos na memória do C para o Lua ler
    lua_pushinteger(m_luaState, size.width());
    lua_pushinteger(m_luaState, size.height());
    lua_pushinteger(m_luaState, rgb);
    lua_pushinteger(m_luaState, step);

    // Chama a função (4 argumentos, 1 retorno esperado - a tabela do mapa)
    if (lua_pcall(m_luaState, 4, 1, 0) != LUA_OK) {
        qWarning() << "Lua rgbMap Error:" << lua_tostring(m_luaState, -1);
        lua_pop(m_luaState, 1);
        return;
    }

    // Processa o retorno: O Lua devolve uma tabela bidimensional (Array de Arrays)
    if (lua_istable(m_luaState, -1))
    {
        int ylen = lua_objlen(m_luaState, -1);
        map.resize(ylen);

        for (int y = 0; y < ylen && y < size.height(); y++)
        {
            lua_rawgeti(m_luaState, -1, y + 1); // Lua usa index-1
            if (lua_istable(m_luaState, -1))
            {
                int xlen = lua_objlen(m_luaState, -1);
                map[y].resize(xlen);

                for (int x = 0; x < xlen && x < size.width(); x++)
                {
                    lua_rawgeti(m_luaState, -1, x + 1);
                    map[y][x] = static_cast<uint>(lua_tointeger(m_luaState, -1));
                    lua_pop(m_luaState, 1); // remove o valor do X
                }
            }
            lua_pop(m_luaState, 1); // remove o valor do Y (a linha atual)
        }
    }
    
    // Limpa a tabela principal retornada da pilha de memória
    lua_pop(m_luaState, 1); 
}

void RGBLua::rgbMapSetColors(const QVector<uint>& colors)
{
    // Esta função só existe para a API v3 ou superior
    if (!m_luaState || m_apiVersion < 3)
        return;

    // Busca a função no escopo global do script
    lua_getglobal(m_luaState, "rgbMapSetColors");
    if (!lua_isfunction(m_luaState, -1)) {
        lua_pop(m_luaState, 1);
        return;
    }

    int accColors = acceptColors();
    int rawColorCount = colors.count();

    // Cria uma nova tabela vazia na memória do Lua
    lua_newtable(m_luaState);

    // Preenche a tabela Lua com as cores do QVector
    for (int i = 0; i < rawColorCount && i < accColors; i++)
    {
        lua_pushinteger(m_luaState, colors.at(i));
        // lua_rawseti insere o valor que acabamos de empilhar na tabela.
        // O índice -2 aponta para a tabela criada acima. O (i + 1) ajusta para o index-1 do Lua.
        lua_rawseti(m_luaState, -2, i + 1); 
    }

    // Executa a função passando 1 argumento (a tabela de cores) e esperando 0 retornos
    if (lua_pcall(m_luaState, 1, 0, 0) != LUA_OK) {
        qWarning() << "Lua rgbMapSetColors Error:" << lua_tostring(m_luaState, -1);
        lua_pop(m_luaState, 1);
    }
}

QVector<uint> RGBLua::rgbMapGetColors()
{
    QVector<uint> colArray;

    if (!m_luaState)
        return colArray;

    lua_getglobal(m_luaState, "rgbMapGetColors");
    if (!lua_isfunction(m_luaState, -1)) {
        lua_pop(m_luaState, 1);
        return colArray;
    }

    // Executa a função passando 0 argumentos e esperando 1 retorno (a tabela)
    if (lua_pcall(m_luaState, 0, 1, 0) != LUA_OK) {
        qWarning() << "Lua rgbMapGetColors Error:" << lua_tostring(m_luaState, -1);
        lua_pop(m_luaState, 1);
        return colArray;
    }

    // Verifica se o script Lua se comportou bem e nos devolveu uma tabela
    if (lua_istable(m_luaState, -1))
    {
        int len = lua_objlen(m_luaState, -1); // Pega o tamanho da array no Lua
        
        for (int i = 1; i <= len; i++) // Iteração baseada em 1 (Padrão Lua)
        {
            lua_rawgeti(m_luaState, -1, i); // Pega o valor na posição 'i'
            
            // Blindagem contra scripts mal escritos que possam retornar textos/nulos na array
            if (lua_isnumber(m_luaState, -1)) {
                colArray.append(static_cast<uint>(lua_tointeger(m_luaState, -1)));
            }
            
            lua_pop(m_luaState, 1); // Remove o valor atual da pilha e prepara para o próximo
        }
    }

    // Limpa a tabela principal retornada da memória
    lua_pop(m_luaState, 1); 
    
    return colArray;
}

bool RGBLua::loadXML(QXmlStreamReader& root)
{
    Q_UNUSED(root)
    return false;
}

bool RGBLua::saveXML(QXmlStreamWriter* doc) const
{
    Q_ASSERT(doc != nullptr);
    if (apiVersion() > 0 && name().isEmpty() == false)
    {
        doc->writeStartElement(KXMLQLCRGBAlgorithm);
        // Marcamos o tipo como RGBLua no XML do show para não conflitar
        doc->writeAttribute(KXMLQLCRGBAlgorithmType, "RGBLua"); 
        doc->writeCharacters(name());
        doc->writeEndElement();
        return true;
    }
    return false;
}

// --- SISTEMA DE PROPRIEDADES ---

QList<RGBScriptProperty> RGBLua::properties()
{
    return m_properties;
}

QHash<QString, QString> RGBLua::propertiesAsStrings()
{
    return m_propertyValues;
}

QString RGBLua::property(QString propertyName) const
{
    return m_propertyValues.value(propertyName);
}

bool RGBLua::setProperty(QString propertyName, QString value)
{
    // Atualiza o cache do C++
    m_propertyValues[propertyName] = value;

    // Injeta o valor instantaneamente na VM do Lua
    if (m_luaState)
    {
        bool isNumber;
        double numValue = value.toDouble(&isNumber);

        if (isNumber) {
            // Se for número (ex: tamanho, velocidade), empilha como float/double
            lua_pushnumber(m_luaState, numValue);
        } else {
            // Se for string (ex: textos, nomes), empilha como string
            lua_pushstring(m_luaState, value.toUtf8().constData());
        }

        // Define a variável global no script com o nome da propriedade
        lua_setglobal(m_luaState, propertyName.toUtf8().constData());
    }

    return true;
}

bool RGBLua::loadProperties()
{
    m_properties.clear();
    m_propertyValues.clear();

    if (!m_luaState) return false;

    // Busca a variável global "properties"
    lua_getglobal(m_luaState, "properties");
    
    // Se não existir ou não for uma tabela, o script não tem propriedades
    if (!lua_istable(m_luaState, -1)) {
        lua_pop(m_luaState, 1);
        return false;
    }

    // Como estamos no LuaJIT (5.1), usamos lua_objlen para pegar o tamanho do array
    int len = lua_objlen(m_luaState, -1);
    
    for (int i = 1; i <= len; i++) 
    {
        lua_rawgeti(m_luaState, -1, i); // Pega o item 'i' da lista
        
        if (lua_istable(m_luaState, -1)) 
        {
            RGBScriptProperty prop;

            // Extrai o nome da variável ("name")
            lua_getfield(m_luaState, -1, "name");
            if (lua_isstring(m_luaState, -1)) prop.m_name = QString::fromUtf8(lua_tostring(m_luaState, -1));
            lua_pop(m_luaState, 1);

            // Extrai o nome de exibição na interface ("display")
            lua_getfield(m_luaState, -1, "display");
            if (lua_isstring(m_luaState, -1)) prop.m_displayName = QString::fromUtf8(lua_tostring(m_luaState, -1));
            lua_pop(m_luaState, 1);

            // Extrai o tipo ("type")
            lua_getfield(m_luaState, -1, "type");
            QString typeStr;
            if (lua_isstring(m_luaState, -1)) typeStr = QString::fromUtf8(lua_tostring(m_luaState, -1));
            lua_pop(m_luaState, 1);

            // Validação de Tipos
            if (typeStr == "range") prop.m_type = RGBScriptProperty::Range;
            else if (typeStr == "list") prop.m_type = RGBScriptProperty::List;
            else if (typeStr == "float") prop.m_type = RGBScriptProperty::Float;
            else prop.m_type = RGBScriptProperty::String; // Padrão de segurança

            // Lógica específica para Sliders (Range)
            if (prop.m_type == RGBScriptProperty::Range) 
            {
                lua_getfield(m_luaState, -1, "min");
                if (lua_isnumber(m_luaState, -1)) prop.m_rangeMinValue = lua_tointeger(m_luaState, -1);
                lua_pop(m_luaState, 1);

                lua_getfield(m_luaState, -1, "max");
                if (lua_isnumber(m_luaState, -1)) prop.m_rangeMaxValue = lua_tointeger(m_luaState, -1);
                lua_pop(m_luaState, 1);
            }

            // Lógica específica para Dropdowns (List)
            if (prop.m_type == RGBScriptProperty::List) 
            {
                lua_getfield(m_luaState, -1, "values");
                if (lua_isstring(m_luaState, -1)) {
                    prop.m_listValues = QString::fromUtf8(lua_tostring(m_luaState, -1)).split(",");
                }
                lua_pop(m_luaState, 1);
            }

            // Extrai o valor padrão e já injeta no motor!
            lua_getfield(m_luaState, -1, "default");
            QString defaultVal;
            if (!lua_isnil(m_luaState, -1)) 
            {
                if (lua_isnumber(m_luaState, -1)) {
                    defaultVal = QString::number(lua_tonumber(m_luaState, -1));
                } else {
                    defaultVal = QString::fromUtf8(lua_tostring(m_luaState, -1));
                }
            }
            lua_pop(m_luaState, 1);

            // Adiciona a propriedade na lista do QLC+ se ela tiver um nome válido
            if (!prop.m_name.isEmpty()) {
                m_properties.append(prop);
                
                // Injeta na interface e na máquina virtual do Lua de uma vez só
                if (!defaultVal.isEmpty()) {
                    setProperty(prop.m_name, defaultVal);
                }
            }
        }
        lua_pop(m_luaState, 1); // Remove a tabela da propriedade atual da pilha
    }
    
    lua_pop(m_luaState, 1); // Remove a tabela global "properties" da pilha
    return true;
}