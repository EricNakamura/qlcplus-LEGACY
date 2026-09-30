cmake_minimum_required(VERSION 3.16)
project(qlcplus VERSION 4.14.4 LANGUAGES C CXX)

option(qmlui "Build for QLC+ 5 QML UI" OFF)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "Builds in-source são proibidos! Delete o cache e use a pasta build.")
endif()

# Set Release build type by default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# --- Dev build speed options -------------------------------------------------
# Runs macdeployqt + ad-hoc codesign on 'install'. It is slow, so it is off by
# default and enabled explicitly by the macos-package preset.
option(QLC_MACOS_PACKAGE "Deploy and ad-hoc sign the macOS bundle on install" OFF)

# Wire the ccache compiler launcher when available. Enabled by default on Apple
# only, to avoid changing behavior on Linux/CI.
if(APPLE)
    set(QLC_USE_CCACHE_DEFAULT ON)
else()
    set(QLC_USE_CCACHE_DEFAULT OFF)
endif()
option(QLC_USE_CCACHE "Use ccache to speed up compilation when available" ${QLC_USE_CCACHE_DEFAULT})

if(QLC_USE_CCACHE AND NOT CMAKE_CXX_COMPILER_LAUNCHER)
    find_program(CCACHE_EXECUTABLE ccache)
    if(CCACHE_EXECUTABLE)
        set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
        message(STATUS "ccache encontrado: ${CCACHE_EXECUTABLE}")
    else()
        message(STATUS "ccache nao encontrado. Build sem cache de compilador.")
    endif()
endif()

# Keep line tables for stack traces but drop the large/slow full debug info.
# Added last so it overrides the -g coming from the build type flags.
option(QLC_LEAN_DEBUG "Use lightweight debug info (-gline-tables-only) on Apple" ON)
if(APPLE AND QLC_LEAN_DEBUG)
    add_compile_options("$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:-gline-tables-only>")
endif()

# Optional targets. Skipping tests and/or the standalone fixture editor speeds
# up day-to-day builds. Both default to ON so 'check' keeps working as before.
option(QLC_BUILD_TESTS "Configure and build the unit test targets" ON)
option(QLC_BUILD_FXE "Build the standalone fixture editor" ON)

# Precompiled headers (opt-in). Speed up incremental rebuilds of the heavy
# targets, but they reduce ccache effectiveness, hence OFF by default.
option(QLC_USE_PCH "Use precompiled headers on the heavy targets" OFF)

# macOS packaging paths used by the QLC_MACOS_PACKAGE step. Overridable from the
# command line so the packaging automation is no longer tied to one machine.
if(APPLE)
    set(QLC_MACOS_APP_PATH "$ENV{HOME}/QLC+.app" CACHE PATH "macOS .app bundle path used by the packaging step")
    set(QLC_MACOS_QT_LIBPATH "/opt/homebrew/lib" CACHE PATH "Extra library search path passed to macdeployqt")
    if(NOT MACDEPLOYQT_EXECUTABLE)
        find_program(MACDEPLOYQT_EXECUTABLE macdeployqt)
    endif()
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Prevent CMake make install strips off non-standard build paths
# Refer to https://stackoverflow.com/questions/7970544/cmake-make-install-output-cant-find-shared-qt-libraries-under-redhat
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

SET(INSTALL_ROOT "/" CACHE STRING "Installation root directory")

set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(UNIX)
    if (APPLE)
        set(iokit ON)

if(APPLE AND QLC_MACOS_PACKAGE)
    install(CODE "
        message(STATUS \"==================================================\")
        message(STATUS \"🧹 LIMPEZA: Apagando versão antiga do QLC+.app...\")
        message(STATUS \"==================================================\")
        file(REMOVE_RECURSE \"${QLC_MACOS_APP_PATH}\")
    ")
endif()
    else()
        set(udev ON)
    endif()
endif()

if (ANDROID OR IOS)
    set(qmlui ON)
endif()

if (ANDROID)
    set(ANDROID_PACKAGE_STAGING_DIR "${CMAKE_BINARY_DIR}/android-package")
    # Tell Qt/Gradle to use the build-tree template
    set(QT_ANDROID_PACKAGE_SOURCE_DIR "${ANDROID_PACKAGE_STAGING_DIR}" CACHE PATH "" FORCE)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Set up AUTOMOC and some sensible defaults for runtime execution
# When using Qt 6.3, you can replace the code block below with
# qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)
include(GNUInstallDirs)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Multimedia MultimediaWidgets Network PrintSupport Qml Quick Svg Test Widgets LinguistTools)
if(qmlui)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS 3DCore 3DInput 3DLogic 3DQuick 3DQuickExtras 3DRender WebSockets)
    if(ANDROID)
        find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Concurrent OpenGL)
    endif()

endif()

message("Found Qt version ${QT_VERSION_MAJOR}: ${QT_DIR}")

if(QT_VERSION_MAJOR EQUAL 5)
    find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS Script)
endif()

find_package(PkgConfig REQUIRED)

include(./variables.cmake)
include(./coverage.cmake)

if((((QT_VERSION_MAJOR LESS 5)) AND (APPLE)))
    pkg_check_modules(PORTAUDIO_2 IMPORTED_TARGET portaudio-2.0)
endif()

if(WIN32)
    # Prefix all shared libraries with ''.
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

add_subdirectory(hotplugmonitor)
add_subdirectory(engine)
add_subdirectory(resources)
add_subdirectory(plugins)
add_subdirectory(webaccess)
if(qmlui)
    message("Building QLC+ 5 QML UI")
    add_subdirectory(qmlui)
else()
    message("Building QLC+ 4 QtWidget UI")
    add_subdirectory(ui)
    add_subdirectory(main)
    if(QLC_BUILD_FXE)
        add_subdirectory(fixtureeditor)
    endif()
    
endif()
if(APPLE AND NOT qmlui)
    add_subdirectory(launcher)
endif()

# Unit testing thru "make check"
if(qmlui)
    if(WIN32)
       add_custom_target(unittests
            COMMAND unittest.bat "qmlui" ${CMAKE_CURRENT_BINARY_DIR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    else()
       add_custom_target(unittests
            COMMAND ./unittest.sh "qmlui" ${CMAKE_CURRENT_BINARY_DIR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
else()
    if(WIN32)
       add_custom_target(unittests
            COMMAND unittest.bat "ui" ${CMAKE_CURRENT_BINARY_DIR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    else()
       add_custom_target(unittests
            COMMAND ./unittest.sh "ui" ${CMAKE_CURRENT_BINARY_DIR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endif()

add_custom_target(check
    DEPENDS unittests
)

# Unit test coverage measurement
if(WIN32)
    add_custom_target(coverage
        COMMAND @echo Get a better OS.
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
else()
    if(qmlui)
       add_custom_target(coverage
            COMMAND ./coverage.sh "qmlui"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    else()
       add_custom_target(coverage
            COMMAND ./coverage.sh "ui"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endif()

add_custom_target(lcov
    DEPENDS coverage
)

# Translations
# NOTE: this target is deliberately NOT part of ALL. Running lrelease on every
# build added several seconds to each 'ninja' invocation. It is generated on
# demand and lazily refreshed by the install rule below.
if (qmlui)
  add_custom_target(translations
    COMMAND ./translate.sh release qmlui
    COMMENT "Translating qmlui translations..."
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
else ()
  add_custom_target(translations
    COMMAND ./translate.sh release ui
    COMMENT "Translating ui translations..."
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

## Install the qm files
if (appimage)
  set(translations_install_path "${TARGET_DIR}/${INSTALLROOT}/${TRANSLATIONDIR}")
else ()
  set(translations_install_path "${INSTALLROOT}/${TRANSLATIONDIR}")
endif()

# Define the pattern for the .qm files
set(translations_qm_pattern "${CMAKE_CURRENT_SOURCE_DIR}/qlcplus_*.qm")

# Install command using a custom script.
# Since the translations target is not part of ALL anymore, build it here so
# that 'install' always ships up-to-date .qm files (POSIX only, translate.sh
# is a shell script).
install(CODE "
  if(NOT WIN32)
    execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target translations)
  endif()
  file(GLOB qm_files \"${translations_qm_pattern}\")
  foreach(file IN LISTS qm_files)
    file(INSTALL DESTINATION \"${translations_install_path}\" TYPE FILE FILES \"\${file}\")
  endforeach()
" COMPONENT translations)

## Clean the generated files
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_CURRENT_BINARY_DIR}/*.qm")

# run
if(UNIX)
    if(qmlui)
       add_custom_target(run
            COMMAND ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=engine/src:$ENV{LD_LIBRARY_PATH} qmlui/qlcplus-qml
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    else()
       add_custom_target(run
            COMMAND ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=engine/src:ui/src:webaccess/src:$ENV{LD_LIBRARY_PATH} main/qlcplus
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    endif()
endif()

# run-fxe
if(UNIX AND NOT qmlui)
    add_custom_target(run-fxe
        COMMAND ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=engine/src:ui/src:webaccess/src:$ENV{LD_LIBRARY_PATH} ./fixtureeditor/qlcplus-fixtureeditor
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

# doxygen
if(UNIX)
    add_custom_target(doxygen
        COMMAND cd resources/doxygen && rm -rf html/ && doxygen qlcplus.dox
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# uninstall target
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()

SET(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "qlcplus")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Massimo Callegari") #required
INCLUDE(CPack)

# Leave this on the last row of this file
add_subdirectory(platforms)

# HELPER: PRINT ALL VARIABLES KNOWN BY CMAKE

#message("QTDIR: ${QT_DIR}")
#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()
