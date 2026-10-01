/*
  Q Light Controller
  fixtureeditordialog_test.cpp

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
#include <QTemporaryDir>
#include <QDir>

#include "qlcfixturedefcache.h"
#include "qlcfixturemode.h"
#include "qlcfixturedef.h"
#include "qlcchannel.h"
#include "qlcfile.h"
#include "doc.h"

#include "fixturedefio.h"
#include "fixtureeditor.h"
#include "fixtureeditordialog.h"
#include "fixtureeditordialog_test.h"

#include "../../../engine/test/common/resource_paths.h"

void FixtureEditorDialog_Test::initTestCase()
{
    m_doc = new Doc(this);

    QDir dir(INTERNAL_FIXTUREDIR);
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList() << QString("*%1").arg(KExtFixture));
    QVERIFY(m_doc->fixtureDefCache()->loadMap(dir) == true);
}

void FixtureEditorDialog_Test::cleanupTestCase()
{
    delete m_doc;
}

void FixtureEditorDialog_Test::saveUpdatesCacheAndFile()
{
    QTemporaryDir dir(QDir::currentPath() + "/fixtureeditordialog_test_XXXXXX");
    QVERIFY(dir.isValid() == true);
    QString path = dir.path() + "/TestUnit-TestModel.qxf";

    /* Create a fixture definition file on disk */
    QLCFixtureDef* def = new QLCFixtureDef();
    def->setManufacturer("TestUnit");
    def->setModel("TestModel");

    QLCChannel* channel = new QLCChannel();
    channel->setName("Dimmer");
    channel->setGroup(QLCChannel::Intensity);
    def->addChannel(channel);

    QLCFixtureMode* mode = new QLCFixtureMode(def);
    mode->setName("Default");
    mode->insertChannel(channel, 0);
    QVERIFY(def->addMode(mode) == true);
    QCOMPARE(def->saveXML(path), QFile::NoError);
    delete def;

    /* Open the definition in the embedded editor */
    QString error;
    QLCFixtureDef* loaded = FixtureDefIO::load(path, error);
    QVERIFY(loaded != NULL);

    FixtureEditorDialog dialog(NULL, m_doc, loaded, path);
    QLCFixtureEditor* editor = dialog.findChild<QLCFixtureEditor*>();
    QVERIFY(editor != NULL);
    QVERIFY(dialog.fixtureDefinition() == loaded);

    QSignalSpy spy(&dialog, SIGNAL(definitionSaved(QLCFixtureDef*)));

    /* Edit and save the definition */
    dialog.fixtureDefinition()->channels().first()->setName("RenamedDimmer");
    QVERIFY(editor->save() == true);
    QCOMPARE(spy.count(), 1);

    /* The cache must contain the reloaded definition as a user definition */
    QLCFixtureDef* cached = m_doc->fixtureDefCache()->fixtureDef("TestUnit", "TestModel");
    QVERIFY(cached != NULL);
    QVERIFY(cached->isUser() == true);
    QVERIFY(cached->channel("RenamedDimmer") != NULL);

    /* The file on disk must have been updated too */
    QLCFixtureDef* saved = FixtureDefIO::load(path, error);
    QVERIFY(saved != NULL);
    QVERIFY(saved->channel("RenamedDimmer") != NULL);
    delete saved;
}

QTEST_MAIN(FixtureEditorDialog_Test)
