/*
  Q Light Controller Plus
  fixturepatchmodel_test.cpp

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

#include <QtTest>
#include <QList>

#include "fixturepatchmodel.h"
#include "patchgridwidget.h"
#include "qlcfixturemode.h"
#include "qlcfixturedef.h"
#include "fixture.h"
#include "doc.h"

#include "fixturepatchmodel_test.h"

void FixturePatchModel_Test::initTestCase()
{
    m_doc = new Doc(this);
}

void FixturePatchModel_Test::init()
{
    m_doc->clearContents();
}

void FixturePatchModel_Test::cleanupTestCase()
{
    delete m_doc;
}

void FixturePatchModel_Test::parsePatchString()
{
    quint32 universe = 0;
    quint32 address = 0;

    /* Plain address */
    QVERIFY(FixturePatchModel::parsePatchString("17", universe, address) == true);
    QVERIFY(universe == FixturePatchModel::NoUniverse);
    QVERIFY(address == 16);

    /* Address range: only the start address matters */
    QVERIFY(FixturePatchModel::parsePatchString("  25 - 30 ", universe, address) == true);
    QVERIFY(address == 24);

    /* Universe-qualified address */
    QVERIFY(FixturePatchModel::parsePatchString("2:17", universe, address) == true);
    QVERIFY(universe == 1);
    QVERIFY(address == 16);

    /* Invalid inputs */
    QVERIFY(FixturePatchModel::parsePatchString("0", universe, address) == false);
    QVERIFY(FixturePatchModel::parsePatchString("513", universe, address) == false);
    QVERIFY(FixturePatchModel::parsePatchString("abc", universe, address) == false);
    QVERIFY(FixturePatchModel::parsePatchString("", universe, address) == false);
}

void FixturePatchModel_Test::fixtureData()
{
    Fixture* f1 = new Fixture(m_doc);
    f1->setName("One");
    f1->setChannels(5);
    f1->setAddress(0);
    f1->setUniverse(0);
    QVERIFY(m_doc->addFixture(f1) == true);

    Fixture* f2 = new Fixture(m_doc);
    f2->setName("Two");
    f2->setChannels(3);
    f2->setAddress(2); // overlaps with f1
    f2->setUniverse(0);
    QVERIFY(m_doc->addFixture(f2) == true);

    FixturePatchModel model(m_doc);
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.fixtureId(0), quint32(0));
    QCOMPARE(model.rowForFixture(1), 1);

    /* Address display */
    QModelIndex addrIdx = model.index(0, FixturePatchModel::ColumnAddress);
    QCOMPARE(model.data(addrIdx, Qt::DisplayRole).toString(), QString("1 - 5"));
    QCOMPARE(model.data(addrIdx, Qt::EditRole).toString(), QString("1 - 5"));

    /* Name, universe and channels */
    QCOMPARE(model.data(model.index(1, FixturePatchModel::ColumnName),
                        Qt::DisplayRole).toString(), QString("Two"));
    QCOMPARE(model.data(model.index(0, FixturePatchModel::ColumnUniverse),
                        Qt::DisplayRole).toString(), QString("1"));
    QCOMPARE(model.data(model.index(1, FixturePatchModel::ColumnChannels),
                        Qt::DisplayRole).toUInt(), quint32(3));

    /* Sorting role returns the absolute address */
    QCOMPARE(model.data(addrIdx, FixturePatchModel::SortRole).toUInt(), quint32(0));

    /* Overlapping fixtures are reported */
    QCOMPARE(model.overlappingFixtures(0), QList <quint32> () << 1);
    QCOMPARE(model.overlappingFixtures(1), QList <quint32> () << 0);
}

void FixturePatchModel_Test::setAddress()
{
    Fixture* f1 = new Fixture(m_doc);
    f1->setName("One");
    f1->setChannels(5);
    f1->setAddress(0);
    f1->setUniverse(0);
    QVERIFY(m_doc->addFixture(f1) == true);

    Fixture* f2 = new Fixture(m_doc);
    f2->setName("Two");
    f2->setChannels(3);
    f2->setAddress(10);
    f2->setUniverse(0);
    QVERIFY(m_doc->addFixture(f2) == true);

    FixturePatchModel model(m_doc);
    QModelIndex idx = model.index(1, FixturePatchModel::ColumnAddress);

    /* Move the second fixture on top of the first one: allowed with overlap */
    QVERIFY(model.setData(idx, "1", Qt::EditRole) == true);
    QCOMPARE(f2->address(), quint32(0));
    QCOMPARE(model.overlappingFixtures(1).isEmpty(), false);

    /* Universe-qualified address */
    QVERIFY(model.setData(idx, "2:100", Qt::EditRole) == true);
    QCOMPARE(f2->universe(), quint32(1));
    QCOMPARE(f2->address(), quint32(99));

    /* Invalid address is rejected */
    QVERIFY(model.setData(idx, "bogus", Qt::EditRole) == false);
    QVERIFY(model.setData(idx, "700", Qt::EditRole) == false);
}

void FixturePatchModel_Test::patchGridGeometry()
{
    Fixture* f1 = new Fixture(m_doc);
    f1->setName("Moving Head");
    f1->setChannels(16);
    f1->setAddress(0);
    f1->setUniverse(0);
    QVERIFY(m_doc->addFixture(f1) == true);

    Fixture* f2 = new Fixture(m_doc);
    f2->setName("LED PAR");
    f2->setChannels(5);
    f2->setAddress(10);
    f2->setUniverse(0);
    QVERIFY(m_doc->addFixture(f2) == true);

    Fixture* f3 = new Fixture(m_doc);
    f3->setName("Overlapping dimmer");
    f3->setChannels(3);
    f3->setAddress(12);
    f3->setUniverse(0);
    QVERIFY(m_doc->addFixture(f3) == true);

    PatchGridWidget grid(m_doc);
    grid.resize(900, 480);
    grid.setUniverse(0);
    grid.refresh();

    /* The first cell starts after the rulers */
    QCOMPARE(grid.cellRect(0), QRect(28, 16, 21, 21));

    /* Cell hit test */
    QCOMPARE(grid.addressAt(grid.cellRect(0).center()), 0);
    QCOMPARE(grid.fixtureAt(grid.cellRect(0).center()), f1->id());

    /* The ruler areas are not channel cells */
    QCOMPARE(grid.addressAt(QPoint(5, 5)), -1);

    /* Overlapping channels report all their occupants, primary first */
    QCOMPARE(grid.fixturesAt(grid.cellRect(12).center()), QList <quint32> () << f1->id() << f2->id() << f3->id());
    QCOMPARE(grid.fixtureAt(grid.cellRect(12).center()), f1->id());

    /* Not overlapping channels report a single fixture */
    QCOMPARE(grid.fixturesAt(grid.cellRect(0).center()).count(), 1);

    /* Filling order: in row-major, address 16 is on the first row, column 17 */
    QCOMPARE(grid.cellRect(16), QRect(28 + 16 * 22, 16, 21, 21));

    /* In column-major, channels fill each column first: address 16
       starts the second column, on the first row */
    grid.setOrientation(PatchGridWidget::ColumnMajor);
    QCOMPARE(grid.cellRect(16), QRect(28 + 22, 16, 21, 21));
}

QTEST_MAIN(FixturePatchModel_Test)
