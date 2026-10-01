/*
  Q Light Controller
  fixtureeditordialog.h

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

#ifndef FIXTUREEDITORDIALOG_H
#define FIXTUREEDITORDIALOG_H

#include <QDialog>
#include <QList>

class QLCFixtureDef;
class QLCFixtureEditor;
class QVBoxLayout;
class QToolBar;
class QAction;
class Doc;

/** @addtogroup ui_fixtures
 * @{
 */

/**
 * A dialog embedding the fixture definition editor, so that definitions can
 * be created and edited without leaving the QLC+ main application.
 *
 * When a definition is saved, the dialog reloads it into the Doc's definition
 * cache and re-applies it to the patched fixtures using it.
 */
class FixtureEditorDialog final : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(FixtureEditorDialog)

public:
    /**
     * Create a new dialog editing the given fixture definition.
     * Ownership of the definition is transferred to the dialog.
     *
     * @param parent The parent object that owns the dialog
     * @param doc The QLC engine instance
     * @param fixtureDef The fixture definition to edit
     * @param fileName The definition's file name, if it has one already
     */
    FixtureEditorDialog(QWidget* parent, const Doc* doc, QLCFixtureDef* fixtureDef,
                        const QString& fileName = QString());

    /** Destructor */
    ~FixtureEditorDialog();

    /**
     * Get the definition currently being edited
     * @return A fixture definition or NULL if the editor is missing
     */
    QLCFixtureDef* fixtureDefinition() const;

    /**
     * Check for unsaved changes and, if any, ask the user what to do.
     * @return true if it is safe to close the dialog, otherwise false
     */
    bool requestClose();

    /**
     * Check and close all the open fixture editor dialogs.
     * @return true if all the editors agreed to close, otherwise false
     */
    static bool closeAll();

    /**
     * Find an open editor dialog for the given manufacturer and model.
     * @return A matching dialog or NULL if none is open
     */
    static FixtureEditorDialog* find(const QString& manufacturer,
                                     const QString& model);

signals:
    /** The definition was saved and reloaded into the definition cache */
    void definitionSaved(QLCFixtureDef* fixtureDef);

protected:
    /** Create the toolbar actions */
    void initActions();

    /** Create the dialog toolbar */
    void initToolBar();

    /**
     * Replace the widget being edited with one for the given definition.
     * Ownership of the definition is transferred to the dialog, also when
     * this method fails.
     *
     * @return true if the editor was set, otherwise false
     */
    bool setEditor(QLCFixtureDef* fixtureDef, const QString& fileName);

    void closeEvent(QCloseEvent* e) override;
    void reject() override;

protected slots:
    void slotFileNew();
    void slotFileOpen();
    void slotFileSave();
    void slotFileSaveAs();
    void slotEditorSaved(QLCFixtureDef* fixtureDef);

private:
    const Doc* m_doc;
    QVBoxLayout* m_layout;
    QLCFixtureEditor* m_editor;
    QToolBar* m_toolBar;
    QAction* m_fileNewAction;
    QAction* m_fileOpenAction;
    QAction* m_fileSaveAction;
    QAction* m_fileSaveAsAction;

    static QList <FixtureEditorDialog*> s_dialogs;
};

/** @} */

#endif
