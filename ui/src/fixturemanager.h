/*
  Q Light Controller
  fixturemanager.h

  Copyright (c) Heikki Junnila

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

#ifndef FIXTUREMANAGER_H
#define FIXTUREMANAGER_H

#include <QWidget>
#include <QList>

#include "function.h"
#include "fixture.h"
#include "doc.h"

class QLCFixtureDefCache;
class FixtureGroupEditor;
class FixturePatchModel;
class FixtureTreeWidget;
class PatchGridWidget;
class QSortFilterProxyModel;
class QTreeWidgetItem;
class QTextBrowser;
class QModelIndex;
class QTreeWidget;
class QTableView;
class QTabWidget;
class QToolButton;
class OutputMap;
class QSplitter;
class QComboBox;
class QLineEdit;
class QLabel;
class QAction;
class QMenu;

/** @addtogroup ui_fixtures
 * @{
 */

#define KXMLQLCFixtureManager             QStringLiteral("FixtureManager")
#define KXMLQLCFixtureManagerSplitterSize QStringLiteral("SplitterSize")

class FixtureManager final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(FixtureManager)

    /********************************************************************
     * Initialization
     ********************************************************************/
public:
    FixtureManager(QWidget* parent, Doc* doc);
    ~FixtureManager();

    /** Get the singleton instance */
    static FixtureManager* instance();

private:
    /** The singleton FixtureManager instance */
    static FixtureManager* s_instance;

    /********************************************************************
     * Doc signal handlers
     ********************************************************************/
public slots:
    /** Callback for Doc::fixtureRemoved() signals */
    void slotFixtureRemoved(quint32 id);

    /** Callback for Doc::fixtureChanged() signals */
    void slotFixtureChanged(quint32 id);

    /** Callback for Doc::channelsGroupRemoved() signals */
    void slotChannelsGroupRemoved(quint32 id);

    /** Callback that listens to mode change signals */
    void slotModeChanged(Doc::Mode mode);

    /** Callback that listens to fixture group removals */
    void slotFixtureGroupRemoved(quint32 id);

    /** Callback that listens to fixture group modifications */
    void slotFixtureGroupChanged(quint32 id);

    /** Callback that listens to workspace loading */
    void slotDocLoaded();

private:
    Doc* m_doc;

    /********************************************************************
     * Data view
     ********************************************************************/
public:
    /** Update the list of fixtures */
    void updateView();

    /** Update the list of channels group */
    void updateChannelsGroupView();

    /** Update the patch grid summary label */
    void updatePatchSummary();

    /** Update icons on RDM view activation */
    void updateRDMView();

    /** Get the fixtures selected in the active view */
    QList <quint32> selectedFixtures() const;

    /** Select the given fixtures in the patch table */
    void selectFixtures(const QList <quint32>& ids);

private:
    /** Open a fixture selector to add new fixtures */
    void addFixture();

    /** Open a channels selector to add new channels group */
    void addChannelsGroup();

    /** Remove a previously created fixture */
    void removeFixture();

    /** Remove a previously created channels group */
    void removeChannelsGroup();

    /** Construct the list view and data view */
    void initDataView();

    /** Construct the patch matrix tab */
    void initPatchView();

    /** Construct the flat patch table tab */
    void initFixturesView();

    /** Handle single fixture selection */
    void fixtureSelected(quint32 id);

    /** Handle fixture group selection */
    void fixtureGroupSelected(FixtureGroup* grp);

    /** Create the text browser for displaying information */
    void createInfo();

    /** Show a temporary warning message at the bottom of the view */
    void showWarning(const QString& message);

    /** Get the fixture group nodes currently selected in the tree */
    QList <quint32> selectedGroups() const;

private slots:
    /** Callback for fixture list selection changes */
    void slotSelectionChanged();

    /** Callback for patch table selection changes */
    void slotTableSelectionChanged();

    /** Callback for patch table double clicks */
    void slotTableDoubleClicked(const QModelIndex& index);

    /** Callback for right mouse button clicks over the patch table */
    void slotTableContextMenuRequested(const QPoint& pos);

    /** Callback for channels group selection changes */
    void slotChannelsGroupSelectionChanged();

    /** Callback for mouse double clicks */
    void slotDoubleClicked(QTreeWidgetItem* item);

    /** Callback for channels group mouse double clicks */
    void slotChannelsGroupDoubleClicked(QTreeWidgetItem*);

    /** Callback for tab selection changes */
    void slotTabChanged(int index = 0);

    /** Callback for fixture tree item expand/collapse */
    void slotFixtureItemExpanded();

    void slotDisplayFixtureInfo(QString &info);

    /** Patch matrix slots */
    void slotGridFixtureClicked(quint32 id, Qt::KeyboardModifiers modifiers);
    void slotGridFixtureDoubleClicked(quint32 id);
    void slotGridContextMenuRequested(quint32 id, const QPoint& pos);
    void slotGridMoveRequested(quint32 id, quint32 universeAddress);
    void slotPatchUniverseChanged(int index);
    void slotPatchZoomIn();
    void slotPatchZoomOut();
    void slotPatchOrientationToggled(bool checked);
    void slotPatchExpandToggled(bool checked);
    void slotPatchSearchChanged(const QString& filter);
    void slotPatchOverlapDetected(quint32 id, const QList <quint32>& others);
    void slotPatchRenumber();

private:
    /** Select a fixture group */
    void selectGroup(quint32 id);

    /** Get a CSS style sheet & HTML header for fixture info */
    QString fixtureInfoStyleSheetHeader() const;

    /**
     * Get the fixture instance's status info for Fixture Manager
     *
     * @return A sort-of HTML-RTF-gibberish for Fixture Manager
     */
    QString fixtureInfo(const Fixture *fixture) const;

    /** Get a CSS style sheet & HTML header for channels group info */
    QString channelsGroupInfoStyleSheetHeader() const;

    /**
     * Get the channels group instance's status info for Fixture Manager
     *
     * @return A sort-of HTML-RTF-gibberish for Fixture Manager
     */
    QString channelsGroupInfo(const ChannelsGroup *channelsGroup) const;

private:
    QSplitter* m_splitter;
    FixtureTreeWidget* m_fixtures_tree;
    QTreeWidget* m_channel_groups_tree;
    QWidget* m_rdmManager;

    PatchGridWidget* m_patchGrid;
    QComboBox* m_universeCombo;
    QLabel* m_patchSummary;

    QTableView* m_patchTable;
    FixturePatchModel* m_patchModel;
    QSortFilterProxyModel* m_patchProxy;
    QLineEdit* m_patchSearch;

    QLabel* m_statusLabel;

    QTextBrowser* m_info;
    FixtureGroupEditor* m_groupEditor;
    int m_currentTabIndex;

    /********************************************************************
     * Menu & Toolbar & Actions
     ********************************************************************/
private:
    /** Construct actions for toolbar & context menu */
    void initActions();

    /** Update the contents of the group menu */
    void updateGroupMenu();

    /** Construct the toolbar */
    void initToolBar();

    /** Edit properties for the selected fixture */
    void editFixtureProperties();

    /** Edit properties for the selected channels group */
    void editChannelGroupProperties();

    /** Count the number of heads in the given list of fixtures */
    int headCount(const QList <quint32>& ids) const;

    QString createDialog(bool import);

private slots:
    void slotAdd();
    void slotAddRGBPanel();
    void slotRemove();
    void slotProperties();
    void slotFadeConfig();
    void slotRemap();
    void slotUnGroup();
    void slotGroupSelected(QAction* action);
    void slotMoveGroupUp();
    void slotMoveGroupDown();
    void slotImport();
    void slotExport();

    /** Callback for right mouse button clicks over a fixture item */
    void slotContextMenuRequested(const QPoint& pos);

private:
    QAction* m_addAction;
    QAction* m_addRGBAction;
    QAction* m_removeAction;
    QAction* m_propertiesAction;
    QAction* m_fadeConfigAction;
    QAction* m_remapAction;
    QAction* m_groupAction;
    QAction* m_unGroupAction;
    QAction* m_newGroupAction;

    QAction* m_moveUpAction;
    QAction* m_moveDownAction;

    QAction* m_importAction;
    QAction* m_exportAction;

    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QAction* m_orientAction;
    QAction* m_expandAction;
    QAction* m_renumberAction;

    QMenu* m_groupMenu;
};

/** @} */

#endif
