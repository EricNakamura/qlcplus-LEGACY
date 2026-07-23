/*
  Q Light Controller Plus
  rgblua.h

  Copyright (c) Seu Nome / Eric Nakamura LEGACY Fork
*/

#ifndef RGBLUA_H
#define RGBLUA_H

#include <QString>
#include <QVector>
#include <QHash>
#include <QSize>

// Inclusão das bibliotecas do Lua(JIT) no modo C puro
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include "rgbscriptproperty.h"
#include "rgbalgorithm.h"

class Doc;

class RGBLua : public RGBAlgorithm
{
public:
    RGBLua(Doc* doc);
    RGBLua(const RGBLua& s);
    ~RGBLua();

    /** Operador de atribuição e cópia */
    RGBLua& operator=(const RGBLua& s);
    bool operator==(const RGBLua& s) const;

    /** Clona a instância (necessário pelo RGBAlgorithm) */
    RGBAlgorithm* clone() const override;

    /** Carrega o arquivo .lua do disco e inicializa a engine */
    bool load(const QString& fileName);
    QString fileName() const;

    /** Funções base do Algoritmo RGB */
    QString name() const override;
    QString author() const override;
    int apiVersion() const override;
    Type type() const override;
    int acceptColors() const override;

    /** Funções de processamento de cor e mapa */
    int rgbMapStepCount(const QSize& size) override;
    void rgbMap(const QSize& size, uint rgb, int step, RGBMap& map) override;

    void rgbMapSetColors(const QVector<uint>& colors) override;
    QVector<uint> rgbMapGetColors() override;

    /** Serialização XML (salvar no arquivo do show) */
    bool loadXML(QXmlStreamReader& root) override;
    bool saveXML(QXmlStreamWriter* doc) const override;

private:
    /** Inicializa e limpa a máquina virtual do Lua */
    void initEngine();
    void cleanupEngine();
    bool evaluate();

private:
    lua_State* m_luaState; // O ponteiro direto para a Engine do LuaJIT
    QString m_fileName;
    QString m_scriptContent;
    QString m_contents;
    int m_rgbMapRef;
    int m_rgbMapStepCountRef;
    int m_rgbMapSetColorsRef;
    int m_rgbMapGetColorsRef;
    
    // Cache de propriedades lidas do script para evitar chamadas pesadas repetidas
    QString m_name;
    QString m_author;
    int m_apiVersion;

public:
    /** Return a list of the loaded script properties */
    QList<RGBScriptProperty> properties();

    /** Return properties as strings */
    QHash<QString, QString> propertiesAsStrings();

    /** Set a property to the given value */
    bool setProperty(QString propertyName, QString value);

    /** Read the value of the property with the given name */
    QString property(QString propertyName) const;

private:
    /** Load the script properties if any is available */
    bool loadProperties();

private:
    QList<RGBScriptProperty> m_properties;
    QHash<QString, QString> m_propertyValues;
};

#endif // RGBLUA_H