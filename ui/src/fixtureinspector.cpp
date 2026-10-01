/*
  Q Light Controller Plus
  fixtureinspector.cpp

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

#include <QHeaderView>
#include <QTableWidgetItem>
#include <QDebug>

#include "qlcfixturemode.h"
#include "qlcfixturedef.h"
#include "qlcchannel.h"
#include "fixture.h"
#include "doc.h"

#include "fixtureinspector.h"

#define COLUMN_INDEX    0
#define COLUMN_NAME     1
#define COLUMN_TYPE     2
#define COLUMN_DMX      3
#define COLUMN_FLAGS    4

FixtureInspector::FixtureInspector(Doc* doc, QWidget* parent)
    : QWidget(parent)
    , m_doc(doc)
    , m_fixtureId(Fixture::invalidId())
{
    Q_ASSERT(doc != NULL);

    setupUi(this);

    m_warningLabel->setStyleSheet("QLabel { background-color: #E0A030; color: black; padding: 3px; }");

    QStringList headers;
    headers << tr("#") << tr("Name") << tr("Type") << tr("DMX") << tr("Flags");
    m_channelsTable->setColumnCount(5);
    m_channelsTable->setHorizontalHeaderLabels(headers);
    m_channelsTable->verticalHeader()->setVisible(false);
    m_channelsTable->horizontalHeader()->setSectionResizeMode(COLUMN_NAME, QHeaderView::Stretch);

    connect(m_propertiesButton, SIGNAL(clicked()),
            this, SLOT(slotPropertiesClicked()));
    connect(m_detailsGroup, SIGNAL(toggled(bool)),
            this, SLOT(slotDetailsToggled(bool)));

    slotDetailsToggled(m_detailsGroup->isChecked());

    setFixture(Fixture::invalidId());
}

FixtureInspector::~FixtureInspector()
{
}

void FixtureInspector::setFixture(quint32 id)
{
    m_fixtureId = id;
    updateContents();
}

quint32 FixtureInspector::fixtureId() const
{
    return m_fixtureId;
}

void FixtureInspector::slotPropertiesClicked()
{
    if (m_fixtureId != Fixture::invalidId())
        emit propertiesRequested();
}

void FixtureInspector::slotDetailsToggled(bool checked)
{
    m_detailsLabel->setVisible(checked);
}

void FixtureInspector::updateContents()
{
    Fixture* fxi = m_doc->fixture(m_fixtureId);

    if (fxi == NULL)
    {
        m_iconLabel->clear();
        m_nameLabel->setText(QString());
        m_subtitleLabel->setText(QString());
        m_typeLabel->setText(QString());
        m_universeLabel->setText(QString());
        m_addressLabel->setText(QString());
        m_channelsLabel->setText(QString());
        m_headsLabel->setText(QString());
        m_warningLabel->setVisible(false);
        m_channelsTable->setRowCount(0);
        m_detailsLabel->setText(QString());
        return;
    }

    /********************************************************************
     * Header
     ********************************************************************/
    m_iconLabel->setPixmap(fxi->getIconFromType().pixmap(32, 32));
    m_nameLabel->setText(fxi->name());

    QString subtitle;
    const QLCFixtureDef* fixtureDef = fxi->fixtureDef();
    const QLCFixtureMode* fixtureMode = fxi->fixtureMode();

    if (fixtureDef != NULL && fixtureMode != NULL)
    {
        subtitle = QString("%1 %2 - %3")
                        .arg(fixtureDef->manufacturer())
                        .arg(fixtureDef->model())
                        .arg(fixtureMode->name());
    }

    m_subtitleLabel->setText(subtitle);

    /********************************************************************
     * Summary
     ********************************************************************/
    if (fixtureDef != NULL && fixtureMode != NULL)
        m_typeLabel->setText(fixtureDef->typeToString(fixtureDef->type()));
    else
        m_typeLabel->setText(fxi->typeString());

    QString universeName = m_doc->inputOutputMap()->getUniverseNameByID(fxi->universe());
    if (universeName.isEmpty() == true)
        m_universeLabel->setText(QString::number(fxi->universe() + 1));
    else
        m_universeLabel->setText(QString("%1 (%2)").arg(fxi->universe() + 1).arg(universeName));

    if (fxi->channels() > 1)
        m_addressLabel->setText(QString("%1 - %2").arg(fxi->address() + 1).arg(fxi->address() + fxi->channels()));
    else
        m_addressLabel->setText(QString::number(fxi->address() + 1));

    m_channelsLabel->setText(QString::number(fxi->channels()));
    m_headsLabel->setText(QString::number(fxi->heads()));

    updateWarning();
    updateChannels();
    updateDetails();
}

void FixtureInspector::updateWarning()
{
    Fixture* fxi = m_doc->fixture(m_fixtureId);
    if (fxi == NULL)
    {
        m_warningLabel->setVisible(false);
        return;
    }

    QList <quint32> overlapping;
    for (quint32 ch = 0; ch < fxi->channels(); ch++)
    {
        foreach (quint32 id, m_doc->fixturesAtAddress(fxi->universeAddress() + ch))
        {
            if (id != fxi->id() && overlapping.contains(id) == false)
                overlapping << id;
        }
    }

    if (overlapping.isEmpty() == true)
    {
        m_warningLabel->setVisible(false);
        return;
    }

    QStringList names;
    foreach (quint32 id, overlapping)
    {
        Fixture* other = m_doc->fixture(id);
        if (other != NULL)
            names << other->name();
    }

    m_warningLabel->setText(tr("Warning: overlapping with %1").arg(names.join(", ")));
    m_warningLabel->setVisible(true);
}

void FixtureInspector::updateChannels()
{
    Fixture* fxi = m_doc->fixture(m_fixtureId);
    m_channelsTable->setRowCount(0);

    if (fxi == NULL)
        return;

    QList <int> forcedHTP = fxi->forcedHTPChannels();
    QList <int> forcedLTP = fxi->forcedLTPChannels();

    m_channelsTable->setRowCount(int(fxi->channels()));

    for (quint32 ch = 0; ch < fxi->channels(); ch++)
    {
        const QLCChannel* channel = fxi->channel(ch);
        if (channel == NULL)
            continue;

        QTableWidgetItem* indexItem = new QTableWidgetItem(QString::number(ch + 1));
        indexItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_channelsTable->setItem(int(ch), COLUMN_INDEX, indexItem);

        m_channelsTable->setItem(int(ch), COLUMN_NAME, new QTableWidgetItem(channel->name()));

        QString type;
        if (channel->group() == QLCChannel::Intensity && channel->colour() != QLCChannel::NoColour)
            type = QLCChannel::colourToString(channel->colour());
        else
            type = QLCChannel::groupToString(channel->group());
        m_channelsTable->setItem(int(ch), COLUMN_TYPE, new QTableWidgetItem(type));

        quint32 absolute = fxi->universeAddress() + ch;
        QTableWidgetItem* dmxItem = new QTableWidgetItem(QString::number((absolute % 512) + 1));
        dmxItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_channelsTable->setItem(int(ch), COLUMN_DMX, dmxItem);

        QStringList flags;
        if (forcedHTP.contains(int(ch)) == true)
            flags << QString("HTP");
        else if (forcedLTP.contains(int(ch)) == true)
            flags << QString("LTP");

        if (fxi->channelCanFade(ch) == true)
            flags << tr("Fade");
        if (fxi->channelModifier(ch) != NULL)
            flags << tr("Modifier");

        m_channelsTable->setItem(int(ch), COLUMN_FLAGS, new QTableWidgetItem(flags.join(", ")));
    }

    m_channelsTable->resizeColumnsToContents();
}

void FixtureInspector::updateDetails()
{
    Fixture* fxi = m_doc->fixture(m_fixtureId);
    if (fxi == NULL)
    {
        m_detailsLabel->setText(QString());
        return;
    }

    QString info;
    const QLCFixtureDef* fixtureDef = fxi->fixtureDef();
    const QLCFixtureMode* fixtureMode = fxi->fixtureMode();

    if (fixtureMode != NULL)
    {
        const QLCPhysical physical = fixtureMode->physical();
        float kgLbs = 2.20462262;

        info += QString("<B>%1</B>: %2 x %3 x %4 mm<br>")
                    .arg(tr("Size"))
                    .arg(physical.width())
                    .arg(physical.height())
                    .arg(physical.depth());
        info += QString("<B>%1</B>: %2 kg (%3 lbs)<br>")
                    .arg(tr("Weight"))
                    .arg(physical.weight())
                    .arg(physical.weight() * kgLbs, 0, 'g', 4);
        info += QString("<B>%1</B>: %2 W<br>")
                    .arg(tr("Power consumption"))
                    .arg(physical.powerConsumption());
        info += QString("<B>%1</B>: %2<br>")
                    .arg(tr("DMX Connector"))
                    .arg(physical.dmxConnector());

        if (physical.bulbType().isEmpty() == false)
        {
            info += QString("<B>%1</B>: %2 (%3K, %4 lm)<br>")
                        .arg(tr("Bulb"))
                        .arg(physical.bulbType())
                        .arg(physical.bulbColourTemperature())
                        .arg(physical.bulbLumens());
        }

        if (physical.lensName().isEmpty() == false)
        {
            info += QString("<B>%1</B>: %2, %3&deg; - %4&deg;<br>")
                        .arg(tr("Lens"))
                        .arg(physical.lensName())
                        .arg(physical.lensDegreesMin())
                        .arg(physical.lensDegreesMax());
        }

        info += QString("<B>%1</B>: %2, %3&deg; / %4&deg;")
                    .arg(tr("Head(s)"))
                    .arg(physical.focusType())
                    .arg(physical.focusPanMax())
                    .arg(physical.focusTiltMax());
    }
    else
    {
        info += tr("No physical information available.");
    }

    /********************************************************************
     * DIP switch
     ********************************************************************/
    QString binaryStr = QString("%1").arg(fxi->address() + 1, 10, 2, QChar('0'));
    QString dipTable("<br><TABLE cellspacing='0'><TR><TD COLSPAN='33'><IMG SRC=\"" ":/ds_top.png\"></TD></TR>");
    dipTable += "<TR><TD><IMG SRC=\"" ":/ds_border.png\"></TD><TD><IMG SRC=\"" ":/ds_border.png\"></TD>";
    for (int i = 9; i >= 0; i--)
    {
        if (binaryStr.at(i) == '0')
            dipTable += "<TD COLSPAN='3'><IMG SRC=\"" ":/ds_off.png\"></TD>";
        else
            dipTable += "<TD COLSPAN='3'><IMG SRC=\"" ":/ds_on.png\"></TD>";
    }
    dipTable += "<TD><IMG SRC=\"" ":/ds_border.png\"></TD></TR>";
    dipTable += "<TR><TD COLSPAN='33'><IMG SRC=\"" ":/ds_bottom.png\"></TD></TR>";
    dipTable += "</TABLE>";

    info += QString("<br><B>%1</B>%2").arg(tr("Binary Address (DIP)")).arg(dipTable);

    if (fixtureDef != NULL && fixtureDef->author().isEmpty() == false)
    {
        info += QString("<br><br><I>%1 %2</I>")
                    .arg(tr("Fixture definition author:")).arg(fixtureDef->author());
    }

    m_detailsLabel->setText(info);
}
