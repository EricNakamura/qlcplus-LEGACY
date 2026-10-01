/*
  Q Light Controller Plus
  patchrenumber.h

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

#ifndef PATCHRENUMBER_H
#define PATCHRENUMBER_H

#include <QDialog>
#include <QList>

#include "ui_patchrenumber.h"

class Fixture;
class Doc;

/** @addtogroup ui_fixtures
 * @{
 */

/**
 * Assign sequential DMX addresses (with an optional gap) to a list of
 * fixtures, showing a live preview before applying the changes.
 */
class PatchRenumber final : public QDialog, public Ui_PatchRenumber
{
    Q_OBJECT
    Q_DISABLE_COPY(PatchRenumber)

public:
    PatchRenumber(Doc* doc, const QList <quint32>& fixtures, QWidget* parent = 0);
    ~PatchRenumber();

protected slots:
    /** Rebuild the address preview according to the current options */
    void slotUpdatePreview();

    /** Apply the new addresses */
    void accept() override;

private:
    /** Get the fixtures in the order selected by the user */
    QList <quint32> orderedFixtures() const;

private:
    Doc* m_doc;
    QList <quint32> m_fixtures;
};

/** @} */

#endif // PATCHRENUMBER_H
