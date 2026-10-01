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

#include <QDebug>
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

    slotUpdatePreview();
}

PatchRenumber::~PatchRenumber()
{
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

void PatchRenumber::slotUpdatePreview()
{
    QStringList lines;
    quint32 universe = m_universeCombo->currentIndex();
    quint32 address = m_addressSpin->value() - 1;
    int gap = m_gapSpin->value();
    quint32 universeCount = m_doc->inputOutputMap()->universesCount();
    bool overflow = false;

    foreach (quint32 id, orderedFixtures())
    {
        Fixture* fxi = m_doc->fixture(id);
        if (fxi == NULL)
            continue;

        quint32 channels = fxi->channels();
        if (channels == 0 || channels > UNIVERSE_SIZE)
        {
            lines << tr("%1 : skipped (invalid channel count)").arg(fxi->name());
            continue;
        }

        if (address + channels > UNIVERSE_SIZE)
        {
            universe++;
            address = 0;
        }

        if (universe >= universeCount)
        {
            overflow = true;
            break;
        }

        lines << tr("%1 : Universe %2, %3 - %4")
                    .arg(fxi->name())
                    .arg(universe + 1)
                    .arg(address + 1)
                    .arg(address + channels);

        address += channels + gap;
    }

    if (overflow == true)
        lines << tr("Not enough universes to complete the operation!");

    m_preview->setPlainText(lines.join("\n"));
}

void PatchRenumber::accept()
{
    quint32 universe = m_universeCombo->currentIndex();
    quint32 address = m_addressSpin->value() - 1;
    int gap = m_gapSpin->value();
    quint32 universeCount = m_doc->inputOutputMap()->universesCount();

    foreach (quint32 id, orderedFixtures())
    {
        Fixture* fxi = m_doc->fixture(id);
        if (fxi == NULL)
            continue;

        quint32 channels = fxi->channels();
        if (channels == 0 || channels > UNIVERSE_SIZE)
            continue;

        if (address + channels > UNIVERSE_SIZE)
        {
            universe++;
            address = 0;
        }

        if (universe >= universeCount)
            break;

        fxi->setUniverse(universe);
        fxi->setAddress(address);

        address += channels + gap;
    }

    QDialog::accept();
}
