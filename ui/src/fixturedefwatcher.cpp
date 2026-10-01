/*
  Q Light Controller
  fixturedefwatcher.cpp

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

#include <QFileSystemWatcher>
#include <QFileInfo>
#include <QSettings>
#include <QTimer>
#include <QDebug>
#include <QDir>

#include "qlcfixturedefcache.h"
#include "qlcfixturedef.h"
#include "avolitesd4parser.h"
#include "qlcfile.h"

#include "fixturedefio.h"
#include "fixturedefutils.h"
#include "fixturedefwatcher.h"

#define SETTINGS_INTERVAL   "fixturedefwatcher/interval"
#define DEFAULT_INTERVAL    3000
#define DEBOUNCE_INTERVAL   500

FixtureDefWatcher* FixtureDefWatcher::s_instance = NULL;

FixtureDefWatcher::FixtureDefWatcher(QObject* parent, const Doc* doc)
    : QObject(parent)
    , m_doc(doc)
    , m_watcher(NULL)
    , m_timer(NULL)
    , m_debounceTimer(NULL)
    , m_initialized(false)
{
    Q_ASSERT(s_instance == NULL);
    s_instance = this;

    m_watcher = new QFileSystemWatcher(this);

    QDir userDir = QLCFixtureDefCache::userDefinitionDirectory();
    if (userDir.exists() == true)
        m_watcher->addPath(userDir.absolutePath());

    connect(m_watcher, SIGNAL(directoryChanged(const QString&)),
            this, SLOT(slotDirectoryChanged(const QString&)));
    connect(m_watcher, SIGNAL(fileChanged(const QString&)),
            this, SLOT(slotFileChanged(const QString&)));

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    connect(m_debounceTimer, SIGNAL(timeout()), this, SLOT(scan()));

    m_timer = new QTimer(this);
    QSettings settings;
    m_timer->setInterval(settings.value(SETTINGS_INTERVAL, DEFAULT_INTERVAL).toInt());
    connect(m_timer, SIGNAL(timeout()), this, SLOT(slotScanTimeout()));
    m_timer->start();

    /* Take the initial snapshot. Definitions were already loaded at
       application startup, so nothing is applied here. */
    scan();
}

FixtureDefWatcher::~FixtureDefWatcher()
{
    s_instance = NULL;
}

FixtureDefWatcher* FixtureDefWatcher::instance()
{
    return s_instance;
}

void FixtureDefWatcher::setInterval(int msec)
{
    if (msec < 500)
        msec = 500;

    QSettings settings;
    settings.setValue(SETTINGS_INTERVAL, msec);
    m_timer->setInterval(msec);
}

int FixtureDefWatcher::interval() const
{
    return m_timer->interval();
}

void FixtureDefWatcher::scan()
{
    QDir userDir = QLCFixtureDefCache::userDefinitionDirectory();
    QStringList filters;
    filters << QString("*%1").arg(KExtFixture);
    filters << QString("*%1").arg(KExtAvolitesFixture);

    QMap <QString, QPair <qint64, qint64> > snapshot;
    QStringList paths;

    QFileInfoList files = userDir.entryInfoList(filters, QDir::Files);
    foreach (const QFileInfo& info, files)
    {
        QString path = info.absoluteFilePath();
        snapshot.insert(path, QPair <qint64, qint64> (info.lastModified().toMSecsSinceEpoch(),
                                                      info.size()));
        paths << path;
    }

    if (m_initialized == false)
    {
        m_snapshot = snapshot;
        m_initialized = true;
        updateWatchedFiles(paths);
        return;
    }

    /* Find added or modified files. Deleted definitions are left in the
       cache, as removing them could invalidate patched fixtures. */
    QStringList changed;
    QMapIterator <QString, QPair <qint64, qint64> > it(snapshot);
    while (it.hasNext() == true)
    {
        it.next();
        if (m_snapshot.contains(it.key()) == false ||
            m_snapshot.value(it.key()) != it.value())
            changed << it.key();
    }

    m_snapshot = snapshot;
    updateWatchedFiles(paths);

    if (changed.isEmpty() == true)
        return;

    bool applied = false;
    foreach (QString path, changed)
    {
        if (reloadDefinition(path) == true)
            applied = true;
    }

    if (applied == true)
        emit definitionsChanged();
}

void FixtureDefWatcher::slotDirectoryChanged(const QString&)
{
    /* Coalesce bursts of file system events */
    m_debounceTimer->start(DEBOUNCE_INTERVAL);
}

void FixtureDefWatcher::slotFileChanged(const QString&)
{
    m_debounceTimer->start(DEBOUNCE_INTERVAL);
}

void FixtureDefWatcher::slotScanTimeout()
{
    scan();
}

bool FixtureDefWatcher::reloadDefinition(const QString& path)
{
    QString error;
    QLCFixtureDef* fixtureDef = FixtureDefIO::load(path, error);
    if (fixtureDef == NULL)
    {
        qWarning() << Q_FUNC_INFO << "Unable to load fixture definition"
                   << path << ":" << error;
        return false;
    }

    bool ok = FixtureDefUtils::reloadAndApply(m_doc, fixtureDef);
    delete fixtureDef;

    if (ok == true)
        qDebug() << Q_FUNC_INFO << "Reloaded fixture definition" << path;

    return ok;
}

void FixtureDefWatcher::updateWatchedFiles(const QStringList& paths)
{
    QStringList watched = m_watcher->files();

    QStringList toRemove;
    foreach (QString path, watched)
    {
        if (paths.contains(path) == false)
            toRemove << path;
    }

    QStringList toAdd;
    foreach (QString path, paths)
    {
        if (watched.contains(path) == false)
            toAdd << path;
    }

    if (toRemove.isEmpty() == false)
        m_watcher->removePaths(toRemove);
    if (toAdd.isEmpty() == false)
        m_watcher->addPaths(toAdd);
}
