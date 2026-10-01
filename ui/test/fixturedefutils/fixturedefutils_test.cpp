/*
  Q Light Controller
  fixturedefutils_test.cpp

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
#include <QDir>

#include "qlcfixturedefcache.h"
#include "qlcfixturemode.h"
#include "qlcfixturedef.h"
#include "qlcchannel.h"
#include "qlcfile.h"
#include "fixture.h"
#include "doc.h"

#include "fixturedefutils.h"
#include "fixturedefutils_test.h"

#include "../../../engine/test/common/resource_paths.h"

void FixtureDefUtils_Test::initTestCase()
{
    m_doc = new Doc(this);

    QDir dir(INTERNAL_FIXTUREDIR);
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList() << QString("*%1").arg(KExtFixture));
    QVERIFY(m_doc->fixtureDefCache()->loadMap(dir) == true);
}

void FixtureDefUtils_Test::cleanupTestCase()
{
    delete m_doc;
}

void FixtureDefUtils_Test::reloadInUseDefinition()
{
    QLCFixtureDef* def = m_doc->fixtureDefCache()->fixtureDef("Martin", "MAC300");
    QVERIFY(def != NULL);
    QVERIFY(def->isUser() == false);

    QLCFixtureMode* mode = def->modes().first();
    QVERIFY(mode != NULL);

    Fixture* fxi = new Fixture(m_doc);
    fxi->setName("Test scanner");
    fxi->setUniverse(0);
    fxi->setAddress(0);
    fxi->setFixtureDefinition(def, mode);
    QVERIFY(m_doc->addFixture(fxi) == true);
    m_doc->resetModified();

    const QString modeName = mode->name();
    const int channels = fxi->channels();

    /* Edit a copy of the definition and rename one of its channels */
    QLCFixtureDef* edited = new QLCFixtureDef(def);
    edited->channels().first()->setName("RenamedChannel");

    QVERIFY(FixtureDefUtils::reloadAndApply(m_doc, edited) == true);
    delete edited;

    /* The cache keeps the same definition instance, so the fixtures
       referencing it stay valid */
    QVERIFY(m_doc->fixtureDefCache()->fixtureDef("Martin", "MAC300") == def);
    QVERIFY(fxi->fixtureDef() == def);
    QVERIFY(fxi->fixtureMode()->name() == modeName);
    QVERIFY(fxi->channels() == channels);
    QVERIFY(def->channel("RenamedChannel") != NULL);

    /* Reloading a definition in use modifies the workspace */
    QVERIFY(m_doc->isModified() == true);

    m_doc->deleteFixture(fxi->id());
}

void FixtureDefUtils_Test::fallbackToFirstMode()
{
    QLCFixtureDef* def = m_doc->fixtureDefCache()->fixtureDef("Futurelight", "CY-200");
    QVERIFY(def != NULL);

    QLCFixtureMode* mode = def->modes().first();
    QVERIFY(mode != NULL);

    Fixture* fxi = new Fixture(m_doc);
    fxi->setName("Test color changer");
    fxi->setAddress(0);
    fxi->setFixtureDefinition(def, mode);
    QVERIFY(m_doc->addFixture(fxi) == true);

    /* Rename the mode in use, so that it cannot be found by name anymore */
    QLCFixtureDef* edited = new QLCFixtureDef(def);
    edited->modes().first()->setName("RenamedMode");

    QVERIFY(FixtureDefUtils::reloadAndApply(m_doc, edited) == true);
    delete edited;

    /* The fixture must fall back to the first mode of the new definition */
    QVERIFY(fxi->fixtureMode() != NULL);
    QCOMPARE(fxi->fixtureMode()->name(), QString("RenamedMode"));

    m_doc->deleteFixture(fxi->id());
}

void FixtureDefUtils_Test::addNewDefinition()
{
    QLCFixtureDef* def = new QLCFixtureDef();
    def->setManufacturer("UnitTest");
    def->setModel("NewFixture");

    QLCChannel* channel = new QLCChannel();
    channel->setName("Dimmer");
    channel->setGroup(QLCChannel::Intensity);
    def->addChannel(channel);

    QLCFixtureMode* mode = new QLCFixtureMode(def);
    mode->setName("Default");
    mode->insertChannel(channel, 0);
    QVERIFY(def->addMode(mode) == true);

    QVERIFY(m_doc->fixtureDefCache()->fixtureDef("UnitTest", "NewFixture") == NULL);
    QVERIFY(FixtureDefUtils::reloadAndApply(m_doc, def) == true);
    delete def;

    QLCFixtureDef* cached = m_doc->fixtureDefCache()->fixtureDef("UnitTest", "NewFixture");
    QVERIFY(cached != NULL);
    QVERIFY(cached->isUser() == true);
    QVERIFY(cached->channels().count() == 1);
    QVERIFY(cached->modes().count() == 1);
    QCOMPARE(cached->modes().first()->name(), QString("Default"));
}

QTEST_MAIN(FixtureDefUtils_Test)
