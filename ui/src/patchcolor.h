/*
  Q Light Controller Plus
  patchcolor.h

  Copyright (c) Massimo Callegari

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

#ifndef PATCHCOLOR_H
#define PATCHCOLOR_H

#include <QColor>
#include <QList>
#include <QString>

/** @addtogroup ui_fixtures
 * @{
 */

/**
 * Utility to get a stable colour for a fixture across the patch views.
 * The same fixture ID always maps to the same colour, so the patch table,
 * the patch matrix and the fixture inspector can visually match.
 */
class PatchColor
{
public:
    /** Get the colour associated to the given fixture ID */
    static QColor fixtureColor(quint32 id)
    {
        static QList <QColor> palette;
        if (palette.isEmpty() == true)
        {
            // Medium-saturation colours readable on both light and dark themes
            palette << QColor(0x3F, 0x8F, 0xDD) // blue
                    << QColor(0xE0, 0x7B, 0x39) // orange
                    << QColor(0x57, 0xA9, 0x5B) // green
                    << QColor(0xC5, 0x50, 0x5A) // red
                    << QColor(0x8E, 0x6B, 0xC4) // purple
                    << QColor(0x2F, 0xA6, 0xA6) // teal
                    << QColor(0xC2, 0x9B, 0x3A) // dark yellow
                    << QColor(0xB0, 0x62, 0xA0) // magenta
                    << QColor(0x6B, 0x8E, 0x3F) // olive
                    << QColor(0x50, 0x6C, 0xA8) // indigo
                    << QColor(0xA0, 0x5A, 0x3F) // brown
                    << QColor(0x5F, 0x9B, 0xC9); // light blue
        }

        return palette.at(int(id % quint32(palette.size())));
    }

    /** Get a darker variant of the given colour, for borders/text */
    static QColor darker(const QColor& color)
    {
        return color.darker(160);
    }
};

/** @} */

#endif // PATCHCOLOR_H
