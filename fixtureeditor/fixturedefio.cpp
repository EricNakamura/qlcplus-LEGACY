/*
  Q Light Controller - Fixture Definition Editor
  fixturedefio.cpp

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <QFileInfo>
#include <QObject>

#include "avolitesd4parser.h"
#include "qlcfixturedef.h"
#include "qlcfile.h"

#include "fixturedefio.h"

bool FixtureDefIO::isFixtureDefinition(const QString& path)
{
    return path.endsWith(KExtFixture, Qt::CaseInsensitive) == true ||
           path.endsWith(KExtAvolitesFixture, Qt::CaseInsensitive) == true;
}

QLCFixtureDef* FixtureDefIO::load(const QString& path, QString& errorMsg)
{
    if (path.endsWith(KExtFixture, Qt::CaseInsensitive) == true)
        return loadQXF(path, errorMsg);
    else if (path.endsWith(KExtAvolitesFixture, Qt::CaseInsensitive) == true)
        return loadD4(path, errorMsg);

    errorMsg = QObject::tr("Unrecognized file extension: %1").arg(path);
    return NULL;
}

QLCFixtureDef* FixtureDefIO::loadQXF(const QString& path, QString& errorMsg)
{
    QLCFixtureDef* fixtureDef = new QLCFixtureDef;
    Q_ASSERT(fixtureDef != NULL);

    QFile::FileError error = fixtureDef->loadXML(path);
    if (error != QFile::NoError)
    {
        errorMsg = QLCFile::errorString(error);
        delete fixtureDef;
        return NULL;
    }

    /* Remember where this definition came from and mark it as fully loaded */
    fixtureDef->setDefinitionSourceFile(QFileInfo(path).absoluteFilePath());
    fixtureDef->setLoaded(true);

    return fixtureDef;
}

QLCFixtureDef* FixtureDefIO::loadD4(const QString& path, QString& errorMsg)
{
    QLCFixtureDef* fixtureDef = new QLCFixtureDef;
    Q_ASSERT(fixtureDef != NULL);

    AvolitesD4Parser parser;
    if (parser.loadXML(path, fixtureDef) == false)
    {
        errorMsg = parser.lastError();
        delete fixtureDef;
        return NULL;
    }

    fixtureDef->setDefinitionSourceFile(QFileInfo(path).absoluteFilePath());
    fixtureDef->setLoaded(true);

    return fixtureDef;
}
