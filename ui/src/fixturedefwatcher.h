/*
  Q Light Controller
  fixturedefwatcher.h

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

#ifndef FIXTUREDEFWATCHER_H
#define FIXTUREDEFWATCHER_H

#include <QMap>
#include <QPair>
#include <QObject>

class QFileSystemWatcher;
class QTimer;
class Doc;

/** @addtogroup ui_fixtures
 * @{
 */

/**
 * Watches the user fixture definition directory and reloads definitions that
 * are added or modified while the application is running, so that QLC+ doesn't
 * need to be restarted to pick them up.
 *
 * Both a QFileSystemWatcher (for immediate reaction) and a periodic scan (for
 * platforms/watchers that miss events) are used.
 */
class FixtureDefWatcher final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(FixtureDefWatcher)

public:
    /**
     * Create a watcher for the Doc's fixture definition cache.
     *
     * @param parent The parent object that owns the watcher
     * @param doc The QLC engine instance
     */
    FixtureDefWatcher(QObject* parent, const Doc* doc);

    /** Destructor */
    ~FixtureDefWatcher();

    /** Get the singleton instance or NULL if it wasn't created yet */
    static FixtureDefWatcher* instance();

    /**
     * Scan the user definitions directory for added or modified definitions
     * and reload them right now.
     */
    void scan();

    /** Set the interval between two periodic scans, in milliseconds */
    void setInterval(int msec);

    /** Get the interval between two periodic scans, in milliseconds */
    int interval() const;

signals:
    /** One or more fixture definitions were added or modified */
    void definitionsChanged();

private slots:
    void slotDirectoryChanged(const QString& path);
    void slotFileChanged(const QString& path);
    void slotScanTimeout();

private:
    /** Load a definition file and reload/apply it to the definition cache */
    bool reloadDefinition(const QString& path);

    /** Add/remove the definition files to/from the file system watcher */
    void updateWatchedFiles(const QStringList& paths);

    static FixtureDefWatcher* s_instance;

    const Doc* m_doc;
    QFileSystemWatcher* m_watcher;
    QTimer* m_timer;
    QTimer* m_debounceTimer;
    QMap <QString, QPair <qint64, qint64> > m_snapshot;
    bool m_initialized;
};

/** @} */

#endif
