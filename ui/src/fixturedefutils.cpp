/*
  Q Light Controller
  fixturedefutils.cpp

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

#include <QMap>
#include <QDebug>

#include "qlcfixturedefcache.h"
#include "qlcfixturemode.h"
#include "qlcfixturedef.h"
#include "fixture.h"
#include "doc.h"

#include "fixturedefutils.h"

bool FixtureDefUtils::reloadAndApply(const Doc* doc, QLCFixtureDef* fixtureDef)
{
    if (doc == NULL || fixtureDef == NULL)
        return false;

    QString manufacturer = fixtureDef->manufacturer();
    QString model = fixtureDef->model();

    /* Remember which fixtures must be updated and their current mode */
    QMap <quint32, QString> fixturesToModes = collectFixturesForDefinition(doc, manufacturer, model);

    /* Reload or add the definition to the cache. When the definition already
       exists, the cache instance is updated in place, so the fixtures keeping
       a pointer to it stay valid. */
    if (doc->fixtureDefCache()->reloadOrAddFixtureDef(fixtureDef) == false)
        return false;

    QLCFixtureDef* cacheDef = doc->fixtureDefCache()->fixtureDef(manufacturer, model);
    if (cacheDef == NULL)
        return false;

    /* Re-apply the definition to the fixtures using it, so that channel
       properties, heads and values are recalculated */
    QMapIterator <quint32, QString> it(fixturesToModes);
    while (it.hasNext() == true)
    {
        it.next();

        Fixture* fixture = doc->fixture(it.key());
        if (fixture == NULL)
            continue;

        QLCFixtureMode* mode = cacheDef->mode(it.value());
        if (mode == NULL)
        {
            QList <QLCFixtureMode*> modes = cacheDef->modes();
            if (modes.isEmpty() == false)
                mode = modes.first();
            else
                qWarning() << Q_FUNC_INFO << "No mode found for fixture" << it.key();
        }

        if (mode != NULL)
            fixture->setFixtureDefinition(cacheDef, mode);
    }

    return true;
}

QMap <quint32, QString> FixtureDefUtils::collectFixturesForDefinition(
        const Doc* doc, const QString& manufacturer, const QString& model)
{
    QMap <quint32, QString> fixturesToModes;

    foreach (Fixture* fixture, doc->fixtures())
    {
        if (fixture == NULL || fixture->fixtureDef() == NULL ||
            fixture->fixtureMode() == NULL)
            continue;

        if (fixture->fixtureDef()->manufacturer() == manufacturer &&
            fixture->fixtureDef()->model() == model)
            fixturesToModes.insert(fixture->id(), fixture->fixtureMode()->name());
    }

    return fixturesToModes;
}
