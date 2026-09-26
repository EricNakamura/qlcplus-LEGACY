---
description: "QLC+ QtWidget UI conventions — use when creating or editing files under ui/ (Virtual Console widgets, properties dialogs, managers, UI tests)."
applyTo: "ui/**"
---

# QLC+ UI Conventions

The UI (`libqlcplusui`) is the QLC+ 4 QtWidget layer. `App` (`ui/src/app.h`) is the main window and owns `Doc`, `VirtualConsole`, `SimpleDesk`, and `WebAccess`. **Per `CONTRIBUTING.md`, `VCWidget` changes must be discussed before implementing.**

## VCWidget contract

`VCWidget` (`ui/src/virtualconsole/vcwidget.h`) is the abstract base for every Virtual Console widget. It is a `QWidget` holding a `Doc* m_doc`, a unique `quint32 m_id`, a `WidgetType m_type`, and a page number.

Pure virtuals a subclass **must** implement:

- `VCWidget* createCopy(VCWidget* parent) const` — deep copy for clipboard/paste.
- `bool loadXML(QXmlStreamReader&)` / `bool saveXML(QXmlStreamWriter*)`.
- `void updateFeedback()` — re-send external-controller feedback (e.g. after a page flip).

Common overrides: `copyFrom(const VCWidget*)` (call the base), `editProperties()`, `setID(quint32)`, `setBackgroundImage()`, `slotInputValueChanged()`, `slotKeyPressed()`/`slotKeyReleased()`, `notifyFunctionStarting()`, `adjustIntensity()`, `postLoad()`.

- `WidgetType` enum (`UnknownWidget`, `ButtonWidget`, `SliderWidget`, `XYPadWidget`, `FrameWidget`, `SoloFrameWidget`, `SpeedDialWidget`, `CueListWidget`, `LabelWidget`, `AudioTriggersWidget`, `AnimationWidget`, `ClockWidget`) — add a value here for a new widget type.
- XML keys are `#define KXMLQLCVC... QStringLiteral("...")` macros in the widget's header (common ones in `vcwidget.h`).
- Input sources are `QSharedPointer<QLCInputSource>` stored in `m_inputs`; use `setInputSource()`/`inputSource()`/`checkInputSource()`/`sendFeedback()`.

## Adding a new VC widget

There is **no central factory** — creation and loading are explicit chains:

1. `vcwidget.h` — add the value to `enum WidgetType` and wire it into `VCWidget::typeToString()` / `typeToIcon()`.
2. `mywidget.h/.cpp` — subclass `VCWidget` (or `VCFrame` for containers); implement the pure virtuals above; define `KXMLQLCVCMyWidget...` keys in the header.
3. `virtualconsole.h/.cpp` — add a `slotAddMyWidget()` that `new`s the widget and calls `setupWidget(widget, parent)`; add the menu action and its `connect(...)` in the constructor.
4. `vcframe.cpp` `VCFrame::loadXML()` — add an `else if (root.name() == KXMLQLCVCMyWidget)` branch that `new`s the widget, calls `loadXML(root)`, and adds it to the frame. Also add the matching `saveXML` branch.
5. `mywidgetproperties.h/.cpp/.ui` — properties dialog (see below).
6. `ui/src/CMakeLists.txt` — add the `.cpp/.h/.ui` files.
7. `ui/test/mywidget/` — add a test + `CMakeLists.txt`; register in `ui/test/CMakeLists.txt`.

## Properties dialogs

- Multiple-inherit the generated `Ui_*` class and call `setupUi(this)` first:
  ```cpp
  class VCButtonProperties final : public QDialog, public Ui_VCButtonProperties
  {
      Q_OBJECT
      Q_DISABLE_COPY(VCButtonProperties)
  public:
      VCButtonProperties(VCButton* button, Doc* doc);
  protected slots:
      void accept() override;
  };
  ```
- The dialog holds a pointer to the widget and writes changes back in `accept()` (there is no `applyProperties()` method).
- `.ui` files are compiled automatically (`CMAKE_AUTOUIC ON`); list them in the module's `CMakeLists.txt` alongside the `.cpp/.h`.

## UI ↔ engine connection

- UI classes receive a `Doc*` and connect to its signals with old-style `SIGNAL()`/`SLOT()` connects, e.g. `connect(m_doc, SIGNAL(fixtureAdded(quint32)), this, SLOT(slotFixtureAdded(quint32)))`.
- Guard with `Q_ASSERT(doc != NULL)`.
- Persist window state with `QSettings` using `#define SETTINGS_...` keys and `restoreGeometry()`/`saveGeometry()`.
- Singleton windows use a `static X* s_instance` + `createAndShow()` pattern (e.g. `Monitor`).
- Managers: `FunctionManager` (`ui/src/functionmanager.h`), `FixtureManager` (`ui/src/fixturemanager.h`).

## UI tests

- Location: `ui/test/<widget>/` with `<widget>_test.cpp`, `<widget>_test.h`, `CMakeLists.txt`. Register in `ui/test/CMakeLists.txt`.
- Tests use `#define private public` / `#define protected public` before including UI headers, then `#undef`.
- Typical `init()` creates a `Doc` and a `VirtualConsole`; `cleanup()` deletes `VirtualConsole::instance()` then the `Doc`.
- CMake uses `include(../../src/include_ui.cmake)` + `include_ui_header(${module_name})` and links `qlcplusengine` + `qlcplusui`.
- UI tests are **skipped when no X server is detected** (and for `qmlui`). Run via `./unittest.sh ui` or `ninja -C build check` — never raw binaries from the source tree.

## Style

Match the surrounding UI code: Allman braces, 4 spaces, `m_` member prefix, `Q_DISABLE_COPY(Class)`, `final` on concrete classes, old-style `SIGNAL()`/`SLOT()` connects, `slotXxx` slot names, `Q_ASSERT` for pointer/state checks, `== true`/`== false` comparisons, include guards (never `#pragma once`), Apache-2.0 header, `/** @addtogroup ui_... @{ */ ... /** @} */` grouping. Wrap user-visible strings in `tr()`.

## Fork-specific UI changes — do not break

- **Global transition slider**: `VCSlider::SliderMode` has a fork-added `GlobalTransition` value (`vcslider.h`), serialized via `KXMLQLCVCSliderGlobalTransition`. It drives `Doc::globalTransitionTime()`. There is a known `// TODO: Make the global transition return when deleted` in `vcslider.cpp`.
- **Monitor "Always on top"**: `ui/src/monitor/monitor.cpp` adds a checkable toolbar action that toggles `Qt::WindowStaysOnTopHint`.
- **Create fixture button**: `ui/src/addfixture.cpp` adds `slotCreateFixtureClicked()` to launch the fixture editor.
- **RGB speed multiplier / Lua**: `ui/src/rgbmatrixeditor.cpp` and `ui/src/virtualconsole/vcmatrix*.cpp` are wired to the fork's Lua RGB engine and `m_speedMultiplier` — see the engine instructions for details.
