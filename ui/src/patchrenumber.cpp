/*
  Q Light Controller Plus
  patchrenumber.cpp

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

#include <QDialogButtonBox>
#include <QPushButton>
#include <QCheckBox>
#include <QDebug>
#include <QSet>
#include <algorithm>

#include "universe.h"
#include "fixture.h"
#include "doc.h"

#include "patchrenumber.h"

#define KOrderSelection 0
#define KOrderAddress   1
#define KOrderName      2

PatchRenumber::PatchRenumber(Doc* doc, const QList <quint32>& fixtures, QWidget* parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_fixtures(fixtures)
{
    Q_ASSERT(doc != NULL);

    setupUi(this);

    m_universeCombo->addItems(m_doc->inputOutputMap()->universeNames());

    m_orderCombo->addItem(tr("Selection order"));
    m_orderCombo->addItem(tr("By address"));
    m_orderCombo->addItem(tr("By name"));

    m_infoLabel->setText(tr("Assign sequential addresses to %n selected fixture(s).", "", m_fixtures.count()));

    connect(m_addressSpin, SIGNAL(valueChanged(int)), this, SLOT(slotUpdatePreview()));
    connect(m_gapSpin, SIGNAL(valueChanged(int)), this, SLOT(slotUpdatePreview()));
    connect(m_universeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(slotUpdatePreview()));
    connect(m_orderCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(slotUpdatePreview()));
    connect(m_firstFreeCheck, SIGNAL(toggled(bool)), this, SLOT(slotFirstFreeToggled(bool)));

    slotUpdatePreview();
}

PatchRenumber::~PatchRenumber()
{
}

void PatchRenumber::setOrderByAddress()
{
    m_orderCombo->setCurrentIndex(KOrderAddress);
}

void PatchRenumber::setStart(int universe, int address)
{
    if (universe >= 0 && universe < m_universeCombo->count())
        m_universeCombo->setCurrentIndex(universe);

    if (m_firstFreeCheck->isChecked() == true)
        m_firstFreeCheck->setChecked(false);

    m_addressSpin->setValue(qBound(1, address, 512));
}

void PatchRenumber::slotFirstFreeToggled(bool checked)
{
    m_universeCombo->setEnabled(checked == false);
    m_addressSpin->setEnabled(checked == false);

    slotUpdatePreview();
}

void PatchRenumber::updateAcceptButton(bool valid)
{
    QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
    if (okButton != NULL)
        okButton->setEnabled(valid);
}

QList <quint32> PatchRenumber::orderedFixtures() const
{
    QList <quint32> list = m_fixtures;

    if (m_orderCombo->currentIndex() == KOrderAddress)
    {
        std::sort(list.begin(), list.end(), [this](quint32 a, quint32 b)
        {
            Fixture* fa = m_doc->fixture(a);
            Fixture* fb = m_doc->fixture(b);
            if (fa == NULL || fb == NULL)
                return a < b;
            if (fa->universeAddress() != fb->universeAddress())
                return fa->universeAddress() < fb->universeAddress();
            return a < b;
        });
    }
    else if (m_orderCombo->currentIndex() == KOrderName)
    {
        std::sort(list.begin(), list.end(), [this](quint32 a, quint32 b)
        {
            Fixture* fa = m_doc->fixture(a);
            Fixture* fb = m_doc->fixture(b);
            if (fa == NULL || fb == NULL)
                return a < b;
            return fa->name().localeAwareCompare(fb->name()) < 0;
        });
    }

    return list;
}

QList <QPair <quint32, quint32> > PatchRenumber::computePlacements(bool firstFree, bool& ok) const
{
    QList <QPair <quint32, quint32> > placements;

    /* Build the occupancy of all the fixtures that are NOT being moved */
    QSet <quint32> occupied;
    if (firstFree == true)
    {
        foreach (Fixture* fxi, m_doc->fixtures())
        {
            if (fxi == NULL || m_fixtures.contains(fxi->id()) == true)
                continue;

            for (quint32 c = 0; c < fxi->channels(); c++)
                occupied.insert(fxi->universeAddress() + c);
        }
    }

    quint32 universe = m_universeCombo->currentIndex();
    quint32 address = firstFree ? 0 : quint32(m_addressSpin->value() - 1);
    int gap = m_gapSpin->value();
    quint32 universeCount = m_doc->inputOutputMap()->universesCount();

    ok = true;

    foreach (quint32 id, orderedFixtures())
    {
        Fixture* fxi = m_doc->fixture(id);
        quint32 channels = fxi != NULL ? fxi->channels() : 0;

        if (fxi == NULL || channels == 0 || channels > UNIVERSE_SIZE)
        {
            placements << qMakePair(quint32(UINT_MAX), quint32(UINT_MAX));
            continue;
        }

        bool found = false;
        while (true)
        {
            if (universe >= universeCount)
                break;

            if (address + channels > UNIVERSE_SIZE)
            {
                universe++;
                address = 0;
                continue;
            }

            /* With a fixed start position, overlapping addresses are allowed.
               When searching free space, skip the occupied ones instead */
            if (firstFree == false)
            {
                found = true;
                break;
            }

            bool free = true;
            for (quint32 c = 0; c < channels; c++)
            {
                if (occupied.contains((universe * UNIVERSE_SIZE) + address + c) == true)
                {
                    free = false;
                    break;
                }
            }

            if (free == true)
            {
                found = true;
                break;
            }

            address++;
        }

        if (found == false)
        {
            ok = false;
            placements << qMakePair(quint32(UINT_MAX), quint32(UINT_MAX));
            break;
        }

        placements << qMakePair(universe, address);

        for (quint32 c = 0; c < channels; c++)
            occupied.insert((universe * UNIVERSE_SIZE) + address + c);

        address += channels + gap;
    }

    return placements;
}

void PatchRenumber::slotUpdatePreview()
{
    bool ok = true;
    bool firstFree = m_firstFreeCheck->isChecked();
    QList <QPair <quint32, quint32> > placements = computePlacements(firstFree, ok);
    QList <quint32> order = orderedFixtures();
    QStringList lines;

    for (int i = 0; i < placements.count() && i < order.count(); i++)
    {
        Fixture* fxi = m_doc->fixture(order.at(i));
        if (fxi == NULL)
            continue;

        quint32 universe = placements.at(i).first;
        quint32 address = placements.at(i).second;

        if (universe == UINT_MAX)
        {
            lines << tr("%1 : skipped (invalid channel count)").arg(fxi->name());
            continue;
        }

        lines << tr("%1 : Universe %2, %3 - %4")
                    .arg(fxi->name())
                    .arg(universe + 1)
                    .arg(address + 1)
                    .arg(address + fxi->channels());
    }

    if (ok == false)
    {
        lines << tr("Not enough space in the available universes to complete the operation!");
        updateAcceptButton(false);
    }
    else
    {
        updateAcceptButton(true);
    }

    m_preview->setPlainText(lines.join("\n"));
}

void PatchRenumber::accept()
{
    bool ok = true;
    QList <QPair <quint32, quint32> > placements = computePlacements(m_firstFreeCheck->isChecked(), ok);

    if (ok == false)
        return;

    QList <quint32> order = orderedFixtures();

    for (int i = 0; i < placements.count() && i < order.count(); i++)
    {
        Fixture* fxi = m_doc->fixture(order.at(i));
        if (fxi == NULL)
            continue;

        quint32 universe = placements.at(i).first;
        quint32 address = placements.at(i).second;

        if (universe == UINT_MAX)
            continue;

        fxi->setUniverse(universe);
        fxi->setAddress(address);
    }

    QDialog::accept();
}
