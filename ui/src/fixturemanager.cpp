/*
  Q Light Controller
  fixturemanager.cpp

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

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QTreeWidgetItem>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QScrollArea>
#include <QMessageBox>
#include <QToolButton>
#include <QFileDialog>
#include <QTabWidget>
#include <QSplitter>
#include <QToolBar>
#include <QTableView>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QItemSelectionModel>
#include <QAbstractItemView>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QAction>
#include <QString>
#include <QDebug>
#include <QIcon>
#include <QMenu>
#include <QtGui>

#include "qlcfixturemode.h"
#include "qlcfixturedef.h"
#include "qlcchannel.h"
#include "qlcfile.h"

#include "fixturepatchmodel.h"
#include "patchgridwidget.h"
#include "patchrenumber.h"
#include "createfixturegroup.h"
#include "fixturegroupeditor.h"
#include "fixturetreewidget.h"
#include "channelsselection.h"
#include "addchannelsgroup.h"
#include "fixturemanager.h"
#include "fixtureremap.h"
#include "addrgbpanel.h"
#include "addfixture.h"
#include "rdmmanager.h"
#include "universe.h"
#include "fixture.h"
#include "apputil.h"
#include "doc.h"

#define SETTINGS_SPLITTER "fixturemanager/splitterstate"

// List view column numbers
#define KColumnName     0
#define KColumnChannels 1
#define KColumnAddress  2

// Tab indexes
#define KPatchTab       0
#define KFixturesTab    1
#define KGroupsTab      2
#define KChannelsTab    3

FixtureManager* FixtureManager::s_instance = NULL;

/*****************************************************************************
 * Initialization
 *****************************************************************************/

FixtureManager::FixtureManager(QWidget* parent, Doc* doc)
    : QWidget(parent)
    , m_doc(doc)
    , m_splitter(NULL)
    , m_fixtures_tree(NULL)
    , m_channel_groups_tree(NULL)
    , m_rdmManager(NULL)
    , m_patchGrid(NULL)
    , m_universeCombo(NULL)
    , m_patchSummary(NULL)
    , m_patchTable(NULL)
    , m_patchModel(NULL)
    , m_patchProxy(NULL)
    , m_patchSearch(NULL)
    , m_statusLabel(NULL)
    , m_info(NULL)
    , m_groupEditor(NULL)
    , m_currentTabIndex(KPatchTab)
    , m_addAction(NULL)
    , m_addRGBAction(NULL)
    , m_removeAction(NULL)
    , m_propertiesAction(NULL)
    , m_fadeConfigAction(NULL)
    , m_remapAction(NULL)
    , m_groupAction(NULL)
    , m_unGroupAction(NULL)
    , m_newGroupAction(NULL)
    , m_moveUpAction(NULL)
    , m_moveDownAction(NULL)
    , m_importAction(NULL)
    , m_exportAction(NULL)
    , m_zoomInAction(NULL)
    , m_zoomOutAction(NULL)
    , m_orientAction(NULL)
    , m_expandAction(NULL)
    , m_renumberAction(NULL)
    , m_groupMenu(NULL)
{
    Q_ASSERT(s_instance == NULL);
    s_instance = this;

    Q_ASSERT(doc != NULL);

    new QVBoxLayout(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setSpacing(0);

    initActions();
    initToolBar();
    initDataView();
    updateView();
    updateChannelsGroupView();
    slotTabChanged(m_currentTabIndex);

    QTreeWidgetItem* grpItem = m_fixtures_tree->topLevelItem(0);
    if (grpItem != NULL)
        grpItem->setExpanded(true);

    /* Connect fixture list change signals from the new document object */
    connect(m_doc, SIGNAL(fixtureRemoved(quint32)),
            this, SLOT(slotFixtureRemoved(quint32)));

    connect(m_doc, SIGNAL(fixtureChanged(quint32)),
            this, SLOT(slotFixtureChanged(quint32)));

    connect(m_doc, SIGNAL(channelsGroupRemoved(quint32)),
            this, SLOT(slotChannelsGroupRemoved(quint32)));

    connect(m_doc, SIGNAL(modeChanged(Doc::Mode)),
            this, SLOT(slotModeChanged(Doc::Mode)));

    connect(m_doc, SIGNAL(fixtureGroupRemoved(quint32)),
            this, SLOT(slotFixtureGroupRemoved(quint32)));

    connect(m_doc, SIGNAL(fixtureGroupChanged(quint32)),
            this, SLOT(slotFixtureGroupChanged(quint32)));

    connect(m_doc, SIGNAL(loaded()),
            this, SLOT(slotDocLoaded()));

    slotModeChanged(m_doc->mode());

    QSettings settings;
    QVariant var = settings.value(SETTINGS_SPLITTER);
    if (var.isValid() == true)
        m_splitter->restoreState(var.toByteArray());
    else
        m_splitter->setSizes(QList <int> () << int(this->width() / 2) << int(this->width() / 2));
}

FixtureManager::~FixtureManager()
{
    QSettings settings;
    settings.setValue(SETTINGS_SPLITTER, m_splitter->saveState());
    FixtureManager::s_instance = NULL;

    s_instance = NULL;
}

FixtureManager* FixtureManager::instance()
{
    return s_instance;
}

/*****************************************************************************
 * Doc signal handlers
 *****************************************************************************/

void FixtureManager::slotFixtureRemoved(quint32 id)
{
    QList<QTreeWidgetItem*> groupsToDelete;

    for (int i = 0; i < m_fixtures_tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem* grpItem = m_fixtures_tree->topLevelItem(i);
        Q_ASSERT(grpItem != NULL);
        for (int j = 0; j < grpItem->childCount(); j++)
        {
            QTreeWidgetItem* fxiItem = grpItem->child(j);
            Q_ASSERT(fxiItem != NULL);
            QVariant var = fxiItem->data(KColumnName, PROP_ID);
            if (var.isValid() == true && var.toUInt() == id)
            {
                delete fxiItem;
                break;
            }
        }
        if (grpItem->childCount() == 0)
            groupsToDelete << grpItem;
    }
    foreach (QTreeWidgetItem* groupToDelete, groupsToDelete)
    {
        QVariant var = groupToDelete->data(KColumnName, PROP_GROUP);
        // If the group is a fixture group, delete it from doc.
        // If not, it is a universe, just "hide" it from the ui.
        if (var.isValid() == true)
            m_doc->deleteFixtureGroup(groupToDelete->data(KColumnName, PROP_GROUP).toUInt());
        else
            delete groupToDelete;
    }

    if (m_patchModel != NULL)
        m_patchModel->update();

    if (m_patchGrid != NULL)
        m_patchGrid->refresh();

    updatePatchSummary();
    slotModeChanged(m_doc->mode());
}

void FixtureManager::slotFixtureChanged(quint32 id)
{
    Q_UNUSED(id)

    /* Keep the patch views in sync with changes coming from the engine
       (addresses edited in the table, dragged in the matrix, etc.) */
    if (m_patchGrid != NULL)
        m_patchGrid->refresh();

    if (m_patchTable != NULL)
        m_patchTable->viewport()->update();

    updatePatchSummary();
}

void FixtureManager::slotChannelsGroupRemoved(quint32 id)
{
    qDebug() << "Channel group removed: " << id;
    for (int i = 0; i < m_channel_groups_tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem* grpItem = m_channel_groups_tree->topLevelItem(i);
        Q_ASSERT(grpItem != NULL);
        QVariant var = grpItem->data(KColumnName, PROP_ID);
        if (var.isValid() == true && var.toUInt() == id)
            delete grpItem;
    }
}

void FixtureManager::slotModeChanged(Doc::Mode mode)
{
    bool design = (mode == Doc::Design);
    int fxiCount = selectedFixtures().count();
    int grpCount = selectedGroups().count();
    int chanGroupCount = m_channel_groups_tree->selectedItems().count();
    bool channelsTab = (m_currentTabIndex == KChannelsTab);
    bool hasFixtures = (m_doc->fixtures().count() > 0);

    m_addAction->setEnabled(design);
    m_addRGBAction->setEnabled(design && channelsTab == false);
    m_removeAction->setEnabled(design && (fxiCount > 0 || grpCount > 0 ||
                                          (channelsTab && chanGroupCount > 0)));
    m_propertiesAction->setEnabled(design && (fxiCount == 1 ||
                                              (channelsTab && chanGroupCount == 1)));
    m_fadeConfigAction->setEnabled(design && hasFixtures && channelsTab == false);
    m_groupAction->setEnabled(design && fxiCount > 0 && channelsTab == false);
    m_unGroupAction->setEnabled(design && fxiCount > 0 && channelsTab == false);
    m_importAction->setEnabled(design && channelsTab == false);
    m_exportAction->setEnabled(design && hasFixtures && channelsTab == false);
    m_remapAction->setEnabled(design && hasFixtures && channelsTab == false);
    m_renumberAction->setEnabled(design && fxiCount > 0 && channelsTab == false);
    m_moveUpAction->setEnabled(design && channelsTab && chanGroupCount > 0);
    m_moveDownAction->setEnabled(design && channelsTab && chanGroupCount > 0);
}

void FixtureManager::slotFixtureGroupRemoved(quint32 id)
{
    for (int i = 0; i < m_fixtures_tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem* item = m_fixtures_tree->topLevelItem(i);
        Q_ASSERT(item != NULL);
        QVariant var = item->data(KColumnName, PROP_GROUP);
        if (var.isValid() && var.toUInt() == id)
        {
            delete item;
            break;
        }
    }

    updateGroupMenu();
}

void FixtureManager::slotFixtureGroupChanged(quint32 id)
{
    QTreeWidgetItem* item = m_fixtures_tree->groupItem(id);
    if (item == NULL)
        return;

    FixtureGroup* grp = m_doc->fixtureGroup(id);
    Q_ASSERT(grp != NULL);
    m_fixtures_tree->updateGroupItem(item, grp);
    updateGroupMenu();
}

void FixtureManager::slotDocLoaded()
{
    slotTabChanged(m_currentTabIndex);
}

/*****************************************************************************
 * Data view
 *****************************************************************************/

void FixtureManager::initDataView()
{
    // Create a splitter to divide list view and text view
    m_splitter = new QSplitter(Qt::Horizontal, this);
    layout()->addWidget(m_splitter);
    m_splitter->setSizePolicy(QSizePolicy::Expanding,
                              QSizePolicy::Expanding);

    QTabWidget *tabs = new QTabWidget(this);
    m_splitter->addWidget(tabs);

    /* Create the patch matrix tab first */
    initPatchView();
    tabs->addTab(m_patchGrid->parentWidget(), tr("Patch"));

    /* Create the flat patch table tab */
    initFixturesView();
    tabs->addTab(m_patchTable->parentWidget(), tr("Fixtures"));

    /* Create a tree widget for the fixture groups */
    quint32 treeFlags = FixtureTreeWidget::UniverseNumber |
                        FixtureTreeWidget::AddressRange |
                        FixtureTreeWidget::ShowGroups;

    m_fixtures_tree = new FixtureTreeWidget(m_doc, treeFlags, this);
    m_fixtures_tree->setIconSize(QSize(32, 32));
    m_fixtures_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_fixtures_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fixtures_tree->sortByColumn(KColumnAddress, Qt::AscendingOrder);

    connect(m_fixtures_tree, SIGNAL(itemSelectionChanged()),
            this, SLOT(slotSelectionChanged()));

    connect(m_fixtures_tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(slotDoubleClicked(QTreeWidgetItem*)));

    connect(m_fixtures_tree, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(slotContextMenuRequested(const QPoint&)));

    connect(m_fixtures_tree, SIGNAL(expanded(QModelIndex)),
            this, SLOT(slotFixtureItemExpanded()));

    connect(m_fixtures_tree, SIGNAL(collapsed(QModelIndex)),
            this, SLOT(slotFixtureItemExpanded()));

    tabs->addTab(m_fixtures_tree, tr("Groups"));

    m_channel_groups_tree = new QTreeWidget(this);
    QStringList chan_labels;
    chan_labels << tr("Name") << tr("Channels");
    m_channel_groups_tree->setHeaderLabels(chan_labels);
    m_channel_groups_tree->setRootIsDecorated(false);
    m_channel_groups_tree->setAllColumnsShowFocus(true);
    m_channel_groups_tree->setIconSize(QSize(32, 32));
    m_channel_groups_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(m_channel_groups_tree, SIGNAL(itemSelectionChanged()),
            this, SLOT(slotChannelsGroupSelectionChanged()));
    connect(m_channel_groups_tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(slotChannelsGroupDoubleClicked(QTreeWidgetItem*)));

    tabs->addTab(m_channel_groups_tree, tr("Channel Groups"));
/*
    m_rdmManager = new RDMManager(this, m_doc);
    tabs->addTab(m_rdmManager, "RDM");
    connect(m_rdmManager, SIGNAL(fixtureInfoReady(QString&)),
            this, SLOT(slotDisplayFixtureInfo(QString&)));
*/
    connect(tabs, SIGNAL(currentChanged(int)), this, SLOT(slotTabChanged(int)));

    /* Create the text view */
    createInfo();

    /* Status bar for patch warnings */
    m_statusLabel = new QLabel(this);
    m_statusLabel->setVisible(false);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setContentsMargins(6, 3, 6, 3);
    m_statusLabel->setStyleSheet("QLabel { background-color: #E0A030; color: black; }");
    layout()->addWidget(m_statusLabel);

    slotSelectionChanged();
    slotTabChanged(m_currentTabIndex);
}

void FixtureManager::initPatchView()
{
    QWidget* patchTab = new QWidget(this);
    QVBoxLayout* patchLayout = new QVBoxLayout(patchTab);
    patchLayout->setContentsMargins(4, 4, 4, 4);
    patchLayout->setSpacing(4);

    /* Header: universe selector, zoom and orientation controls */
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* uniLabel = new QLabel(tr("Universe"), patchTab);
    headerLayout->addWidget(uniLabel);

    m_universeCombo = new QComboBox(patchTab);
    m_universeCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_universeCombo->addItems(m_doc->inputOutputMap()->universeNames());
    headerLayout->addWidget(m_universeCombo);

    QToolButton* zoomInButton = new QToolButton(patchTab);
    zoomInButton->setDefaultAction(m_zoomInAction);
    headerLayout->addWidget(zoomInButton);

    QToolButton* zoomOutButton = new QToolButton(patchTab);
    zoomOutButton->setDefaultAction(m_zoomOutAction);
    headerLayout->addWidget(zoomOutButton);

    QToolButton* orientButton = new QToolButton(patchTab);
    orientButton->setDefaultAction(m_orientAction);
    headerLayout->addWidget(orientButton);

    headerLayout->addStretch();

    QToolButton* expandButton = new QToolButton(patchTab);
    expandButton->setDefaultAction(m_expandAction);
    headerLayout->addWidget(expandButton);

    patchLayout->addLayout(headerLayout);

    m_patchGrid = new PatchGridWidget(m_doc, patchTab);
    patchLayout->addWidget(m_patchGrid, 1);

    m_patchSummary = new QLabel(patchTab);
    patchLayout->addWidget(m_patchSummary);

    connect(m_universeCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotPatchUniverseChanged(int)));
    connect(m_patchGrid, SIGNAL(fixtureClicked(quint32,Qt::KeyboardModifiers)),
            this, SLOT(slotGridFixtureClicked(quint32,Qt::KeyboardModifiers)));
    connect(m_patchGrid, SIGNAL(fixtureDoubleClicked(quint32)),
            this, SLOT(slotGridFixtureDoubleClicked(quint32)));
    connect(m_patchGrid, SIGNAL(fixtureContextMenuRequested(quint32,QPoint)),
            this, SLOT(slotGridContextMenuRequested(quint32,QPoint)));
    connect(m_patchGrid, SIGNAL(fixtureMoveRequested(quint32,quint32)),
            this, SLOT(slotGridMoveRequested(quint32,quint32)));
}

void FixtureManager::initFixturesView()
{
    QWidget* fixturesTab = new QWidget(this);
    QVBoxLayout* fixturesLayout = new QVBoxLayout(fixturesTab);
    fixturesLayout->setContentsMargins(4, 4, 4, 4);
    fixturesLayout->setSpacing(4);

    m_patchSearch = new QLineEdit(fixturesTab);
    m_patchSearch->setPlaceholderText(tr("Search..."));
    m_patchSearch->setClearButtonEnabled(true);
    fixturesLayout->addWidget(m_patchSearch);

    m_patchModel = new FixturePatchModel(m_doc, this);

    m_patchProxy = new QSortFilterProxyModel(this);
    m_patchProxy->setSourceModel(m_patchModel);
    m_patchProxy->setSortRole(FixturePatchModel::SortRole);
    m_patchProxy->setFilterKeyColumn(-1);
    m_patchProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_patchTable = new QTableView(fixturesTab);
    m_patchTable->setModel(m_patchProxy);
    m_patchTable->setSortingEnabled(true);
    m_patchTable->sortByColumn(FixturePatchModel::ColumnAddress, Qt::AscendingOrder);
    m_patchTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_patchTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_patchTable->setAlternatingRowColors(true);
    m_patchTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_patchTable->verticalHeader()->setVisible(false);
    m_patchTable->horizontalHeader()->setStretchLastSection(true);
    fixturesLayout->addWidget(m_patchTable, 1);

    connect(m_patchTable->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(slotTableSelectionChanged()));
    connect(m_patchTable, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(slotTableDoubleClicked(QModelIndex)));
    connect(m_patchTable, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(slotTableContextMenuRequested(QPoint)));
    connect(m_patchSearch, SIGNAL(textChanged(QString)),
            this, SLOT(slotPatchSearchChanged(QString)));
    connect(m_patchModel, SIGNAL(patchOverlapDetected(quint32,QList<quint32>)),
            this, SLOT(slotPatchOverlapDetected(quint32,QList<quint32>)));

    m_patchTable->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
}

void FixtureManager::updateView()
{
    // Record the current selection to restore it after the views are rebuilt
    QList <quint32> selection = selectedFixtures();

    // Record which top level items are open
    QList <QVariant> openGroups;
    for (int i = 0; i < m_fixtures_tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem* item = m_fixtures_tree->topLevelItem(i);
        if (item->isExpanded() == true)
            openGroups << item->data(KColumnName, PROP_GROUP);
    }

    if (m_doc->fixtures().count() > 0)
    {
        m_exportAction->setEnabled(true);
        m_remapAction->setEnabled(true);
        m_fadeConfigAction->setEnabled(true);
    }
    else
    {
        m_exportAction->setEnabled(false);
        m_fadeConfigAction->setEnabled(false);
        m_remapAction->setEnabled(false);
    }
    m_addRGBAction->setEnabled(true);
    m_importAction->setEnabled(true);
    m_moveUpAction->setEnabled(false);
    m_moveDownAction->setEnabled(false);

    m_fixtures_tree->updateTree();

    // Reopen groups that were open before update
    for (int i = 0; i < m_fixtures_tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem* item = m_fixtures_tree->topLevelItem(i);
        QVariant var = item->data(KColumnName, PROP_GROUP);
        if (openGroups.contains(var) == true)
        {
            item->setExpanded(true);
            openGroups.removeAll(var);
        }
    }

    if (m_patchModel != NULL)
    {
        m_patchModel->update();
        m_patchTable->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
        selectFixtures(selection);
    }

    if (m_patchGrid != NULL)
    {
        /* Universes may have been added or removed in the meantime */
        if (m_universeCombo != NULL)
        {
            QStringList names = m_doc->inputOutputMap()->universeNames();
            QStringList current;
            for (int i = 0; i < m_universeCombo->count(); i++)
                current << m_universeCombo->itemText(i);

            if (names != current)
            {
                QSignalBlocker blocker(m_universeCombo);
                m_universeCombo->clear();
                m_universeCombo->addItems(names);
                if (m_universeCombo->count() > 0)
                    m_universeCombo->setCurrentIndex(qMin(m_patchGrid->universe(), m_universeCombo->count() - 1));
            }
        }

        m_patchGrid->refresh();
    }

    updatePatchSummary();
    updateGroupMenu();
    slotModeChanged(m_doc->mode());

    m_fixtures_tree->header()->resizeSections(QHeaderView::ResizeToContents);
}

void FixtureManager::updatePatchSummary()
{
    if (m_patchSummary == NULL || m_patchGrid == NULL)
        return;

    int universe = m_patchGrid->universe();
    int used = 0;
    int conflicts = 0;
    int firstFreeBlock = -1;
    int freeRun = 0;

    for (int addr = 0; addr < UNIVERSE_SIZE; addr++)
    {
        QList <quint32> owners = m_doc->fixturesAtAddress((universe * UNIVERSE_SIZE) + addr);
        if (owners.isEmpty() == true)
        {
            freeRun++;
            if (freeRun == 8 && firstFreeBlock < 0)
                firstFreeBlock = addr - 7;
        }
        else
        {
            used++;
            freeRun = 0;
            if (owners.count() > 1)
                conflicts++;
        }
    }

    QString text = tr("%1/512 channels used").arg(used);

    if (conflicts > 0)
        text += QString(" · %1").arg(tr("%n overlapping channel(s)", "", conflicts));

    if (firstFreeBlock >= 0)
        text += QString(" · %1").arg(tr("next free block of 8 channels at %1").arg(firstFreeBlock + 1));
    else
        text += QString(" · %1").arg(tr("no free block of 8 channels"));

    m_patchSummary->setText(text);
}

void FixtureManager::updateChannelsGroupView()
{
    quint32 selGroupID = ChannelsGroup::invalidId();

    if (m_channel_groups_tree->selectedItems().size() > 0)
    {
        QTreeWidgetItem *item = m_channel_groups_tree->selectedItems().first();
        selGroupID = item->data(KColumnName, PROP_ID).toUInt();
    }

    if (m_channel_groups_tree->topLevelItemCount() > 0)
        for (int i = m_channel_groups_tree->topLevelItemCount() - 1; i >= 0; i--)
            m_channel_groups_tree->takeTopLevelItem(i);

    foreach (ChannelsGroup *grp, m_doc->channelsGroups())
    {
        QTreeWidgetItem *grpItem = new QTreeWidgetItem(m_channel_groups_tree);
        grpItem->setText(KColumnName, grp->name());
        grpItem->setData(KColumnName, PROP_ID, grp->id());
        grpItem->setText(KColumnChannels, QString("%1").arg(grp->getChannels().count()));
        if (grp->getChannels().count() > 0)
        {
            SceneValue scv = grp->getChannels().at(0);
            Fixture *fxi = m_doc->fixture(scv.fxi);
            if (fxi == NULL)
                continue;

            const QLCChannel *ch = fxi->channel(scv.channel);
            if (ch != NULL)
                grpItem->setIcon(KColumnName, ch->getIcon());
        }
        if (selGroupID == grp->id())
            grpItem->setSelected(true);
    }
    m_addRGBAction->setEnabled(false);
    m_propertiesAction->setEnabled(false);
    m_groupAction->setEnabled(false);
    m_unGroupAction->setEnabled(false);
    m_fadeConfigAction->setEnabled(false);
    m_exportAction->setEnabled(false);
    m_importAction->setEnabled(false);
    m_remapAction->setEnabled(false);

    m_channel_groups_tree->header()->resizeSections(QHeaderView::ResizeToContents);
}

void FixtureManager::updateRDMView()
{
    m_addRGBAction->setEnabled(false);
    m_propertiesAction->setEnabled(false);
    m_groupAction->setEnabled(false);
    m_unGroupAction->setEnabled(false);
    m_fadeConfigAction->setEnabled(false);
    m_exportAction->setEnabled(false);
    m_importAction->setEnabled(false);
    m_remapAction->setEnabled(false);
}

QList <quint32> FixtureManager::selectedFixtures() const
{
    QList <quint32> ids;

    bool preferTree = (m_currentTabIndex == KGroupsTab);

    if (preferTree == false && m_patchTable != NULL && m_patchTable->selectionModel() != NULL)
    {
        foreach (QModelIndex index, m_patchTable->selectionModel()->selectedRows())
        {
            int row = m_patchProxy->mapToSource(index).row();
            quint32 id = m_patchModel->fixtureId(row);
            if (id != Fixture::invalidId())
                ids << id;
        }
    }

    if (ids.isEmpty() == true && m_fixtures_tree != NULL)
    {
        foreach (QTreeWidgetItem* item, m_fixtures_tree->selectedItems())
        {
            QVariant var = item->data(KColumnName, PROP_ID);
            if (var.isValid() == true)
                ids << var.toUInt();
        }
    }

    return ids;
}

QList <quint32> FixtureManager::selectedGroups() const
{
    QList <quint32> ids;

    if (m_fixtures_tree == NULL)
        return ids;

    foreach (QTreeWidgetItem* item, m_fixtures_tree->selectedItems())
    {
        QVariant var = item->data(KColumnName, PROP_GROUP);
        if (var.isValid() == true)
            ids << var.toUInt();
    }

    return ids;
}

void FixtureManager::selectFixtures(const QList <quint32>& ids)
{
    if (m_patchTable == NULL || m_patchTable->selectionModel() == NULL)
        return;

    QItemSelection selection;
    foreach (quint32 id, ids)
    {
        int row = m_patchModel->rowForFixture(id);
        if (row < 0)
            continue;

        QModelIndex top = m_patchProxy->mapFromSource(
                              m_patchModel->index(row, FixturePatchModel::ColumnID));
        QModelIndex bottom = m_patchProxy->mapFromSource(
                              m_patchModel->index(row, FixturePatchModel::ColumnCount - 1));
        selection.select(top, bottom);
    }

    m_patchTable->selectionModel()->select(selection,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void FixtureManager::showWarning(const QString& message)
{
    if (m_statusLabel == NULL)
        return;

    if (message.isEmpty() == true)
    {
        m_statusLabel->setVisible(false);
        return;
    }

    m_statusLabel->setText(message);
    m_statusLabel->setVisible(true);
}

void FixtureManager::fixtureSelected(quint32 id)
{
    Fixture* fxi = m_doc->fixture(id);
    if (fxi == NULL)
        return;

    if (m_info == NULL)
        createInfo();

    m_info->setText(QString("%1<BODY>%2</BODY></HTML>")
                    .arg(fixtureInfoStyleSheetHeader())
                    .arg(fixtureInfo(fxi)));

    // Enable/disable actions
    slotModeChanged(m_doc->mode());
}

void FixtureManager::fixtureGroupSelected(FixtureGroup* grp)
{
    QByteArray state = m_splitter->saveState();

    if (m_info != NULL)
    {
        delete m_info;
        m_info = NULL;
    }

    if (m_groupEditor != NULL)
    {
        delete m_groupEditor;
        m_groupEditor = NULL;
    }

    m_groupEditor = new FixtureGroupEditor(grp, m_doc, this);
    m_splitter->addWidget(m_groupEditor);

    m_splitter->restoreState(state);
}

void FixtureManager::createInfo()
{
    QByteArray state = m_splitter->saveState();

    if (m_info != NULL)
    {
        delete m_info;
        m_info = NULL;
    }

    if (m_groupEditor != NULL)
    {
        delete m_groupEditor;
        m_groupEditor = NULL;
    }

    m_info = new QTextBrowser(this);
    m_splitter->addWidget(m_info);

    m_splitter->restoreState(state);
}

void FixtureManager::slotSelectionChanged()
{
    int selectedCount = m_fixtures_tree->selectedItems().size();
    if (selectedCount == 1)
    {
        QTreeWidgetItem* item = m_fixtures_tree->selectedItems().first();
        Q_ASSERT(item != NULL);

        // Set the text view's contents
        QVariant fxivar = item->data(KColumnName, PROP_ID);
        QVariant grpvar = item->data(KColumnName, PROP_GROUP);
        if (fxivar.isValid() == true)
        {
            // Selected a fixture
            fixtureSelected(fxivar.toUInt());
        }
        else if (grpvar.isValid() == true)
        {
            FixtureGroup* grp = m_doc->fixtureGroup(grpvar.toUInt());
            Q_ASSERT(grp != NULL);
            fixtureGroupSelected(grp);
        }
        else
        {
            QString info = "<HTML><BODY>";
            QString uniName;
            double totalWeight = 0;
            int totalPower = 0;
            QVariant uniID = item->data(KColumnName, PROP_UNIVERSE);
            if (uniID.isValid() == true)
                uniName = m_doc->inputOutputMap()->getUniverseNameByID(uniID.toUInt());

            foreach (Fixture *fixture, m_doc->fixtures())
            {
                if (fixture == NULL || fixture->universe() != uniID.toUInt() || fixture->fixtureMode() == NULL)
                    continue;

                QLCFixtureMode *mode = fixture->fixtureMode();
                totalWeight += mode->physical().weight();
                totalPower += mode->physical().powerConsumption();
            }

            if (m_info == NULL)
                createInfo();

            info += QString("<H1>%1</H1><P>%2 <B>%3</B></P>")
                    .arg(uniName).arg(tr("This group contains all fixtures of"))
                    .arg(uniName);

            info += QString("<BR><P><B>%1</B>: %2Kg<BR><B>%3</B>: %4W</P>")
                    .arg(tr("Total estimated weight")).arg(QString::number(totalWeight))
                    .arg(tr("Maximum estimated power consumption")).arg(totalPower);

            info += "</BODY></HTML>";

            m_info->setText(info);
        }
    }
    else
    {
        // More than one or less than one selected
        QString info = "<HTML><BODY>";
        if (selectedCount > 1)
        {
            // Enable removal of multiple items in design mode
            if (m_doc->mode() == Doc::Design)
            {
                double totalWeight = 0;
                int totalPower = 0;

                info += tr("<H1>Multiple fixtures selected</H1>" \
                          "<P>Click <IMG SRC=\"" ":/edit_remove.png\">" \
                          " to remove the selected fixtures.</P>");

                foreach (QTreeWidgetItem *item, m_fixtures_tree->selectedItems())
                {
                    QVariant fxID = item->data(KColumnName, PROP_ID);
                    if (fxID.isValid() == false)
                        continue;

                    Fixture *fixture = m_doc->fixture(fxID.toUInt());

                    if (fixture == NULL || fixture->fixtureMode() == NULL)
                        continue;

                    QLCFixtureMode *mode = fixture->fixtureMode();
                    totalWeight += mode->physical().weight();
                    totalPower += mode->physical().powerConsumption();
                }

                info += QString("<BR><P><B>%1</B>: %2Kg<BR><B>%3</B>: %4W</P>")
                        .arg(tr("Total estimated weight")).arg(QString::number(totalWeight))
                        .arg(tr("Maximum estimated power consumption")).arg(totalPower);
            }
            else
            {
                info += tr("<H1>Multiple fixtures selected</H1>" \
                          "<P>Fixture list modification is not permitted" \
                          " in operate mode.</P>");
            }
        }
        else
        {
            if (m_fixtures_tree->topLevelItemCount() <= 0)
            {
                info += tr("<H1>No fixtures</H1>" \
                          "<P>Click <IMG SRC=\"" ":/edit_add.png\">" \
                          " to add fixtures.</P>");
            }
            else
            {
                info += tr("<H1>Nothing selected</H1>" \
                          "<P>Select a fixture from the list or " \
                          "click <IMG SRC=\"" ":/edit_add.png\">" \
                          " to add fixtures.</P>");
            }
        }
        info += "</BODY></HTML>";

        if (m_info == NULL)
            createInfo();
        m_info->setText(info);
    }

    // Enable/disable actions
    slotModeChanged(m_doc->mode());
}

void FixtureManager::slotTableSelectionChanged()
{
    QList <quint32> ids = selectedFixtures();

    if (m_patchGrid != NULL && ids != m_patchGrid->selectedFixtures())
        m_patchGrid->setSelectedFixtures(ids);

    showWarning(QString());

    if (ids.count() == 1)
    {
        fixtureSelected(ids.first());
    }
    else if (ids.count() > 1)
    {
        QString info = "<HTML><BODY>";
        if (m_doc->mode() == Doc::Design)
        {
            double totalWeight = 0;
            int totalPower = 0;

            info += tr("<H1>Multiple fixtures selected</H1>"
                      "<P>Click <IMG SRC=\"" ":/edit_remove.png\">"
                      " to remove the selected fixtures.</P>");

            foreach (quint32 id, ids)
            {
                Fixture* fixture = m_doc->fixture(id);
                if (fixture == NULL || fixture->fixtureMode() == NULL)
                    continue;

                QLCFixtureMode* mode = fixture->fixtureMode();
                totalWeight += mode->physical().weight();
                totalPower += mode->physical().powerConsumption();
            }

            info += QString("<BR><P><B>%1</B>: %2Kg<BR><B>%3</B>: %4W</P>")
                    .arg(tr("Total estimated weight")).arg(QString::number(totalWeight))
                    .arg(tr("Maximum estimated power consumption")).arg(totalPower);
        }
        else
        {
            info += tr("<H1>Multiple fixtures selected</H1>"
                      "<P>Fixture list modification is not permitted"
                      " in operate mode.</P>");
        }
        info += "</BODY></HTML>";

        if (m_info == NULL)
            createInfo();
        m_info->setText(info);
    }

    // Enable/disable actions
    slotModeChanged(m_doc->mode());
}

void FixtureManager::slotTableDoubleClicked(const QModelIndex& index)
{
    // Editing the address inline shouldn't open the properties dialog
    if (index.isValid() == true && index.column() == FixturePatchModel::ColumnAddress)
        return;

    if (m_doc->mode() != Doc::Operate)
        slotProperties();
}

void FixtureManager::slotTableContextMenuRequested(const QPoint& pos)
{
    QMenu menu(this);
    menu.addAction(m_addAction);
    menu.addAction(m_addRGBAction);
    menu.addSeparator();
    menu.addAction(m_propertiesAction);
    menu.addAction(m_removeAction);
    menu.addSeparator();
    menu.addAction(m_groupAction);
    menu.addAction(m_unGroupAction);
    menu.addSeparator();
    menu.addAction(m_renumberAction);
    menu.exec(m_patchTable->viewport()->mapToGlobal(pos));
}

void FixtureManager::slotGridFixtureClicked(quint32 id, Qt::KeyboardModifiers modifiers)
{
    QList <quint32> current = selectedFixtures();

    if (modifiers & Qt::ControlModifier)
    {
        if (current.contains(id) == true)
            current.removeAll(id);
        else
            current << id;
    }
    else if (modifiers & Qt::ShiftModifier)
    {
        if (current.contains(id) == false)
            current << id;
    }
    else
    {
        current.clear();
        current << id;
    }

    selectFixtures(current);

    int row = m_patchModel->rowForFixture(id);
    if (row >= 0)
        m_patchTable->scrollTo(m_patchProxy->mapFromSource(m_patchModel->index(row, 0)));
}

void FixtureManager::slotGridFixtureDoubleClicked(quint32 id)
{
    selectFixtures(QList <quint32> () << id);

    if (m_doc->mode() != Doc::Operate)
        editFixtureProperties();
}

void FixtureManager::slotGridContextMenuRequested(quint32 id, const QPoint& pos)
{
    selectFixtures(QList <quint32> () << id);
    slotTableContextMenuRequested(m_patchTable->viewport()->mapFromGlobal(pos));
}

void FixtureManager::slotGridMoveRequested(quint32 id, quint32 universeAddress)
{
    Fixture* fxi = m_doc->fixture(id);
    if (fxi == NULL)
        return;

    quint32 universe = universeAddress / UNIVERSE_SIZE;
    quint32 address = universeAddress % UNIVERSE_SIZE;

    if (universe >= quint32(m_doc->inputOutputMap()->universesCount()))
        return;

    if (fxi->universe() == universe && fxi->address() == address)
        return;

    /* Non cross-universe fixtures cannot overflow their universe */
    if (fxi->crossUniverse() == false && fxi->channels() <= UNIVERSE_SIZE &&
        address + fxi->channels() > UNIVERSE_SIZE)
    {
        address = UNIVERSE_SIZE - fxi->channels();
    }

    fxi->setUniverse(universe);
    fxi->setAddress(address);

    selectFixtures(QList <quint32> () << id);
    updateView();

    QList <quint32> others = m_patchModel->overlappingFixtures(id);
    if (others.isEmpty() == false)
        slotPatchOverlapDetected(id, others);
}

void FixtureManager::slotPatchUniverseChanged(int index)
{
    if (m_patchGrid == NULL)
        return;

    m_patchGrid->setUniverse(index);
    updatePatchSummary();
}

void FixtureManager::slotPatchZoomIn()
{
    if (m_patchGrid != NULL)
        m_patchGrid->setCellSize(m_patchGrid->cellSize() + 4);
}

void FixtureManager::slotPatchZoomOut()
{
    if (m_patchGrid != NULL)
        m_patchGrid->setCellSize(m_patchGrid->cellSize() - 4);
}

void FixtureManager::slotPatchOrientationToggled(bool checked)
{
    if (m_patchGrid != NULL)
        m_patchGrid->setOrientation(checked ? PatchGridWidget::ColumnMajor : PatchGridWidget::RowMajor);

    if (m_orientAction != NULL)
        m_orientAction->setText(checked ? tr("Columns") : tr("Rows"));
}

void FixtureManager::slotPatchExpandToggled(bool checked)
{
    QWidget* right = m_splitter->widget(1);
    if (right != NULL)
        right->setVisible(checked == false);

    if (m_expandAction != NULL)
        m_expandAction->setText(checked ? tr("Collapse") : tr("Expand"));
}

void FixtureManager::slotPatchSearchChanged(const QString& filter)
{
    if (m_patchProxy != NULL)
        m_patchProxy->setFilterFixedString(filter);
}

void FixtureManager::slotPatchOverlapDetected(quint32 id, const QList <quint32>& others)
{
    if (others.isEmpty() == true)
    {
        showWarning(QString());
        return;
    }

    Fixture* fxi = m_doc->fixture(id);
    QString fixtureName = fxi != NULL ? fxi->name() : tr("Fixture");

    QStringList names;
    foreach (quint32 other, others)
    {
        Fixture* fixture = m_doc->fixture(other);
        if (fixture != NULL)
            names << fixture->name();
    }

    showWarning(tr("Warning: %1 overlaps with %2")
                .arg(fixtureName).arg(names.join(", ")));
}

void FixtureManager::slotPatchRenumber()
{
    QList <quint32> ids = selectedFixtures();
    if (ids.isEmpty() == true)
    {
        QMessageBox::information(this, tr("Renumber fixtures"),
            tr("Please select at least one fixture to renumber."));
        return;
    }

    PatchRenumber dlg(m_doc, ids, this);
    if (dlg.exec() == QDialog::Accepted)
    {
        updateView();
        selectFixtures(ids);
    }
}

void FixtureManager::slotChannelsGroupSelectionChanged()
{
    if (m_info == NULL)
        createInfo();

    int selectedCount = m_channel_groups_tree->selectedItems().size();

    if (selectedCount == 1)
    {
        QTreeWidgetItem* item = m_channel_groups_tree->selectedItems().first();
        Q_ASSERT(item != NULL);

        // Set the text view's contents
        QVariant grpvar = item->data(KColumnName, PROP_ID);
        if (grpvar.isValid() == true)
        {
            ChannelsGroup *chGroup = m_doc->channelsGroup(grpvar.toUInt());
            if (chGroup != NULL)
                m_info->setText(QString("%1<BODY>%2</BODY></HTML>")
                                .arg(channelsGroupInfoStyleSheetHeader())
                                .arg(channelsGroupInfo(chGroup)));
        }
        m_removeAction->setEnabled(true);
        m_propertiesAction->setEnabled(true);
        int selIdx = m_channel_groups_tree->currentIndex().row();
        if (selIdx == 0)
            m_moveUpAction->setEnabled(false);
        else
            m_moveUpAction->setEnabled(true);
        if (selIdx == m_channel_groups_tree->topLevelItemCount() - 1)
            m_moveDownAction->setEnabled(false);
        else
            m_moveDownAction->setEnabled(true);
    }
    else if (selectedCount > 1)
    {
        m_info->setText(tr("<HTML><BODY><H1>Multiple groups selected</H1>" \
                  "<P>Click <IMG SRC=\"" ":/edit_remove.png\">" \
                  " to remove the selected groups.</P></BODY></HTML>"));
        m_removeAction->setEnabled(true);
        m_propertiesAction->setEnabled(false);
    }
    else
    {
        m_info->setText(tr("<HTML><BODY><H1>Nothing selected</H1>" \
                  "<P>Select a channel group from the list or " \
                  "click <IMG SRC=\"" ":/edit_add.png\">" \
                  " to add a new channels group.</P></BODY></HTML>"));
        m_removeAction->setEnabled(false);
        m_propertiesAction->setEnabled(false);
    }
}

void FixtureManager::slotDoubleClicked(QTreeWidgetItem* item)
{
    if (item != NULL && m_doc->mode() != Doc::Operate)
        slotProperties();
}

void FixtureManager::slotChannelsGroupDoubleClicked(QTreeWidgetItem*)
{
    slotChannelsGroupSelectionChanged();
    editChannelGroupProperties();
}

void FixtureManager::slotTabChanged(int index)
{
    m_currentTabIndex = index;

    if (index == KChannelsTab)
    {
        m_addAction->setToolTip(tr("Add group..."));
        if (m_expandAction->isChecked() == true)
            m_expandAction->setChecked(false);
        updateChannelsGroupView();
        slotChannelsGroupSelectionChanged();
    }
    else if (index == KFixturesTab)
    {
        m_addAction->setToolTip(tr("Add fixture..."));
        updateView();
    }
    else if (index == KGroupsTab)
    {
        m_addAction->setToolTip(tr("Add fixture..."));
        updateView();
        slotSelectionChanged();
    }
    else
    {
        m_addAction->setToolTip(tr("Add fixture..."));
        updateView();
        updatePatchSummary();
    }

    slotModeChanged(m_doc->mode());
}

void FixtureManager::slotFixtureItemExpanded()
{
    m_fixtures_tree->header()->resizeSections(QHeaderView::ResizeToContents);
}

void FixtureManager::slotDisplayFixtureInfo(QString &info)
{
    m_info->setText(info);
}

void FixtureManager::selectGroup(quint32 id)
{
    for (int i = 0; i < m_fixtures_tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem* item = m_fixtures_tree->topLevelItem(i);
        QVariant var = item->data(KColumnName, PROP_GROUP);
        if (var.isValid() == false)
            continue;

        if (var.toUInt() == id)
        {
            m_fixtures_tree->setCurrentItem(item);
            slotSelectionChanged();
            break;
        }
    }
}

QString FixtureManager::fixtureInfoStyleSheetHeader() const
{
    QString info;

    QPalette pal;
    QColor hlBack(pal.color(QPalette::Highlight));
    QColor hlText(pal.color(QPalette::HighlightedText));

    info += "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">";
    info += "<HTML><HEAD></HEAD><STYLE>";
    info += QString(".hilite {" \
                    "   background-color: %1;" \
                    "   color: %2;" \
                    "   font-size: x-large;" \
                    "}").arg(hlBack.name()).arg(hlText.name());
    info += QString(".subhi {" \
                    "   background-color: %1;" \
                    "   color: %2;" \
                    "   font-weight: bold;" \
                    "}").arg(hlBack.name()).arg(hlText.name());
    info += QString(".emphasis {" \
                    "   font-weight: bold;" \
                    "}");
    info += QString(".tiny {"\
                    "   font-size: small;" \
                    "}");
    info += QString(".author {" \
                    "   font-weight: light;" \
                    "   font-style: italic;" \
                    "   text-align: right;" \
                    "   font-size: small;"  \
                    "}");
    info += "</STYLE>";
    return info;
}

QString FixtureManager::fixtureInfo(const Fixture *fixture) const
{
    QString info;

    QString title("<TR><TD CLASS='hilite' COLSPAN='3'>%1</TD></TR>");
    QString subTitle("<TR><TD CLASS='subhi' COLSPAN='3'>%1</TD></TR>");
    QString genInfo("<TR><TD CLASS='emphasis'>%1</TD><TD COLSPAN='2'>%2</TD></TR>");

    /********************************************************************
     * General info
     ********************************************************************/

    info += "<TABLE COLS='3' WIDTH='100%'>";

    if (fixture == NULL)
    {
        info += "</TABLE>";
        return info;
    }

    // Fixture title
    info += title.arg(fixture->name());

    const QLCFixtureDef* fixtureDef = fixture->fixtureDef();
    const QLCFixtureMode* fixtureMode = fixture->fixtureMode();

    if (fixtureDef != NULL && fixtureMode != NULL)
    {
        // Manufacturer
        info += genInfo.arg(tr("Manufacturer")).arg(fixtureDef->manufacturer());
        info += genInfo.arg(tr("Model")).arg(fixtureDef->model());
        info += genInfo.arg(tr("Mode")).arg(fixtureMode->name());
        info += genInfo.arg(tr("Type")).arg(fixtureDef->typeToString(fixtureDef->type()));
    }

    // Universe
    info += genInfo.arg(tr("Universe")).arg(fixture->universe() + 1);

    // Address
    QString range = QString("%1 - %2").arg(fixture->address() + 1).arg(fixture->address() + fixture->channels());
    info += genInfo.arg(tr("Address Range")).arg(range);

    // Channels
    info += genInfo.arg(tr("Channels")).arg(fixture->channels());

    // Overlapping addresses (allowed, but worth a warning)
    QList <quint32> overlapping;
    for (quint32 ch = 0; ch < fixture->channels(); ch++)
    {
        foreach (quint32 id, m_doc->fixturesAtAddress(fixture->universeAddress() + ch))
        {
            if (id != fixture->id() && overlapping.contains(id) == false)
                overlapping << id;
        }
    }

    if (overlapping.isEmpty() == false)
    {
        QStringList names;
        foreach (quint32 id, overlapping)
        {
            Fixture* other = m_doc->fixture(id);
            if (other != NULL)
                names << other->name();
        }

        info += QString("<TR><TD CLASS='emphasis'>%1</TD><TD COLSPAN='2'>%2</TD></TR>")
                .arg(tr("Overlaps with")).arg(names.join(", "));
    }

    // Binary address
    QString binaryStr = QString("%1").arg(fixture->address() + 1, 10, 2, QChar('0'));
    QString dipTable("<TABLE COLS='33' cellspacing='0'><TR><TD COLSPAN='33'><IMG SRC=\"" ":/ds_top.png\"></TD></TR>");
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

    info += genInfo.arg(tr("Binary Address (DIP)"))
            .arg(QString("%1").arg(dipTable));

    /********************************************************************
     * Channels
     ********************************************************************/

    // Title row
    info += QString("<TR><TD CLASS='subhi'>%1</TD>").arg(tr("Channel"));
    info += QString("<TD CLASS='subhi'>%1</TD>").arg(tr("DMX"));
    info += QString("<TD CLASS='subhi'>%1</TD></TR>").arg(tr("Name"));

    // Fill table with the fixture's channels
    for (quint32 ch = 0; ch < fixture->channels(); ch++)
    {
        QString chInfo("<TR><TD>%1</TD><TD>%2</TD><TD>%3</TD></TR>");
        info += chInfo.arg(ch + 1).arg(fixture->address() + ch + 1)
                .arg(fixture->channel(ch)->name());
    }

    /********************************************************************
     * Extended device information
     ********************************************************************/

    if (fixtureMode != NULL)
    {
        const QLCPhysical physical = fixtureMode->physical();
        info += title.arg(tr("Physical"));

        float mmInch = 0.0393700787;
        float kgLbs = 2.20462262;
        QString mm("%1mm (%2\")");
        QString kg("%1kg (%2 lbs)");
        QString W("%1W");
        info += genInfo.arg(tr("Width")).arg(mm.arg(physical.width()))
                                        .arg(physical.width() * mmInch, 0, 'g', 4);
        info += genInfo.arg(tr("Height")).arg(mm.arg(physical.height()))
                                         .arg(physical.height() * mmInch, 0, 'g', 4);
        info += genInfo.arg(tr("Depth")).arg(mm.arg(physical.depth()))
                                        .arg(physical.depth() * mmInch, 0, 'g', 4);
        info += genInfo.arg(tr("Weight")).arg(kg.arg(physical.weight()))
                                         .arg(physical.weight() * kgLbs, 0, 'g', 4);
        info += genInfo.arg(tr("Power consumption")).arg(W.arg(physical.powerConsumption()));
        info += genInfo.arg(tr("DMX Connector")).arg(physical.dmxConnector());

        // Bulb
        QString K("%1K");
        QString lm("%1lm");
        info += subTitle.arg(tr("Bulb"));
        info += genInfo.arg(tr("Type")).arg(physical.bulbType());
        info += genInfo.arg(tr("Luminous Flux")).arg(lm.arg(physical.bulbLumens()));
        info += genInfo.arg(tr("Colour Temperature")).arg(K.arg(physical.bulbColourTemperature()));

        // Lens
        QString angle1("%1&deg;");
        QString angle2("%1&deg; &ndash; %2&deg;");

        info += subTitle.arg(tr("Lens"));
        info += genInfo.arg(tr("Name")).arg(physical.lensName());

        if (physical.lensDegreesMin() == physical.lensDegreesMax())
        {
            info += genInfo.arg(tr("Beam Angle"))
                .arg(angle1.arg(physical.lensDegreesMin()));
        }
        else
        {
            info += genInfo.arg(tr("Beam Angle"))
                .arg(angle2.arg(physical.lensDegreesMin())
                .arg(physical.lensDegreesMax()));
        }

        // Focus
        QString frange("%1&deg;");
        info += subTitle.arg(tr("Head(s)"));
        info += genInfo.arg(tr("Type")).arg(physical.focusType());
        info += genInfo.arg(tr("Pan Range")).arg(frange.arg(physical.focusPanMax()));
        info += genInfo.arg(tr("Tilt Range")).arg(frange.arg(physical.focusTiltMax()));
        if (physical.layoutSize() != QSize(1, 1))
        {
            info += genInfo.arg(tr("Layout"))
                           .arg(QString("%1 x %2").arg(physical.layoutSize().width()).arg(physical.layoutSize().height()));
        }
    }

    // HTML document & table closure
    info += "</TABLE>";

    if (fixtureDef != NULL)
    {
        info += "<HR>";
        info += "<DIV CLASS='author' ALIGN='right'>";
        info += tr("Fixture definition author: ") + fixtureDef->author();
        info += "</DIV>";
    }

    return info;
}

QString FixtureManager::channelsGroupInfoStyleSheetHeader() const
{
    QString info;

    QPalette pal;
    QColor hlBack(pal.color(QPalette::Highlight));
    QColor hlBackSmall(pal.color(QPalette::Shadow));
    QColor hlText(pal.color(QPalette::HighlightedText));

    info += "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">";
    info += "<HTML><HEAD></HEAD><STYLE>";
    info += QString(".hilite {" \
                    "   background-color: %1;" \
                    "   color: %2;" \
                    "   font-size: x-large;" \
                    "}").arg(hlBack.name()).arg(hlText.name());
    info += QString(".subhi {" \
                    "   background-color: %1;" \
                    "   color: %2;" \
                    "   font-weight: bold;" \
                    "}").arg(hlBackSmall.name()).arg(hlText.name());
    info += QString(".emphasis {" \
                    "   font-weight: bold;" \
                    "}");
    info += QString(".tiny {"\
                    "   font-size: small;" \
                    "}");
    info += "</STYLE>";
    return info;
}

QString FixtureManager::channelsGroupInfo(const ChannelsGroup *channelsGroup) const
{
    QString info;

    info += "<TABLE COLS='3' WIDTH='100%'>";

    if (channelsGroup != NULL)
    {
        // Fixture title
        const QString title("<TR><TD CLASS='hilite' COLSPAN='3'><CENTER>%1</CENTER></TD></TR>");
        info += title.arg(channelsGroup->name());

        /********************************************************************
         * Channels
         ********************************************************************/

        // Title row
        info += QString("<TR><TD CLASS='subhi'>%1</TD>").arg(tr("Fixture"));
        info += QString("<TD CLASS='subhi'>%1</TD>").arg(tr("Channel"));
        info += QString("<TD CLASS='subhi'>%1</TD></TR>").arg(tr("Description"));

        foreach (const SceneValue value, channelsGroup->getChannels())
        {
            const Fixture *fixture = m_doc->fixture(value.fxi);
            if (fixture == NULL)
                continue;

            const QLCFixtureMode *mode = fixture->fixtureMode();
            const QString chInfo("<TR><TD>%1</TD><TD>%2</TD><TD>%3</TD></TR>");
            if (mode != NULL)
            {
                info += chInfo.arg(fixture->name()).arg(value.channel + 1)
                    .arg(mode->channels().at(value.channel)->name());
            }
            else
            {
                info += chInfo.arg(fixture->name()).arg(value.channel + 1)
                    .arg(QString(tr("Channel %1")).arg(value.channel));
            }
        }
    }

    // HTML document & table closure
    info += "</TABLE>";

    return info;
}

/*****************************************************************************
 * Menu, toolbar and actions
 *****************************************************************************/

void FixtureManager::initActions()
{
    // Fixture actions
    m_addAction = new QAction(QIcon(":/edit_add.png"),
                              tr("Add fixture..."), this);
    connect(m_addAction, SIGNAL(triggered(bool)),
            this, SLOT(slotAdd()));

    m_addRGBAction = new QAction(QIcon(":/rgbpanel.png"),
                              tr("Add RGB panel..."), this);
    connect(m_addRGBAction, SIGNAL(triggered(bool)),
            this, SLOT(slotAddRGBPanel()));

    m_removeAction = new QAction(QIcon(":/edit_remove.png"),
                                 tr("Delete items"), this);
    connect(m_removeAction, SIGNAL(triggered(bool)),
            this, SLOT(slotRemove()));

    m_propertiesAction = new QAction(QIcon(":/configure.png"),
                                     tr("Properties..."), this);
    connect(m_propertiesAction, SIGNAL(triggered(bool)),
            this, SLOT(slotProperties()));

    m_fadeConfigAction = new QAction(QIcon(":/fade.png"),
                                     tr("Channels Fade Configuration..."), this);
    connect(m_fadeConfigAction, SIGNAL(triggered(bool)),
            this, SLOT(slotFadeConfig()));

    // Group actions
    m_groupAction = new QAction(QIcon(":/group.png"),
                                tr("Add fixture to group..."), this);

    m_unGroupAction = new QAction(QIcon(":/ungroup.png"),
                                tr("Remove fixture from group"), this);
    connect(m_unGroupAction, SIGNAL(triggered(bool)),
            this, SLOT(slotUnGroup()));

    m_newGroupAction = new QAction(tr("New Group..."), this);

    m_moveUpAction = new QAction(QIcon(":/up.png"),
                                 tr("Move channel group up..."), this);
    m_moveUpAction->setEnabled(false);
    connect(m_moveUpAction, SIGNAL(triggered(bool)),
            this, SLOT(slotMoveGroupUp()));

    m_moveDownAction = new QAction(QIcon(":/down.png"),
                                 tr("Move channel group down..."), this);
    m_moveDownAction->setEnabled(false);
    connect(m_moveDownAction, SIGNAL(triggered(bool)),
            this, SLOT(slotMoveGroupDown()));

    m_importAction = new QAction(QIcon(":/fileimport.png"),
                                 tr("Import fixtures..."), this);
    connect(m_importAction, SIGNAL(triggered(bool)),
            this, SLOT(slotImport()));

    m_exportAction = new QAction(QIcon(":/fileexport.png"),
                                 tr("Export fixtures..."), this);

    connect(m_exportAction, SIGNAL(triggered(bool)),
            this, SLOT(slotExport()));

    m_remapAction = new QAction(QIcon(":/remap.png"),
                               tr("Remap fixtures..."), this);
    connect(m_remapAction, SIGNAL(triggered(bool)),
            this, SLOT(slotRemap()));

    // Patch matrix actions
    m_zoomInAction = new QAction(tr("Zoom +"), this);
    m_zoomInAction->setToolTip(tr("Increase the cell size of the patch matrix"));
    connect(m_zoomInAction, SIGNAL(triggered(bool)),
            this, SLOT(slotPatchZoomIn()));

    m_zoomOutAction = new QAction(tr("Zoom -"), this);
    m_zoomOutAction->setToolTip(tr("Decrease the cell size of the patch matrix"));
    connect(m_zoomOutAction, SIGNAL(triggered(bool)),
            this, SLOT(slotPatchZoomOut()));

    m_orientAction = new QAction(tr("Rows"), this);
    m_orientAction->setToolTip(tr("Change the channels filling order"));
    m_orientAction->setCheckable(true);
    connect(m_orientAction, SIGNAL(toggled(bool)),
            this, SLOT(slotPatchOrientationToggled(bool)));

    m_expandAction = new QAction(tr("Expand"), this);
    m_expandAction->setToolTip(tr("Hide the info panel to enlarge the patch matrix"));
    m_expandAction->setCheckable(true);
    connect(m_expandAction, SIGNAL(toggled(bool)),
            this, SLOT(slotPatchExpandToggled(bool)));

    m_renumberAction = new QAction(QIcon(":/edit.png"),
                                   tr("Renumber..."), this);
    connect(m_renumberAction, SIGNAL(triggered(bool)),
            this, SLOT(slotPatchRenumber()));
}

void FixtureManager::updateGroupMenu()
{
    if (m_groupMenu == NULL)
    {
        m_groupMenu = new QMenu(this);
        connect(m_groupMenu, SIGNAL(triggered(QAction*)),
                this, SLOT(slotGroupSelected(QAction*)));
    }

    foreach (QAction* a, m_groupMenu->actions())
        m_groupMenu->removeAction(a);

    // Put all known fixture groups to the menu
    foreach (FixtureGroup* grp, m_doc->fixtureGroups())
    {
        QAction* a = m_groupMenu->addAction(grp->name());
        a->setData((qulonglong) grp);
    }

    // Put a new group action to the group menu
    m_groupMenu->addAction(m_newGroupAction);

    // Put the group menu to the group action
    m_groupAction->setMenu(m_groupMenu);
}

void FixtureManager::initToolBar()
{
    QToolBar* toolbar = new QToolBar(tr("Fixture manager"), this);
    toolbar->setFloatable(false);
    toolbar->setMovable(false);
    layout()->setMenuBar(toolbar);
    toolbar->addAction(m_addAction);
    toolbar->addAction(m_addRGBAction);
    toolbar->addAction(m_removeAction);
    toolbar->addAction(m_propertiesAction);
    toolbar->addAction(m_fadeConfigAction);
    toolbar->addSeparator();
    toolbar->addAction(m_groupAction);
    toolbar->addAction(m_unGroupAction);
    toolbar->addSeparator();
    toolbar->addAction(m_moveUpAction);
    toolbar->addAction(m_moveDownAction);
    toolbar->addSeparator();
    toolbar->addAction(m_importAction);
    toolbar->addAction(m_exportAction);
    toolbar->addAction(m_remapAction);
    toolbar->addAction(m_renumberAction);

    QToolButton* btn = qobject_cast<QToolButton*> (toolbar->widgetForAction(m_groupAction));
    Q_ASSERT(btn != NULL);
    btn->setPopupMode(QToolButton::InstantPopup);
}

void FixtureManager::addFixture()
{
    AddFixture af(this, m_doc);
    if (af.exec() == QDialog::Rejected)
        return;

    if (af.invalidAddress())
    {
        QMessageBox msg(QMessageBox::Critical, tr("Error"),
                tr("Please enter a valid address"), QMessageBox::Ok);
        msg.exec();
        return;
    }

    quint32 latestFxi = Fixture::invalidId();

    QString name = af.name();
    quint32 address = af.address();
    quint32 universe = af.universe();
    quint32 channels = af.channels();
    int gap = af.gap();

    QLCFixtureDef* fixtureDef = af.fixtureDef();
    QLCFixtureMode* mode = af.mode();

    FixtureGroup* addToGroup = NULL;
    QTreeWidgetItem* current = m_fixtures_tree->currentItem();
    if (current != NULL)
    {
        if (current->parent() != NULL)
        {
            // Fixture selected
            QVariant var = current->parent()->data(KColumnName, PROP_GROUP);
            if (var.isValid() == true)
                addToGroup = m_doc->fixtureGroup(var.toUInt());
        }
        else
        {
            // Group selected
            QVariant var = current->data(KColumnName, PROP_GROUP);
            if (var.isValid() == true)
                addToGroup = m_doc->fixtureGroup(var.toUInt());
        }
    }

    /* If an empty name was given use the model instead */
    if (name.simplified().isEmpty())
    {
        if (fixtureDef != NULL)
            name = fixtureDef->model();
        else
            name = tr("Generic Dimmer");
    }

    /* Add the rest (if any) WITH address gap */
    for (int i = 0; i < af.amount(); i++)
    {
        QString modname;

        /* If we're adding more than one fixture,
           append a number to the end of the name */
        if (af.amount() > 1)
            modname = QString("%1 #%2").arg(name).arg(i + 1, AppUtil::digits(af.amount()), 10, QChar('0'));
        else
            modname = name;

        /* Create the fixture */
        Fixture* fxi = new Fixture(m_doc);

        /* Assign the next address AFTER the previous fixture
           address space plus gap. */
        fxi->setAddress(address + (i * channels) + (i * gap));
        fxi->setUniverse(universe);
        fxi->setName(modname);
        /* Set a fixture definition & mode if they were
           selected. Otherwise create a fixture definition
           and mode for a generic dimmer. */
        if (fixtureDef != NULL && mode != NULL)
        {
            fxi->setFixtureDefinition(fixtureDef, mode);
        }
        else
        {
            QLCFixtureDef* genericDef = fxi->genericDimmerDef(channels);
            QLCFixtureMode* genericMode = fxi->genericDimmerMode(genericDef, channels);
            fxi->setFixtureDefinition(genericDef, genericMode);
        }

        if (m_doc->addFixture(fxi) == true)
        {
            latestFxi = fxi->id();
            if (addToGroup != NULL)
                addToGroup->assignFixture(latestFxi);
        }
        else
        {
            qWarning() << Q_FUNC_INFO << "Unable to add fixture" << fxi->name();
            delete fxi;
        }
    }

    updateView();

    if (latestFxi != Fixture::invalidId())
    {
        selectFixtures(QList <quint32> () << latestFxi);

        QList <quint32> others = m_patchModel->overlappingFixtures(latestFxi);
        if (others.isEmpty() == false)
            slotPatchOverlapDetected(latestFxi, others);
    }
}

void FixtureManager::addChannelsGroup()
{
    ChannelsGroup *group = new ChannelsGroup(m_doc);

    AddChannelsGroup cs(this, m_doc, group);
    if (cs.exec() == QDialog::Accepted)
    {
        qDebug() << "Channels group added. Count: " << group->getChannels().count();
        m_doc->addChannelsGroup(group, group->id());
        updateChannelsGroupView();
    }
    else
        delete group;
}

void FixtureManager::slotAdd()
{
    if (m_currentTabIndex == KChannelsTab)
        addChannelsGroup();
    else
        addFixture();
}

void FixtureManager::slotAddRGBPanel()
{
    AddRGBPanel rgb(this, m_doc);
    if (rgb.exec() == QDialog::Accepted)
    {
        int rows = rgb.rows();
        int columns = rgb.columns();
        Fixture::Components components = rgb.components();

        FixtureGroup *grp = new FixtureGroup(m_doc);
        Q_ASSERT(grp != NULL);
        grp->setName(rgb.name());
        QSize panelSize(columns, rows);
        grp->setSize(panelSize);
        m_doc->addFixtureGroup(grp);
        updateGroupMenu();

        int transpose = 0;
        if (rgb.direction() == AddRGBPanel::Vertical)
        {
            int tmp = columns;
            columns = rows;
            rows = tmp;
            transpose = 1;
        }

        QLCFixtureDef *rowDef = NULL;
        QLCFixtureMode *rowMode = NULL;
        quint32 address = (quint32)rgb.address();
        int uniIndex = rgb.universeIndex();
        int currRow = 0;
        int rowInc = 1;
        int xPosStart = 0;
        int xPosEnd = columns - 1;
        int xPosInc = 1;

        quint32 phyWidth = rgb.physicalWidth();
        quint32 phyHeight = rgb.physicalHeight() / rows;

        if (transpose)
        {
            if (rgb.orientation() == AddRGBPanel::TopRight ||
                rgb.orientation() == AddRGBPanel::BottomRight)
            {
                currRow = rows -1;
                rowInc = -1;
            }
            if (rgb.orientation() == AddRGBPanel::BottomRight ||
                rgb.orientation() == AddRGBPanel::BottomLeft)
            {
                xPosStart = columns - 1;
                xPosEnd = 0;
                xPosInc = -1;
            }
        }
        else
        {
            if (rgb.orientation() == AddRGBPanel::BottomLeft ||
                rgb.orientation() == AddRGBPanel::BottomRight)
            {
                currRow = rows -1;
                rowInc = -1;
            }
            if (rgb.orientation() == AddRGBPanel::TopRight ||
                rgb.orientation() == AddRGBPanel::BottomRight)
            {
                xPosStart = columns - 1;
                xPosEnd = 0;
                xPosInc = -1;
            }
        }

        for (int i = 0; i < rows; i++)
        {
            Fixture *fxi = new Fixture(m_doc);
            Q_ASSERT(fxi != NULL);
            fxi->setName(tr("%1 - Row %2").arg(rgb.name()).arg(i + 1));
            if (rowDef == NULL)
                rowDef = fxi->genericRGBPanelDef(columns, components, rgb.is16Bit());
            if (rowMode == NULL)
                rowMode = fxi->genericRGBPanelMode(rowDef, components, rgb.is16Bit(), phyWidth, phyHeight);
            fxi->setFixtureDefinition(rowDef, rowMode);

            // Check universe span
            if (address + fxi->channels() > 512)
            {
                if (!rgb.crossUniverse())
                {
                    uniIndex++;
                    address = 0;
                }
            }
            if (m_doc->inputOutputMap()->getUniverseID(uniIndex) == m_doc->inputOutputMap()->invalidUniverse())
            {
                m_doc->inputOutputMap()->addUniverse();
                m_doc->inputOutputMap()->startUniverses();
            }

            fxi->setUniverse(m_doc->inputOutputMap()->getUniverseID(uniIndex));
            if (address + fxi->channels() > 512)
                fxi->setCrossUniverse(rgb.crossUniverse());
            fxi->setAddress(address);
            m_doc->addFixture(fxi, Fixture::invalidId(), rgb.crossUniverse());

            address += fxi->channels();
            if (address >= 512 && rgb.crossUniverse())
            {
                address -= 512;
                uniIndex++;
            }

            if (rgb.type() == AddRGBPanel::ZigZag)
            {
                int xPos = xPosStart;
                for (int h = 0; h < fxi->heads(); h++)
                {
                    if (transpose)
                        grp->assignHead(QLCPoint(currRow, xPos), GroupHead(fxi->id(), h));
                    else
                        grp->assignHead(QLCPoint(xPos, currRow), GroupHead(fxi->id(), h));
                    xPos += xPosInc;
                }
            }
            else if (rgb.type() == AddRGBPanel::Snake)
            {
                if (i%2 == 0)
                {
                    int xPos = xPosStart;
                    for (int h = 0; h < fxi->heads(); h++)
                    {
                        if (transpose)
                            grp->assignHead(QLCPoint(currRow, xPos), GroupHead(fxi->id(), h));
                        else
                            grp->assignHead(QLCPoint(xPos, currRow), GroupHead(fxi->id(), h));
                        xPos += xPosInc;
                    }
                }
                else
                {
                    int xPos = xPosEnd;
                    for (int h = 0; h < fxi->heads(); h++)
                    {
                        if (transpose)
                            grp->assignHead(QLCPoint(currRow, xPos), GroupHead(fxi->id(), h));
                        else
                            grp->assignHead(QLCPoint(xPos, currRow), GroupHead(fxi->id(), h));
                        xPos += (-xPosInc);
                    }
                }
            }
            currRow += rowInc;
        }

        updateView();
        m_doc->setModified();
    }
}

void FixtureManager::removeFixture()
{
    // Ask before deletion
    if (QMessageBox::question(this, tr("Delete Fixtures"),
                              tr("Do you want to delete the selected items?"),
                              QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
    {
        return;
    }

    QSet <quint32> groupsToDelete;
    QSet <quint32> fixturesToDelete;

    foreach (quint32 id, selectedFixtures())
        fixturesToDelete << id;

    // Fixture group nodes can only be selected in the Groups tree
    foreach (quint32 id, selectedGroups())
        groupsToDelete << id;

    // delete fixture groups
    foreach (quint32 id, groupsToDelete)
        m_doc->deleteFixtureGroup(id);

    // delete fixtures
    foreach (quint32 id, fixturesToDelete)
    {
        /** @todo This is REALLY bogus here, since Fixture or Doc should do
            this. However, FixtureManager is the only place to destroy fixtures,
            so it's rather safe to reset the fixture's address space here. */
        Fixture* fxi = m_doc->fixture(id);
        Q_ASSERT(fxi != NULL);
        QList<Universe*> ua = m_doc->inputOutputMap()->claimUniverses();
        int universe = fxi->universe();
        if (universe < ua.count())
            ua[universe]->reset(fxi->address(), fxi->channels());
        m_doc->inputOutputMap()->releaseUniverses();

        m_doc->deleteFixture(id);
    }
}

void FixtureManager::removeChannelsGroup()
{
    // Ask before deletion
    if (QMessageBox::question(this, tr("Delete Channels Group"),
                              tr("Do you want to delete the selected groups?"),
                              QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
    {
        return;
    }

    disconnect(m_channel_groups_tree, SIGNAL(itemSelectionChanged()),
            this, SLOT(slotChannelsGroupSelectionChanged()));

    QListIterator <QTreeWidgetItem*> it(m_channel_groups_tree->selectedItems());
    while (it.hasNext() == true)
    {
        QTreeWidgetItem* item(it.next());
        Q_ASSERT(item != NULL);

        QVariant var = item->data(KColumnName, PROP_ID);
        if (var.isValid() == true)
            m_doc->deleteChannelsGroup(var.toUInt());
    }
    updateChannelsGroupView();

    connect(m_channel_groups_tree, SIGNAL(itemSelectionChanged()),
            this, SLOT(slotChannelsGroupSelectionChanged()));
}

void FixtureManager::slotRemove()
{
    if (m_currentTabIndex == KChannelsTab)
        removeChannelsGroup();
    else
        removeFixture();
}

void FixtureManager::editFixtureProperties()
{
    QList <quint32> ids = selectedFixtures();
    if (ids.count() != 1)
        return;

    quint32 id = ids.first();
    Fixture* fxi = m_doc->fixture(id);
    if (fxi == NULL)
        return;

    QString manuf;
    QString model;
    QString mode;

    if (fxi->fixtureDef() != NULL)
    {
        manuf = fxi->fixtureDef()->manufacturer();
        model = fxi->fixtureDef()->model();
        mode = fxi->fixtureMode()->name();
    }

    AddFixture af(this, m_doc, fxi);
    af.setWindowTitle(tr("Change fixture properties"));
    if (af.exec() == QDialog::Accepted)
    {
        if (af.invalidAddress() == false)
        {
            bool changed = false;

            fxi->blockSignals(true);
            if (fxi->name() != af.name())
            {
                fxi->setName(af.name());
                changed = true;
            }
            if (fxi->universe() != af.universe())
            {
                fxi->setUniverse(af.universe());
                changed = true;
            }
            if (fxi->address() != af.address())
            {
                fxi->setAddress(af.address());
                changed = true;
            }
            fxi->blockSignals(false);

            if (af.fixtureDef() != NULL && af.mode() != NULL)
            {
                if (af.fixtureDef()->manufacturer() == KXMLFixtureGeneric &&
                    af.fixtureDef()->model() == KXMLFixtureGeneric)
                {
                    if (fxi->channels() != af.channels())
                    {
                        QLCFixtureDef* fixtureDef = fxi->genericDimmerDef(af.channels());
                        QLCFixtureMode* fixtureMode = fxi->genericDimmerMode(fixtureDef, af.channels());
                        fxi->setFixtureDefinition(fixtureDef, fixtureMode);
                    }
                }
                else
                {
                    fxi->setFixtureDefinition(af.fixtureDef(), af.mode());
                }
            }
            else
            {
                /* Generic dimmer */
                fxi->setFixtureDefinition(NULL, NULL);
                fxi->setChannels(af.channels());
            }

            // Emit changed signal
            if (changed)
                fxi->setID(fxi->id());

            updateView();
            selectFixtures(QList <quint32> () << id);

            QList <quint32> others = m_patchModel->overlappingFixtures(id);
            if (others.isEmpty() == false)
                slotPatchOverlapDetected(id, others);
        }
        else
        {
            QMessageBox msg(QMessageBox::Critical, tr("Error"),
                    tr("Please enter a valid address"), QMessageBox::Ok);
            msg.exec();
        }
    }
}

void FixtureManager::editChannelGroupProperties()
{
    int selectedCount = m_channel_groups_tree->selectedItems().size();

    if (selectedCount > 0)
    {
        QTreeWidgetItem* current = m_channel_groups_tree->selectedItems().first();
        QVariant var = current->data(KColumnName, PROP_ID);
        if (var.isValid() == true)
        {
            ChannelsGroup *group = m_doc->channelsGroup(var.toUInt());

            AddChannelsGroup cs(this, m_doc, group);
            if (cs.exec() == QDialog::Accepted)
            {
                qDebug() << "CHANNEL GROUP MODIFIED. Count: " << group->getChannels().count();
                m_doc->addChannelsGroup(group, group->id());
                updateChannelsGroupView();
            }
        }
    }
}

int FixtureManager::headCount(const QList <quint32>& ids) const
{
    int count = 0;

    foreach (quint32 id, ids)
    {
        Fixture* fxi = m_doc->fixture(id);
        if (fxi != NULL)
            count += fxi->heads();
    }

    return count;
}

void FixtureManager::slotProperties()
{
    if (m_currentTabIndex == KChannelsTab)
        editChannelGroupProperties();
    else
        editFixtureProperties();
}

void FixtureManager::slotFadeConfig()
{
    ChannelsSelection cfg(m_doc, this, ChannelsSelection::ConfigurationMode);
    if (cfg.exec() == QDialog::Rejected)
        return; // User pressed cancel
    m_doc->setModified();
}

void FixtureManager::slotRemap()
{
    FixtureRemap fxr(m_doc);
    if (fxr.exec() == QDialog::Rejected)
        return; // User pressed cancel

    updateView();
}

void FixtureManager::slotUnGroup()
{
    if (QMessageBox::question(this, tr("Ungroup fixtures?"),
                              tr("Do you want to ungroup the selected fixtures?"),
                              QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
    {
        return;
    }

    QList <quint32> ids = selectedFixtures();

    // Remove the selected fixtures from every fixture group they belong to
    foreach (FixtureGroup* grp, m_doc->fixtureGroups())
    {
        foreach (quint32 id, ids)
        {
            if (grp->fixtureList().contains(id) == true)
                grp->resignFixture(id);
        }
    }

    updateView();
}

void FixtureManager::slotGroupSelected(QAction* action)
{
    FixtureGroup* grp = NULL;

    if (action->data().isValid() == true)
    {
        // Existing group selected
        grp = (FixtureGroup*) (action->data().toULongLong());
        Q_ASSERT(grp != NULL);
    }
    else
    {
        // New Group selected.

        // Suggest an equilateral grid
        qreal side = sqrt(headCount(selectedFixtures()));
        if (side != floor(side))
            side += 1; // Fixture number doesn't provide a full square

        CreateFixtureGroup cfg(this);
        cfg.setSize(QSize(side, side));
        if (cfg.exec() != QDialog::Accepted)
            return; // User pressed cancel

        grp = new FixtureGroup(m_doc);
        Q_ASSERT(grp != NULL);
        grp->setName(cfg.name());
        grp->setSize(cfg.size());
        m_doc->addFixtureGroup(grp);
        updateGroupMenu();
    }

    // Assign selected fixtures to the group
    foreach (quint32 id, selectedFixtures())
        grp->assignFixture(id);

    updateView();
}

void FixtureManager::slotMoveGroupUp()
{
    if (m_channel_groups_tree->selectedItems().size() > 0)
    {
        QTreeWidgetItem* item = m_channel_groups_tree->selectedItems().first();
        quint32 grpID = item->data(KColumnName, PROP_ID).toUInt();
        m_doc->moveChannelGroup(grpID, -1);
        updateChannelsGroupView();
    }
}

void FixtureManager::slotMoveGroupDown()
{
    if (m_channel_groups_tree->selectedItems().size() > 0)
    {
        QTreeWidgetItem* item = m_channel_groups_tree->selectedItems().first();
        quint32 grpID = item->data(KColumnName, PROP_ID).toUInt();
        m_doc->moveChannelGroup(grpID, 1);
        updateChannelsGroupView();
    }
}

QString FixtureManager::createDialog(bool import)
{
    QString fileName;

    /* Create a file save dialog */
    QFileDialog dialog(this);
    if (import == true)
    {
        dialog.setWindowTitle(tr("Import Fixtures List"));
        dialog.setAcceptMode(QFileDialog::AcceptOpen);
    }
    else
    {
        dialog.setWindowTitle(tr("Export Fixtures List As"));
        dialog.setAcceptMode(QFileDialog::AcceptSave);
    }

    /* Append file filters to the dialog */
    QStringList filters;
    filters << tr("Fixtures List (*%1)").arg(KExtFixtureList);
#if defined(WIN32) || defined(Q_OS_WIN)
    filters << tr("All Files (*.*)");
#else
    filters << tr("All Files (*)");
#endif
    dialog.setNameFilters(filters);

    /* Append useful URLs to the dialog */
    QList <QUrl> sidebar;
    sidebar.append(QUrl::fromLocalFile(QDir::homePath()));
    sidebar.append(QUrl::fromLocalFile(QDir::rootPath()));
    dialog.setSidebarUrls(sidebar);

    /* Get file name */
    if (dialog.exec() != QDialog::Accepted)
        return "";

    fileName = dialog.selectedFiles().first();
    if (fileName.isEmpty() == true)
        return "";

    /* Always use the fixture definition suffix */
    if (import == false && fileName.right(5) != KExtFixtureList)
        fileName += KExtFixtureList;

    return fileName;
}

void FixtureManager::slotImport()
{
    QString fileName = createDialog(true);

    QXmlStreamReader *doc = QLCFile::getXMLReader(fileName);
    if (doc == NULL || doc->device() == NULL || doc->hasError())
    {
        qWarning() << Q_FUNC_INFO << "Unable to read from" << fileName;
        return;
    }

    while (!doc->atEnd())
    {
        if (doc->readNext() == QXmlStreamReader::DTD)
            break;
    }
    if (doc->hasError())
    {
        QLCFile::releaseXMLReader(doc);
        return;
    }

    if (doc->dtdName() == KXMLQLCFixturesList)
    {
        doc->readNextStartElement();
        if (doc->name() != KXMLQLCFixturesList)
        {
            qWarning() << Q_FUNC_INFO << "Fixture Definition node not found";
            QLCFile::releaseXMLReader(doc);
            return;
        }

        while (doc->readNextStartElement())
        {
            if (doc->name() == KXMLFixture)
            {
                Fixture* fxi = new Fixture(m_doc);
                Q_ASSERT(fxi != NULL);

                if (fxi->loadXML(*doc, m_doc, m_doc->fixtureDefCache()) == true)
                {
                    if (m_doc->addFixture(fxi /*, fxi->id()*/) == true)
                    {
                        /* Success */
                        qWarning() << Q_FUNC_INFO << "Fixture" << fxi->name() << "successfully created.";
                    }
                    else
                    {
                        /* Doc is full */
                        qWarning() << Q_FUNC_INFO << "Fixture" << fxi->name() << "cannot be created.";
                        delete fxi;
                    }
                }
                else
                {
                    qWarning() << Q_FUNC_INFO << "Fixture" << fxi->name() << "cannot be loaded.";
                    delete fxi;
                }
            }
            else if (doc->name() == KXMLQLCFixtureGroup)
            {
                FixtureGroup* grp = new FixtureGroup(m_doc);
                Q_ASSERT(grp != NULL);

                if (grp->loadXML(*doc) == true)
                {
                    m_doc->addFixtureGroup(grp, grp->id());
                }
                else
                {
                    qWarning() << Q_FUNC_INFO << "FixtureGroup" << grp->name() << "cannot be loaded.";
                    delete grp;
                }
            }
            else
            {
                qWarning() << Q_FUNC_INFO << "Unknown label tag:" << doc->name().toString();
                doc->skipCurrentElement();
            }
        }
        updateView();
    }
    QLCFile::releaseXMLReader(doc);
}

void FixtureManager::slotExport()
{
    QString fileName = createDialog(false);

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly) == false)
        return;

    QXmlStreamWriter doc(&file);
    doc.setAutoFormatting(true);
    doc.setAutoFormattingIndent(1);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    doc.setCodec("UTF-8");
#endif
    QLCFile::writeXMLHeader(&doc, KXMLQLCFixturesList);

    QListIterator <Fixture*> fxit(m_doc->fixtures());
    while (fxit.hasNext() == true)
    {
        Fixture* fxi(fxit.next());
        Q_ASSERT(fxi != NULL);
        fxi->saveXML(&doc);
    }

    QListIterator <FixtureGroup*>grpit(m_doc->fixtureGroups());
    while (grpit.hasNext() == true)
    {
        FixtureGroup *fxgrp(grpit.next());
        Q_ASSERT(fxgrp != NULL);
        fxgrp->saveXML(&doc);
    }

    doc.writeEndDocument();
    file.close();
}

void FixtureManager::slotContextMenuRequested(const QPoint&)
{
    QMenu menu(this);
    menu.addAction(m_addAction);
    menu.addAction(m_addRGBAction);
    menu.addAction(m_propertiesAction);
    menu.addAction(m_removeAction);
    menu.addSeparator();
    menu.addAction(m_groupAction);
    menu.addAction(m_unGroupAction);
    menu.exec(QCursor::pos());
}
