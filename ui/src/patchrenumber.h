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
#include <QPair>

#include "ui_patchrenumber.h"

class Fixture;
class Doc;

/** @addtogroup ui_fixtures
 * @{
 */

/**
 * Assign sequential DMX addresses (with an optional gap) to a list of
 * fixtures, showing a live preview before applying the changes.
 * Addresses can be assigned from a given position, or by searching the
 * first free space available in the universes (packing).
 */
class PatchRenumber final : public QDialog, public Ui_PatchRenumber
{
    Q_OBJECT
    Q_DISABLE_COPY(PatchRenumber)

public:
    PatchRenumber(Doc* doc, const QList <quint32>& fixtures, QWidget* parent = 0);
    ~PatchRenumber();

    /** Sort the fixtures by their current address */
    void setOrderByAddress();

    /** Set the start position. $address is 1-based */
    void setStart(int universe, int address);

protected slots:
    /** Rebuild the address preview according to the current options */
    void slotUpdatePreview();

    /** Apply the new addresses */
    void accept() override;

private slots:
    /** Handle the "first free address" option */
    void slotFirstFreeToggled(bool checked);

private:
    /** Get the fixtures in the order selected by the user */
    QList <quint32> orderedFixtures() const;

    /**
     * Compute the new position of every fixture, in the selected order.
     * When $firstFree is true, the first free address range is searched,
     * skipping the addresses used by other fixtures. $ok is set to false
     * when there is not enough space in the available universes.
     */
    QList <QPair <quint32, quint32> > computePlacements(bool firstFree, bool& ok) const;

    /** Enable/disable the OK button according to the last computation */
    void updateAcceptButton(bool valid);

private:
    Doc* m_doc;
    QList <quint32> m_fixtures;
};

/** @} */

#endif // PATCHRENUMBER_H
