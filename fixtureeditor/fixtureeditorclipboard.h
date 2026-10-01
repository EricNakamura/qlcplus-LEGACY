/*
  Q Light Controller - Fixture Definition Editor
  fixtureeditorclipboard.h

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

#ifndef FIXTUREEDITORCLIPBOARD_H
#define FIXTUREEDITORCLIPBOARD_H

#include <QObject>

class QLCChannel;

/** @addtogroup fixtureeditor Fixture Editor
 * @{
 */

/**
 * A shared channel clipboard for all fixture definition editors, whether
 * they run inside the standalone fixture editor application or are embedded
 * in the QLC+ main application.
 */
class FixtureEditorClipboard final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(FixtureEditorClipboard)

public:
    /** Get the singleton instance */
    static FixtureEditorClipboard* instance();

    /**
     * Replace the clipboard content with a copy of the given channel.
     * A NULL channel clears the clipboard.
     *
     * @param ch The channel to copy, or NULL to empty the clipboard
     */
    void setChannel(QLCChannel* ch);

    /** Get a pointer to the clipboard channel or NULL if the clipboard is empty */
    QLCChannel* channel() const;

signals:
    /** The clipboard content changed */
    void changed();

private:
    FixtureEditorClipboard();
    ~FixtureEditorClipboard();

    static FixtureEditorClipboard* s_instance;

    QLCChannel* m_channel;
};

/** @} */

#endif
