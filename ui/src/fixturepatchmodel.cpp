/*
  Q Light Controller Plus
  fixturepatchmodel.cpp

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

#include <QRegularExpression>
#include <QPainter>
#include <QPixmap>
#include <QDebug>

#include "qlcfixturemode.h"
#include "qlcfixturedef.h"
#include "fixturegroup.h"
#include "universe.h"
#include "fixture.h"
#include "patchcolor.h"
#include "doc.h"

#include "fixturepatchmodel.h"

FixturePatchModel::FixturePatchModel(Doc* doc, QObject* parent)
    : QAbstractTableModel(parent)
    , m_doc(doc)
{
    Q_ASSERT(doc != NULL);
    update();
}

FixturePatchModel::~FixturePatchModel()
{
}

void FixturePatchModel::update()
{
    beginResetModel();
    m_fixtures.clear();
    foreach (Fixture* fxi, m_doc->fixtures())
    {
        if (fxi != NULL)
            m_fixtures << fxi->id();
    }
    endResetModel();
}

quint32 FixturePatchModel::fixtureId(int row) const
{
    if (row < 0 || row >= m_fixtures.count())
        return Fixture::invalidId();

    return m_fixtures.at(row);
}

int FixturePatchModel::rowForFixture(quint32 id) const
{
    return m_fixtures.indexOf(id);
}

QString FixturePatchModel::patchString(const Fixture* fxi) const
{
    if (fxi == NULL)
        return QString();

    if (fxi->channels() > 1)
        return QString("%1 - %2").arg(fxi->address() + 1).arg(fxi->address() + fxi->channels());
    else
        return QString("%1").arg(fxi->address() + 1);
}

bool FixturePatchModel::parsePatchString(const QString& str, quint32& universe, quint32& address)
{
    QRegularExpression re("^\\s*(?:(\\d+)\\s*:)?\\s*(\\d+)(?:\\s*-\\s*(\\d+))?\\s*$");
    QRegularExpressionMatch match = re.match(str);
    if (match.hasMatch() == false)
        return false;

    quint32 uni = 0;
    if (match.captured(1).isEmpty() == false)
        uni = match.captured(1).toUInt();
    quint32 addr = match.captured(2).toUInt();

    if (addr < 1 || addr > 512)
        return false;

    if (match.captured(1).isEmpty() == false && uni < 1)
        return false;

    universe = match.captured(1).isEmpty() ? NoUniverse : uni - 1;
    address = addr - 1;

    return true;
}

QList <quint32> FixturePatchModel::overlappingFixtures(quint32 id) const
{
    QList <quint32> others;
    Fixture* fxi = m_doc->fixture(id);
    if (fxi == NULL)
        return others;

    quint32 start = fxi->universeAddress();
    for (quint32 i = 0; i < fxi->channels(); i++)
    {
        foreach (quint32 owner, m_doc->fixturesAtAddress(start + i))
        {
            if (owner != id && others.contains(owner) == false)
                others << owner;
        }
    }

    return others;
}

bool FixturePatchModel::hasOverlap(quint32 id) const
{
    Fixture* fxi = m_doc->fixture(id);
    if (fxi == NULL)
        return false;

    quint32 start = fxi->universeAddress();
    for (quint32 i = 0; i < fxi->channels(); i++)
    {
        if (m_doc->fixturesAtAddress(start + i).count() > 1)
            return true;
    }

    return false;
}

QString FixturePatchModel::groupNames(quint32 id) const
{
    QStringList names;
    foreach (FixtureGroup* grp, m_doc->fixtureGroups())
    {
        if (grp->fixtureList().contains(id) == true)
            names << grp->name();
    }

    return names.join(", ");
}

int FixturePatchModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid() == true)
        return 0;

    return m_fixtures.count();
}

int FixturePatchModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid() == true)
        return 0;

    return ColumnCount;
}

QVariant FixturePatchModel::data(const QModelIndex& index, int role) const
{
    if (index.isValid() == false)
        return QVariant();

    Fixture* fxi = m_doc->fixture(m_fixtures.at(index.row()));
    if (fxi == NULL)
        return QVariant();

    if (role == FixtureIdRole)
        return fxi->id();

    if (role == Qt::ToolTipRole)
    {
        QString tooltip = QString("<B>%1</B><BR>%2: %3<BR>%4: %5")
                            .arg(fxi->name())
                            .arg(tr("Universe")).arg(fxi->universe() + 1)
                            .arg(tr("Address")).arg(patchString(fxi));

        QList <quint32> others = overlappingFixtures(fxi->id());
        if (others.isEmpty() == false)
        {
            QStringList names;
            foreach (quint32 id, others)
            {
                Fixture* other = m_doc->fixture(id);
                if (other != NULL)
                    names << other->name();
            }
            tooltip += QString("<BR><B>%1</B>").arg(tr("Overlaps with: %1").arg(names.join(", ")));
        }
        return tooltip;
    }

    if (role == Qt::DecorationRole)
    {
        if (index.column() == ColumnID)
        {
            QPixmap chip(12, 12);
            chip.fill(Qt::transparent);
            QPainter p(&chip);
            p.setRenderHint(QPainter::Antialiasing);
            QColor col = PatchColor::fixtureColor(fxi->id());
            p.setBrush(col);
            p.setPen(PatchColor::darker(col));
            p.drawRoundedRect(1, 1, 10, 10, 2, 2);
            p.end();
            return chip;
        }
        else if (index.column() == ColumnName)
        {
            return fxi->getIconFromType();
        }
        return QVariant();
    }

    if (role == Qt::BackgroundRole)
    {
        if (index.column() == ColumnAddress && hasOverlap(fxi->id()) == true)
        {
            QColor warning(0xE0, 0x50, 0x50, 90);
            return warning;
        }
        return QVariant();
    }

    if (role == Qt::TextAlignmentRole)
    {
        if (index.column() == ColumnID || index.column() == ColumnUniverse ||
            index.column() == ColumnAddress || index.column() == ColumnChannels)
            return int(Qt::AlignRight | Qt::AlignVCenter);
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    }

    if (role == SortRole)
    {
        switch (index.column())
        {
            case ColumnID: return fxi->id();
            case ColumnName: return fxi->name();
            case ColumnType: return fxi->typeString();
            case ColumnModel: return fxi->fixtureDef() != NULL ? fxi->fixtureDef()->model() : QString(tr("Generic"));
            case ColumnUniverse: return fxi->universe();
            case ColumnAddress: return fxi->universeAddress();
            case ColumnChannels: return fxi->channels();
            case ColumnGroups: return groupNames(fxi->id());
            default: break;
        }
        return QVariant();
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {
            case ColumnID: return fxi->id();
            case ColumnName: return fxi->name();
            case ColumnType: return fxi->typeString();
            case ColumnModel:
                return fxi->fixtureDef() != NULL ? fxi->fixtureDef()->model() : QString(tr("Generic"));
            case ColumnUniverse: return QString("%1").arg(fxi->universe() + 1);
            case ColumnAddress: return patchString(fxi);
            case ColumnChannels: return fxi->channels();
            case ColumnGroups: return groupNames(fxi->id());
            default: break;
        }
    }

    return QVariant();
}

bool FixturePatchModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (index.isValid() == false || role != Qt::EditRole || index.column() != ColumnAddress)
        return false;

    quint32 universe = 0;
    quint32 address = 0;
    if (parsePatchString(value.toString(), universe, address) == false)
        return false;

    Fixture* fxi = m_doc->fixture(m_fixtures.at(index.row()));
    if (fxi == NULL)
        return false;

    if (universe == FixturePatchModel::NoUniverse)
        universe = fxi->universe();

    if (universe == fxi->universe() && address == fxi->address())
        return false;

    /* Non cross-universe fixtures cannot overflow their universe */
    if (fxi->crossUniverse() == false && fxi->channels() <= UNIVERSE_SIZE &&
        address + fxi->channels() > UNIVERSE_SIZE)
    {
        address = UNIVERSE_SIZE - fxi->channels();
    }

    fxi->setUniverse(universe);
    fxi->setAddress(address);

    /* Update all the rows, since conflicting addresses may change */
    emit dataChanged(createIndex(0, 0),
                     createIndex(rowCount() - 1, ColumnCount - 1));

    QList <quint32> others = overlappingFixtures(fxi->id());
    if (others.isEmpty() == false)
        emit patchOverlapDetected(fxi->id(), others);

    return true;
}

Qt::ItemFlags FixturePatchModel::flags(const QModelIndex& index) const
{
    if (index.isValid() == false)
        return Qt::NoItemFlags;

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (index.column() == ColumnAddress)
        flags |= Qt::ItemIsEditable;

    return flags;
}

QVariant FixturePatchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch (section)
    {
        case ColumnID: return tr("ID");
        case ColumnName: return tr("Name");
        case ColumnType: return tr("Type");
        case ColumnModel: return tr("Model");
        case ColumnUniverse: return tr("Universe");
        case ColumnAddress: return tr("Address");
        case ColumnChannels: return tr("Channels");
        case ColumnGroups: return tr("Groups");
        default: break;
    }

    return QVariant();
}
