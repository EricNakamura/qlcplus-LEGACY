/*
  Q Light Controller
  fixtureeditordialog.cpp

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

#include <QVBoxLayout>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QToolBar>
#include <QAction>
#include <QDebug>
#include <QIcon>

#include "qlcfixturedefcache.h"
#include "qlcfixturedef.h"
#include "fixturedefio.h"
#include "fixtureeditor.h"

#include "fixturedefutils.h"
#include "fixtureeditordialog.h"

#define SETTINGS_OPENDIALOGSTATE "fixtureeditor/opendialog"

QList <FixtureEditorDialog*> FixtureEditorDialog::s_dialogs;

FixtureEditorDialog::FixtureEditorDialog(QWidget* parent, const Doc* doc,
                                         QLCFixtureDef* fixtureDef,
                                         const QString& fileName)
    : QDialog(parent)
    , m_doc(doc)
    , m_layout(NULL)
    , m_editor(NULL)
    , m_toolBar(NULL)
{
    /* A real window, so that it can be resized and maximized */
    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowIcon(QIcon(":/qlcplus-fixtureeditor.png"));

    initActions();

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    initToolBar();
    m_layout->addWidget(m_toolBar);

    setEditor(fixtureDef, fileName);

    s_dialogs << this;
}

FixtureEditorDialog::~FixtureEditorDialog()
{
    s_dialogs.removeAll(this);
}

QLCFixtureDef* FixtureEditorDialog::fixtureDefinition() const
{
    if (m_editor == NULL)
        return NULL;

    return m_editor->fixtureDefinition();
}

bool FixtureEditorDialog::requestClose()
{
    if (m_editor == NULL)
        return true;

    return m_editor->requestClose();
}

bool FixtureEditorDialog::closeAll()
{
    while (s_dialogs.isEmpty() == false)
    {
        FixtureEditorDialog* dialog = s_dialogs.first();
        if (dialog->requestClose() == false)
            return false;

        s_dialogs.removeAll(dialog);
        dialog->close();
    }

    return true;
}

FixtureEditorDialog* FixtureEditorDialog::find(const QString& manufacturer,
                                               const QString& model)
{
    foreach (FixtureEditorDialog* dialog, s_dialogs)
    {
        QLCFixtureDef* def = dialog->fixtureDefinition();
        if (def != NULL && def->manufacturer() == manufacturer &&
            def->model() == model)
            return dialog;
    }

    return NULL;
}

/*****************************************************************************
 * User interface
 *****************************************************************************/

void FixtureEditorDialog::initActions()
{
    m_fileNewAction = new QAction(QIcon(":/filenew.png"),
                                  tr("&New"), this);
    m_fileNewAction->setShortcut(QKeySequence("CTRL+N"));
    connect(m_fileNewAction, SIGNAL(triggered(bool)),
            this, SLOT(slotFileNew()));

    m_fileOpenAction = new QAction(QIcon(":/fileopen.png"),
                                   tr("&Open..."), this);
    m_fileOpenAction->setShortcut(QKeySequence("CTRL+O"));
    connect(m_fileOpenAction, SIGNAL(triggered(bool)),
            this, SLOT(slotFileOpen()));

    m_fileSaveAction = new QAction(QIcon(":/filesave.png"),
                                   tr("&Save"), this);
    m_fileSaveAction->setShortcut(QKeySequence("CTRL+S"));
    connect(m_fileSaveAction, SIGNAL(triggered(bool)),
            this, SLOT(slotFileSave()));

    m_fileSaveAsAction = new QAction(QIcon(":/filesaveas.png"),
                                     tr("Save &As..."), this);
    m_fileSaveAsAction->setShortcut(QKeySequence("CTRL+SHIFT+S"));
    connect(m_fileSaveAsAction, SIGNAL(triggered(bool)),
            this, SLOT(slotFileSaveAs()));
}

void FixtureEditorDialog::initToolBar()
{
    m_toolBar = new QToolBar(tr("Fixture definition editor"), this);
    m_toolBar->setFloatable(false);
    m_toolBar->setMovable(false);

    m_toolBar->addAction(m_fileNewAction);
    m_toolBar->addAction(m_fileOpenAction);
    m_toolBar->addAction(m_fileSaveAction);
    m_toolBar->addAction(m_fileSaveAsAction);
}

bool FixtureEditorDialog::setEditor(QLCFixtureDef* fixtureDef, const QString& fileName)
{
    Q_ASSERT(fixtureDef != NULL);

    if (m_editor != NULL)
    {
        if (m_editor->requestClose() == false)
        {
            delete fixtureDef;
            return false;
        }

        m_layout->removeWidget(m_editor);
        delete m_editor;
        m_editor = NULL;
    }

    m_editor = new QLCFixtureEditor(this, fixtureDef, fileName);
    m_layout->addWidget(m_editor);

    connect(m_editor, SIGNAL(definitionSaved(QLCFixtureDef*)),
            this, SLOT(slotEditorSaved(QLCFixtureDef*)));

    return true;
}

void FixtureEditorDialog::closeEvent(QCloseEvent* e)
{
    if (requestClose() == false)
        e->ignore();
    else
        e->accept();
}

void FixtureEditorDialog::reject()
{
    /* The user pressed Esc: make sure unsaved changes are handled */
    if (requestClose() == false)
        return;

    QDialog::reject();
    deleteLater();
}

/*****************************************************************************
 * Slots
 *****************************************************************************/

void FixtureEditorDialog::slotFileNew()
{
    setEditor(new QLCFixtureDef(), QString());
}

void FixtureEditorDialog::slotFileOpen()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Open a fixture definition"));
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);

    QStringList filters;
    filters << KAllFilter;
    filters << KQXFFilter;
    filters << KD4Filter;
    dialog.setNameFilters(filters);

    QSettings settings;
    QVariant var = settings.value(SETTINGS_OPENDIALOGSTATE);
    if (var.isValid() == true)
        dialog.restoreState(var.toByteArray());

    dialog.setDirectory(QLCFixtureDefCache::userDefinitionDirectory());

    if (dialog.exec() != QDialog::Accepted)
        return;

    settings.setValue(SETTINGS_OPENDIALOGSTATE, dialog.saveState());

    QString path = dialog.selectedFiles().first();
    QString error;
    QLCFixtureDef* fixtureDef = FixtureDefIO::load(path, error);
    if (fixtureDef == NULL)
    {
        QMessageBox::warning(this, tr("Fixture loading failed"),
                             tr("Unable to load fixture definition: ") + error);
        return;
    }

    setEditor(fixtureDef, path);
}

void FixtureEditorDialog::slotFileSave()
{
    if (m_editor != NULL)
        m_editor->save();
}

void FixtureEditorDialog::slotFileSaveAs()
{
    if (m_editor != NULL)
        m_editor->saveAs();
}

void FixtureEditorDialog::slotEditorSaved(QLCFixtureDef* fixtureDef)
{
    /* Reload the definition into the cache and re-apply it to the fixtures
       currently using it */
    FixtureDefUtils::reloadAndApply(m_doc, fixtureDef);

    emit definitionSaved(fixtureDef);
}
