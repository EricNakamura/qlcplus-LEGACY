/*
  Q Light Controller
  fixturedefutils.h

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

#ifndef FIXTUREDEFUTILS_H
#define FIXTUREDEFUTILS_H

#include <QMap>
#include <QString>

class QLCFixtureDef;
class Doc;

/** @addtogroup ui_fixtures
 * @{
 */

/**
 * Helper functions to keep fixture definitions and patched fixtures in sync.
 */
class FixtureDefUtils
{
public:
    /**
     * Reload or add the given fixture definition to the Doc's definition
     * cache and re-apply it to all the fixtures currently using it.
     *
     * The definition mode of each patched fixture is preserved when possible;
     * if the mode doesn't exist anymore, the first mode of the new definition
     * is used.
     *
     * @param doc The QLC engine instance
     * @param fixtureDef The (edited) fixture definition to store
     * @return true if the definition could be stored, otherwise false
     */
    static bool reloadAndApply(const Doc* doc, QLCFixtureDef* fixtureDef);

private:
    /** Collect the fixtures using a definition and their current mode names */
    static QMap<quint32, QString> collectFixturesForDefinition(const Doc* doc,
                                                               const QString& manufacturer,
                                                               const QString& model);
};

/** @} */

#endif
