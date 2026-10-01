/*
  Q Light Controller Plus
  fixturepatchmodel.h

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

#ifndef FIXTUREPATCHMODEL_H
#define FIXTUREPATCHMODEL_H

#include <QAbstractTableModel>
#include <QHash>
#include <QList>
#include <QVariant>

class Fixture;
class Doc;

/** @addtogroup ui_fixtures
 * @{
 */

/**
 * A flat, sortable table model exposing the DMX patch of the whole workspace:
 * one row per fixture, with an inline editable address column.
 *
 * The model is updated manually by calling update(), so it stays in sync
 * with the Doc signals handled by FixtureManager.
 */
class FixturePatchModel final : public QAbstractTableModel
{
    Q_OBJECT
    Q_DISABLE_COPY(FixturePatchModel)

public:
    enum Columns
    {
        ColumnID = 0,
        ColumnName,
        ColumnType,
        ColumnModel,
        ColumnUniverse,
        ColumnAddress,
        ColumnChannels,
        ColumnGroups,
        ColumnCount
    };

    enum Roles
    {
        SortRole = Qt::UserRole + 1,
        FixtureIdRole
    };

    /** Sentinel returned by parsePatchString() when the address string
     *  doesn't contain an explicit universe */
    static const quint32 NoUniverse = 0xFFFFFFFF;

    FixturePatchModel(Doc* doc, QObject* parent = NULL);
    ~FixturePatchModel();

    /** Rebuild the model from the current Doc contents */
    void update();

    /** Get the fixture ID of the given model row */
    quint32 fixtureId(int row) const;

    /** Get the model row of the given fixture ID. Returns -1 if not found */
    int rowForFixture(quint32 id) const;

    /** Rebuild the display string of a fixture's address, e.g. "17" or "17 - 32" */
    QString patchString(const Fixture* fxi) const;

    /** Parse a patch address string. Accepted formats are:
     *  "17", "17-32", "2:17" and "2:17-32", all 1-based.
     *  On success, $universe and $address are zero-based. */
    static bool parsePatchString(const QString& str, quint32& universe, quint32& address);

    /** Get the list of fixtures sharing at least a channel with the given fixture */
    QList <quint32> overlappingFixtures(quint32 id) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

signals:
    /** Emitted after an inline patch change created overlapping addresses */
    void patchOverlapDetected(quint32 id, const QList <quint32>& others);

private:
    QString groupNames(quint32 id) const;
    bool hasOverlap(quint32 id) const;

private:
    Doc* m_doc;
    QList <quint32> m_fixtures;
};

/** @} */

#endif // FIXTUREPATCHMODEL_H
