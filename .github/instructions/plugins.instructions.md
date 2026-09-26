---
description: "QLC+ I/O plugin conventions — use when creating or editing files under plugins/ (plugin classes, config dialogs, plugin CMake, plugin tests)."
applyTo: "plugins/**"
---

# QLC+ I/O Plugin Conventions

All I/O plugins implement the `QLCIOPlugin` interface defined in `plugins/interfaces/qlcioplugin.h`. Use `plugins/artnet/` as the reference implementation.

## Plugin class boilerplate

```cpp
class MyPlugin final : public QLCIOPlugin
{
    Q_OBJECT
    Q_INTERFACES(QLCIOPlugin)
    Q_PLUGIN_METADATA(IID QLCIOPlugin_iid)

public:
    // ...
};
```

- Pure virtuals that **must** be overridden: `init()`, `name()`, `capabilities()`, `pluginInfo()`.
- Optional virtuals: `openOutput`/`closeOutput`/`outputs`/`outputInfo`/`writeUniverse`, `openInput`/`closeInput`/`inputs`/`inputInfo`, `configure`/`canConfigure`/`setParameter`, `sendRDMCommand`, `sendFeedBack`.
- Capabilities are a bitmask: `Output`, `Input`, `Feedback`, `Infinite`, `RDM`, `Beats`.
- Emit `valueChanged(...)` for input and `configurationChanged()` when config changes.

## Directory layout

```
plugins/<name>/
├── CMakeLists.txt              # project(<name>) + add_subdirectory(src) [+ test]
├── src/
│   ├── CMakeLists.txt          # set(module_name ...) + add_library SHARED
│   ├── <name>plugin.h/.cpp
│   ├── configure<name>.h/.cpp/.ui   # optional config dialog
│   ├── <Name>_<lang>.ts / .qm
│   └── org.qlcplus.QLCPlus.<name>.metainfo.xml
└── test/                       # optional Qt Test executable
```

## CMake pattern

Follow `plugins/artnet/src/CMakeLists.txt`:

```cmake
set(module_name "myplugin")
add_library(${module_name} SHARED ${QM_FILES})
target_sources(${module_name} PRIVATE
    ../../interfaces/qlcioplugin.cpp ../../interfaces/qlcioplugin.h
    myplugin.cpp myplugin.h
)
target_include_directories(${module_name} PRIVATE ../../interfaces)
target_link_libraries(${module_name} PRIVATE
    Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Gui
    Qt${QT_MAJOR_VERSION}::Network Qt${QT_MAJOR_VERSION}::Widgets)
install(TARGETS ${module_name}
    LIBRARY DESTINATION ${INSTALLROOT}/${PLUGINDIR}
    RUNTIME DESTINATION ${INSTALLROOT}/${PLUGINDIR})
```

- Use `qt_add_translation` (Qt6) / `qt5_add_translation` (Qt5) guarded by `QT_VERSION_MAJOR GREATER 5`.
- Register the plugin with `add_subdirectory(<name>)` in `plugins/CMakeLists.txt`, respecting the existing platform guards (`if(NOT ANDROID AND NOT IOS)`, `if(UNIX AND NOT APPLE)`, etc.).
- Plugins are loaded at runtime by `engine/src/ioplugincache.cpp` via `QPluginLoader` — there is no central registry.

## Config dialogs

- Multiple-inherit the generated `Ui_*` class and call `setupUi(this)` first (see `plugins/artnet/src/configureartnet.cpp`).
- Wrap all user-visible strings in `tr()`.
- Persist settings with `QSettings`; use `#define` keys.

## Style

Match the surrounding plugin code: Allman braces, 4 spaces, `m_` member prefix, `Q_DISABLE_COPY`, old-style `SIGNAL()`/`SLOT()` connects, Apache-2.0 header block at the top of every file.
