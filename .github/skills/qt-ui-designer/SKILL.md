---
name: qt-ui-designer
description: 'Create, edit, or review Qt Designer .ui files and their C++ wiring in this QLC+ fork. Use when asked to create a new screen, dialog, window, form, panel, or widget UI (criar tela, janela, diálogo, formulário, interface), add or rearrange controls in an existing .ui, promote a custom widget, wire a .ui to a QDialog/QWidget class, register a .ui in CMakeLists.txt, or fix a .ui that fails to compile or does not show up. Covers AUTOUIC, the Ui_* multiple-inheritance pattern, objectName conventions, translations, and QLC+ house style.'
argument-hint: 'Describe the screen you want (e.g. "a dialog to configure 4 DMX universes with a table and OK/Cancel")'
---

# Qt UI Designer (.ui) — QLC+ LEGACY

Build Qt Widgets screens as `.ui` XML files and wire them into C++ the way this codebase does.

## When to Use

- Creating a new dialog, window, form, panel, or reusable widget.
- Adding, removing, or rearranging controls in an existing `.ui`.
- Promoting a QLC+ custom widget (e.g. `SpeedDial`) inside a `.ui`.
- Wiring a `.ui` to a `QDialog`/`QWidget` subclass.
- Registering a `.ui` in a `CMakeLists.txt`.
- Debugging a `.ui` that fails to compile, is not found, or renders wrong.

## Non-negotiables (this fork)

Read these before writing anything — they are verified facts about this repo, not generic Qt advice.

1. **`CMAKE_AUTOUIC ON` is global** (`CMakeLists.txt:23`). There is **no** `qt_add_ui` / `set(..._UI ...)` anywhere. You register a `.ui` by listing it **as a source** next to its `.cpp`/`.h` in the target's source list.
2. **Generated `ui_<name>.h` files are never committed.** AUTOUIC writes them into the build tree. Never create, edit, or add them to source control. Include them by bare name: `#include "ui_editphysical.h"`.
3. **Multiple inheritance is the house pattern** (64 of 68 files). The C++ class inherits the Qt base **and** `Ui_<Class>`, then calls `setupUi(this)` first thing in the constructor. Composition (`Ui::X *ui`) exists only in `ui/src/addresstool.*` — do not copy it.
4. **The `.ui` root widget class must match the C++ base class.** A `QDialog` `.ui` cannot be inherited by a `QWidget` subclass — it will not compile.
5. **`QFormLayout` is used nowhere in this repo.** Use `QVBoxLayout`, `QHBoxLayout`, or `QGridLayout`.
6. **`retranslateUi` is never called manually.** No runtime language switching exists; translations apply at construction via `setupUi`.
7. **`.ui` files are version-agnostic** (`<ui version="4.0">`). Qt5/Qt6 differences live in CMake, not in the XML. Both scoped (`Qt::Horizontal`) and unscoped (`Qt::TextElideMode::ElideNone`) enum forms are accepted.
8. **Every new `.ui` carries the Apache-2.0 header inside `<comment>`** (see the templates). `AGENTS.md` requires it.

## Clarify before building

**Always ask before writing a `.ui` when any of the following is unknown.** Use the ask-questions tool; do not guess. A wrong guess here produces a screen that compiles but is unusable.

| Ask about | Why it matters |
|---|---|
| **Purpose & user flow** | What does the user do on this screen, in what order? Drives layout and control choice. |
| **Dialog vs embedded widget** | Determines the root class (`QDialog` vs `QWidget`) and whether OK/Cancel is needed. |
| **Where it lives** | `ui/src/`, `ui/src/virtualconsole/`, `fixtureeditor/`, or `plugins/<name>/` — decides the CMake target and the `<resources>` relative path. |
| **C++ class name** | The `<class>` element and the `Ui_<Class>` type. Confirm it, since some files mismatch (see Gotchas). |
| **Which controls** | Exact fields, tables, tabs, buttons — and their types (spin box vs line edit vs combo). |
| **Data binding** | Which `Doc`/engine object the screen edits, and whether changes are written back in `accept()` (the house pattern) or live. |
| **Persistence** | Does it need `QSettings` geometry save/restore (`SETTINGS_*` keys)? |
| **Icons/resources** | Does it need icons from `qlcui.qrc`? |
| **Translations** | Any string that must be translatable (default) vs `notr="true"` (units, symbols, format strings). |

If the user gives a vague request ("make me a screen for X"), ask the 2–3 most consequential questions first, then proceed. Do not interrogate — batch the questions.

## Procedure

### 1. Locate the target module

Pick the directory from the table above. Read a **neighbouring** `.ui` and its `.cpp`/`.h` first — match their structure, objectName style, and layout choices. Do not invent a new style.

### 2. Write the `.ui`

Start from [the dialog template](./assets/dialog-template.ui) or [the widget template](./assets/widget-template.ui). Follow the [.ui XML reference](./references/ui-xml-reference.md) for element order and attributes.

Minimum viable structure:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
 <author>...</author>
 <comment>... Apache-2.0 header ...</comment>
 <class>MyDialog</class>
 <widget class="QDialog" name="MyDialog">
  <property name="geometry"><rect><x>0</x><y>0</y><width>400</width><height>300</height></rect></property>
  <property name="windowTitle"><string>My Dialog</string></property>
  <layout class="QVBoxLayout" name="m_mainLayout">
   <!-- items -->
  </layout>
 </widget>
 <resources><include location="qlcui.qrc"/></resources>
 <connections>
  <connection>
   <sender>m_buttonBox</sender><signal>accepted()</signal>
   <receiver>MyDialog</receiver><slot>accept()</slot>
  </connection>
  <connection>
   <sender>m_buttonBox</sender><signal>rejected()</signal>
   <receiver>MyDialog</receiver><slot>reject()</slot>
  </connection>
 </connections>
</ui>
```

### 3. Wire the C++ side

Header — include the generated header and multiple-inherit:

```cpp
#include "ui_mydialog.h"

class MyDialog final : public QDialog, public Ui_MyDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(MyDialog)
public:
    MyDialog(QWidget* parent, Doc* doc);
protected slots:
    void accept() override;
};
```

Source — `setupUi(this)` **first**, then `init()`:

```cpp
MyDialog::MyDialog(QWidget* parent, Doc* doc)
    : QDialog(parent)
    , m_doc(doc)
{
    Q_ASSERT(doc != NULL);
    setupUi(this);
    init();
}
```

- Write changes back to the model in `accept()` (there is no `applyProperties()`).
- Connect to the engine with old-style `SIGNAL()`/`SLOT()`.
- Persist geometry with `QSettings` + `restoreGeometry()`/`saveGeometry()` when the screen is a standalone window.

### 4. Register in CMake

Add the `.ui` to the target's source list, alphabetically next to its `.cpp`/`.h`:

```cmake
add_library(${module_name}
    SHARED
    mydialog.cpp mydialog.h mydialog.ui
    ...
)
```

For plugins use `target_sources(${module_name} PRIVATE ...)` instead. Never add a `ui_*.h`.

### 5. Build and verify

```bash
cmake --build build
ninja -C build run
```

Then run the tests: `./unittest.sh ui` (or `ninja -C build check`). Never run raw test binaries from the source tree.

### 6. Translations (only if strings changed)

`lupdate` scans `.ui` files automatically. Add the module's `.ts` files to `TS_FILES` if new, then:

```bash
./translate.sh update
./translate.sh release
```

## objectName conventions

Two styles coexist. **Match the file you are editing**; for new files prefer the `m_` prefix (dominant in newer and Virtual Console code).

| Style | Examples | Where |
|---|---|---|
| `m_` prefix (preferred for new) | `m_buttonBox`, `m_nameEdit`, `m_sizeXSpin`, `m_mainLayout`, `m_uniMapTree` | newer code, `ui/src/virtualconsole/` |
| Designer defaults | `buttonBox`, `groupBox`, `tabWidget`, `verticalLayout`, `label_2` | older code, `plugins/*` |

- `<class>` is PascalCase and normally equals the root widget `name`.
- Layouts may also take the `m_` prefix (`m_mainLayout`, `m_mainGrid`).

## Gotchas

| Symptom / trap | Cause & fix |
|---|---|
| `ui_<name>.h` not found | The `.ui` is not listed in the target's CMake source list. AUTOUIC only generates for listed files. |
| Compile error on `setupUi` / base class | The `.ui` root class does not match the C++ base class. Make them agree. |
| `<class>` ≠ C++ class | Real mismatches exist: `fixtureeditor.ui` → `<class>FixtureEditor</class>` but C++ is `QLCFixtureEditor`; `vcproperties.ui` → `<class>VCPropertiesEditor</class>`; `vcaudiotriggersproperties.ui` → `<class>AudioTriggersConfiguration</class>`. The `Ui_<Class>` name follows the **`.ui` `<class>`**, not the C++ class. |
| `ui/src/app.ui` drives the main window | `<class>App</class>`; `App` is `class App : public QMainWindow, public Ui_App`. The toolbar and its actions live in the `.ui`; C++ keeps only dynamic bits (tabs, fade/recent menus, quit-action removal, toolbar spacer). `main/CMakeLists.txt` calls `include_ui_header()` because `app.h` pulls the generated `ui_app.h`. |
| `<widget>` inside a `QToolBar` lands in the wrong place | uic emits toolbar child widgets **before** all `<addaction>` entries, so an interleaved spacer jumps to the front. Declare only actions in the `.ui` and insert spacer widgets in C++ with `QToolBar::insertWidget()`. |
| Broken `<resources>` path | Some files point at the build tree (`../../build/fixtureeditor/.qt/rcc/qlcui.qrc`). Use a correct relative path to `ui/src/qlcui.qrc` for the file's location. |
| Editing a shared `.ui` affects several binaries | `aboutbox.ui` and `debugbox.ui` live in `ui/src/` but are compiled into `fixtureeditor` and `main` via relative paths. |
| String not translated | It is inside `<string notr="true">`. Remove `notr` for user-visible text. |
| Custom widget not found | It must be declared in `<customwidgets>` with `<extends>` and `<header>`. Only `SpeedDial` is promoted today (`vcspeeddialproperties.ui`). |

## Checklist

- [ ] Asked the clarifying questions that were actually unknown.
- [ ] Root widget class matches the C++ base class.
- [ ] `<class>` matches the intended `Ui_<Class>` name.
- [ ] Apache-2.0 header present in `<comment>`.
- [ ] Layouts are `QVBoxLayout`/`QHBoxLayout`/`QGridLayout` (no `QFormLayout`).
- [ ] objectName style matches the neighbouring files.
- [ ] User-visible strings are translatable; units/symbols are `notr="true"`.
- [ ] `<resources>` path is correct for the file's location (if icons are used).
- [ ] `buttonBox` `accepted()`/`rejected()` wired to `accept()`/`reject()` (dialogs).
- [ ] `.ui` added to the target's CMake source list; no `ui_*.h` committed.
- [ ] `setupUi(this)` is the first call in the constructor.
- [ ] Build passes and `./unittest.sh ui` is green.

## References

- [.ui XML reference](./references/ui-xml-reference.md) — element order, layouts, size policies, custom widgets, resources.
- [Dialog template](./assets/dialog-template.ui)
- [Widget template](./assets/widget-template.ui)
- Repo conventions: `.github/instructions/ui.instructions.md`, `AGENTS.md`.
