/*
  Q Light Controller - Fixture Definition Editor
  fixtureeditorclipboard.cpp

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

#include "fixtureeditorclipboard.h"
#include "qlcchannel.h"

FixtureEditorClipboard* FixtureEditorClipboard::s_instance = NULL;

FixtureEditorClipboard::FixtureEditorClipboard()
    : QObject(NULL)
    , m_channel(NULL)
{
}

FixtureEditorClipboard::~FixtureEditorClipboard()
{
    if (m_channel != NULL)
        delete m_channel;
    m_channel = NULL;
}

FixtureEditorClipboard* FixtureEditorClipboard::instance()
{
    if (s_instance == NULL)
        s_instance = new FixtureEditorClipboard();
    return s_instance;
}

void FixtureEditorClipboard::setChannel(QLCChannel* ch)
{
    if (m_channel != NULL)
        delete m_channel;
    m_channel = NULL;

    if (ch != NULL)
        m_channel = ch->createCopy();

    emit changed();
}

QLCChannel* FixtureEditorClipboard::channel() const
{
    return m_channel;
}
