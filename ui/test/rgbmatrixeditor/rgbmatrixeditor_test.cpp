/*
  Q Light Controller Plus - Test Unit
  rgbmatrixeditor_test.cpp

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
#include <algorithm>

#include "qlcfixturemode.h"
#include "qlcfixturedef.h"
#include "rgbscriptscache.h"
#include "fixturegroup.h"
#include "rgbmatrix.h"
#include "fixture.h"
#include "rgbitem.h"
#include "qlcfile.h"
#include "doc.h"

#define private public
#define protected public
#include "rgbmatrixeditor.h"
#undef protected
#undef private

#include "../../../engine/test/common/resource_paths.h"
#include "rgbmatrixeditor_test.h"

void RGBMatrixEditor_Test::initTestCase()
{
    m_doc = new Doc(this);

    QDir fxiDir(INTERNAL_FIXTUREDIR);
    fxiDir.setFilter(QDir::Files);
    fxiDir.setNameFilters(QStringList() << QString("*%1").arg(KExtFixture));
    QVERIFY(m_doc->fixtureDefCache()->loadMap(fxiDir) == true);

    QLCFixtureDef* def = m_doc->fixtureDefCache()->fixtureDef("Stairville", "LED PAR56");
    QVERIFY(def != NULL);
    QLCFixtureMode* mode = def->modes().first();
    QVERIFY(mode != NULL);

    FixtureGroup* grp = new FixtureGroup(m_doc);
    grp->setName("Test Group");
    grp->setSize(QSize(5, 5));
    m_doc->addFixtureGroup(grp);

    for (int i = 0; i < 25; i++)
    {
        Fixture* fxi = new Fixture(m_doc);
        fxi->setFixtureDefinition(def, mode);
        fxi->setAddress(i * fxi->channels());
        m_doc->addFixture(fxi);
        grp->assignFixture(fxi->id());
    }

    QVERIFY(m_doc->rgbScriptsCache()->load(QDir(INTERNAL_SCRIPTDIR)));
    QVERIFY(m_doc->rgbScriptsCache()->names().size() != 0);
}

void RGBMatrixEditor_Test::cleanupTestCase()
{
    delete m_doc;
}

static QList<QRgb> previewColors(RGBMatrixEditor *editor)
{
    QList<QRgb> colors;
    QHashIterator<QLCPoint, RGBItem*> it(editor->m_previewHash);
    while (it.hasNext())
    {
        it.next();
        colors.append(it.value()->color());
    }
    std::sort(colors.begin(), colors.end());
    return colors;
}

void RGBMatrixEditor_Test::previewAnimation()
{
    RGBMatrix *mtx = new RGBMatrix(m_doc);
    m_doc->addFunction(mtx);
    mtx->setFixtureGroup(0);

    RGBLua *script = m_doc->rgbScriptsCache()->script("Varredura Lua");
    QVERIFY(script != NULL);
    QVERIFY(script->apiVersion() > 0);
    mtx->setAlgorithm(script);

    RGBMatrixEditor *editor = new RGBMatrixEditor(NULL, mtx, m_doc);
    QVERIFY(editor->m_previewTimer->isActive());
    QVERIFY(editor->m_previewHash.count() == 25);

    // First frame, continuous phase == 0: the sweep head is at the beginning
    QList<QRgb> phaseZero = previewColors(editor);
    QVERIFY(phaseZero.size() == 25);

    // The preview timer must animate the effect on wall time. After half a
    // step duration the sweep head must be in the middle of the group
    QTest::qWait(mtx->duration() / 2);
    QList<QRgb> phaseHalf = previewColors(editor);
    QVERIFY(phaseHalf != phaseZero);

    QTest::qWait(mtx->duration() / 2);
    QVERIFY(previewColors(editor) != phaseHalf);

    delete editor;
}

QTEST_MAIN(RGBMatrixEditor_Test)
