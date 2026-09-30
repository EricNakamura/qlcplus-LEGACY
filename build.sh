#!/bin/bash
#
# Q Light Controller Plus - macOS build helper
#
# Wraps the CMake workflow presets (CMakePresets.json) so that building is a
# single short command instead of a long cmake invocation.
#
# Usage:  ./build.sh [debug|fast|release|package]
#
#   debug    (default) full Debug build (tests + fixture editor), inside ./build
#   fast     Debug build without tests and without the fixture editor
#   release  RelWithDebInfo build
#   package  Release build + macOS bundle deploy/sign (run install afterwards)
#
# You can also pass any preset name directly, e.g. ./build.sh macos-debug-fast.

set -euo pipefail

cd "$(dirname "$0")"

case "${1:-debug}" in
    debug)   preset="macos-debug" ;;
    fast)    preset="macos-debug-fast" ;;
    release) preset="macos-release" ;;
    package) preset="macos-package" ;;
    macos-*) preset="$1" ;;
    *)
        echo "Unknown preset '${1}'." >&2
        echo "Usage: $0 [debug|fast|release|package]" >&2
        exit 1
        ;;
esac

echo "==> cmake --workflow --preset ${preset}"
exec cmake --workflow --preset "${preset}"
