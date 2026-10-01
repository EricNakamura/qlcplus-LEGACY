/*
  Q Light Controller - Fixture Definition Editor
  fixturedefio.h

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

#ifndef FIXTUREDEFIO_H
#define FIXTUREDEFIO_H

#include <QString>

class QLCFixtureDef;

/** @addtogroup fixtureeditor Fixture Editor
 * @{
 */

#define KAllFilter  QStringLiteral("Fixture Definitions (*.qxf *.d4)")
#define KQXFFilter  QStringLiteral("QLC fixtures (*.qxf)")
#define KD4Filter   QStringLiteral("Avolites fixtures (*.d4)")

/**
 * Helper to load fixture definition files from disk. Shared by the standalone
 * fixture editor application and the fixture editor embedded in the QLC+ main
 * application.
 */
class FixtureDefIO
{
public:
    /**
     * Load a fixture definition from the given path. Both the QLC+ native
     * (.qxf) and the Avolites Diamond 4 (.d4) formats are supported.
     *
     * @param path The file to load
     * @param errorMsg An optional error message on failure
     * @return A newly-created QLCFixtureDef if successful, otherwise NULL.
     *         Ownership of the pointer is transferred to the caller.
     */
    static QLCFixtureDef* load(const QString& path, QString& errorMsg);

    /**
     * Load a QLC+ native fixture definition from $path.
     *
     * @param path The path to a .qxf file to load
     * @param errorMsg An optional error message on failure
     * @return A newly-created QLCFixtureDef if successful, otherwise NULL.
     *         Ownership of the pointer is transferred to the caller.
     */
    static QLCFixtureDef* loadQXF(const QString& path, QString& errorMsg);

    /**
     * Load an Avolites Diamond 4 fixture definition from $path.
     *
     * @param path The path to a .d4 file to load
     * @param errorMsg An optional error message on failure
     * @return A newly-created QLCFixtureDef if successful, otherwise NULL.
     *         Ownership of the pointer is transferred to the caller.
     */
    static QLCFixtureDef* loadD4(const QString& path, QString& errorMsg);

    /** Check whether the given path has a known fixture definition extension */
    static bool isFixtureDefinition(const QString& path);
};

/** @} */

#endif
