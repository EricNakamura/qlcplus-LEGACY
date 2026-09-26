---
description: "QLC+ engine library conventions — use when creating or editing files under engine/ (Function subclasses, Doc, DMX pipeline, XML serialization, engine tests)."
applyTo: "engine/**"
---

# QLC+ Engine Conventions

The engine (`libqlcplusengine`) is UI-free. `Doc` (`engine/src/doc.h`) is the central document; `Function` (`engine/src/function.h`) is the base for all runnable objects. **Per `CONTRIBUTING.md`, engine changes must be discussed before implementing.**

## Adding a new Function type

1. `function.h` — add the next free bit to `enum Type` (it is a bitmask, `Q_ENUM`).
2. `function.cpp` — add a file-scope `const QString KMyTypeString("MyType");` and wire it into **both** `typeToString()` and `stringToType()`.
3. `mytype.h/.cpp` — subclass `Function` (and `DMXSource` too if it writes DMX directly, like `Scene`). Override:
   - `createCopy(Doc*, bool)` — base returns a plain `Function`, so this is **required**.
   - `copyFrom(const Function*)` — copy subclass data, then **call `Function::copyFrom()` last**.
   - `saveXML(QXmlStreamWriter*)` / `loadXML(QXmlStreamReader&)` — both **required** (base returns `false`).
   - `preRun()` / `write()` / `postRun()` — call the base implementation.
   - `totalDuration()`, `getIcon()`, `postLoad()`, `contains()`, `components()` as needed.
4. `engine/src/CMakeLists.txt` — add `mytype.cpp mytype.h` to the `add_library` list.
5. `function.cpp` `Function::loader()` — add `else if (type == Function::MyType) function = new class MyType(doc);` (the `new class X` elaborated-type syntax is house style here).
6. `engine/test/mytype/` — add a test + `CMakeLists.txt` + `test.sh`, and `add_subdirectory(mytype)` in `engine/test/CMakeLists.txt`.

## XML serialization

- Keys are `#define KXMLQLC... QStringLiteral("...")` macros in the owning class's header. Common Function keys live in `function.h`; subclass keys go in the subclass header.
- Booleans use `KXMLQLCTrue` / `KXMLQLCFalse` (`engine/src/qlcfile.h`).
- `saveXML`: `writeStartElement(KXMLQLCFunction)` → `saveXMLCommon(doc)` → `saveXMLSpeed`/`saveXMLTempoType` → subclass data → `writeEndElement()`.
- `loadXML`: validate the root name **and** the `Type` attribute, then `while (root.readNextStartElement())` dispatch on tag; `root.skipCurrentElement()` on unknown tags with `qWarning() << Q_FUNC_INFO`.
- Every enum gets a `xxxToString()` / `stringToXxx()` static pair (e.g. `Function::typeToString`/`stringToType`, `Universe::blendModeToString`/`stringToBlendMode`).

## Doc ownership

`Doc` owns all engine objects as raw pointers and deletes them in `clearContents()`/`~Doc`. To add a new owned object, mirror the existing blocks (`addFunction`, `addFixtureGroup`, `addChannelsGroup`, `addPalette`):

- `addX(X*, id = X::invalidId())` — connect `changed`→`slotXChanged`, store in `QMap<quint32,X*>`, assign ID, `emit xAdded(id)`, `setModified()`.
- `deleteX(id)`, `x(id)`, `xList()`, `createXId()`, `xAdded`/`xRemoved`/`xChanged` signals, `m_latestXId` member.

## DMX output pipeline

- `MasterTimer` ticks at **50 Hz** (`s_tick = 20` ms). Each tick: `timerTickFunctions()` calls `function->write(this, universes)` for running functions, then `timerTickDMXSources()` calls `source->writeDMX(...)`.
- A Function writes DMX by requesting one `QSharedPointer<GenericFader>` per `Universe` (`universe->requestFader()`), caching it in `m_fadersMap`, and updating channels via the thread-safe `fader->updateChannel(doc, universe, fxi, ch, lambda)` API. See `Scene::write()` (`scene.cpp`) and `RGBMatrix::getFader()` (`rgbmatrix.cpp`) as references.
- `FadeChannel` holds one channel's start/target/fade time; `GenericFader::write()` pushes values into the `Universe` buffer each tick.
- Flash path: `Function::flash()` → `timer->registerDMXSource(this)`; `unFlash()` unregisters. `Function::dismissAllFaders()` calls `requestDelete()` on all faders.

## Engine tests

- Location: `engine/test/<class>/` with `<class>_test.cpp`, `<class>_test.h`, `CMakeLists.txt`, `test.sh`. Register in `engine/test/CMakeLists.txt`.
- Test classes are `final : public QObject` with `private slots:` for `initTestCase`/`cleanupTestCase`/`init`/`cleanup` and each case.
- Tests use `#define private public` / `#define protected public` before including engine headers to reach internals, then `#undef` them.
- Use `MasterTimerStub` (`engine/test/mastertimer/`) and call `function->write(&timer, timer.m_universes)` manually to simulate ticks. `IOPluginStub` (`engine/test/iopluginstub/`) fakes a `QLCIOPlugin`.
- Resource paths come from `engine/test/common/resource_paths.h` (`INTERNAL_FIXTUREDIR`, `INTERNAL_SCRIPTDIR`).
- **Run via `./unittest.sh ui` or `ninja -C build check`** — never raw binaries from the source tree (resources must be copied into the build dir first).

## Style

Match the surrounding engine code: Allman braces, 4 spaces, `m_`/`s_` prefixes, `Q_DISABLE_COPY(Class)`, `final` on concrete classes, old-style `SIGNAL()`/`SLOT()` connects, `slotXxx` slot names, `Q_ASSERT` for pointer/state checks, `qDebug() << Q_FUNC_INFO` for logging, `== true`/`== false` comparisons, include guards (never `#pragma once`), Apache-2.0 header, `/** @addtogroup engine_... @{ */ ... /** @} */` grouping.

## Fork-specific engine changes — do not break

- **LuaJIT RGB engine**: `RGBScriptsCache` was changed to load `.lua` files and its `script()` returns `RGBLua*` (not `RGBScript*`). Do not reintroduce `.js` handling. `RGBAlgorithm::setStepFloat()` is a fork-added virtual (base no-op) — keep it so non-Lua algorithms still compile.
- **Global transition slider**: `Function::overrideFadeInSpeed()`/`overrideFadeOutSpeed()` return `doc()->globalTransitionTime()` when it is `> 0` and the function is not flashing. New Functions inherit this automatically — don't bypass it.
- **RGBMatrix 50 Hz continuous-phase engine**: `RGBMatrix::write()` uses `m_continuousPhase`/`m_speedMultiplier` and calls `setStepFloat()`. `RGBLua::rgbMap()` discards the integer step index and passes `m_stepFloat` (the phase) to Lua. `m_speedMultiplier` is serialized under the raw key `"SpeedMultiplier"` (not a `KXMLQLC...` macro) — preserve it or old workspaces lose the setting. Note: `RGBMatrix::previewMap()` (editor preview) does **not** call `setStepFloat()`, so continuous-phase scripts render statically in the preview.
- `engine/src/qlcconfig.h` is **generated** — edit `qlcconfig.h.in` / `qlcconfig.h.noroot.in`.
- Portuguese comments/messages in `rgblua.*` and CMake are intentional; leave as-is unless asked.
