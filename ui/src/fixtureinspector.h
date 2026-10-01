/*
  Q Light Controller Plus
  fixtureinspector.h

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

#ifndef FIXTUREINSPECTOR_H
#define FIXTUREINSPECTOR_H

#include <QWidget>

#include "ui_fixtureinspector.h"

class Doc;

/** @addtogroup ui_fixtures
 * @{
 */

/**
 * A compact, sectioned view of a fixture: summary, channel mapping and
 * physical details. It replaces the old HTML information panel.
 */
class FixtureInspector final : public QWidget, public Ui_FixtureInspector
{
    Q_OBJECT
    Q_DISABLE_COPY(FixtureInspector)

public:
    FixtureInspector(Doc* doc, QWidget* parent = NULL);
    ~FixtureInspector();

    /** Display the given fixture. Use Fixture::invalidId() to clear the view */
    void setFixture(quint32 id);

    /** Get the currently displayed fixture ID */
    quint32 fixtureId() const;

    /** Refresh the view contents of the current fixture */
    void updateContents();

signals:
    /** The user asked to edit the fixture properties */
    void propertiesRequested();

private slots:
    void slotPropertiesClicked();

    /** Show/hide the details section contents */
    void slotDetailsToggled(bool checked);

private:
    void updateWarning();
    void updateChannels();
    void updateDetails();

private:
    Doc* m_doc;
    quint32 m_fixtureId;
};

/** @} */

#endif // FIXTUREINSPECTOR_H
