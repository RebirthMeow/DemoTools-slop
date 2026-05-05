#!/usr/bin/env bash
# build_metadata.sh
# One-shot build of jkdemometadata.exe.  Run from any cwd.
#
# Usage:
#   ./build_metadata.sh           # incremental build
#   ./build_metadata.sh clean     # full rebuild (make clean first)
#   ./build_metadata.sh install   # build + copy exe to D:/JSON
#
# Requires (MSYS2 mingw64 shell):
#   pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-jansson
# On Linux:
#   apt install build-essential libjansson-dev

set -e
cd "$(dirname "$(readlink -f "$0")")/JKDemoMetadata"

# Make sure the mingw64 toolchain is on PATH even when this script is invoked
# from a plain login bash (which gives us /usr/bin only).  Adjust if your
# MSYS2 install is somewhere other than /mingw64.
if [ -d /mingw64/bin ] && ! command -v gcc >/dev/null 2>&1; then
  export PATH="/mingw64/bin:$PATH"
fi

# Sanity-check the toolchain before make starts so the error message is
# informative instead of "gcc: No such file or directory".
if ! command -v gcc >/dev/null 2>&1; then
  echo "ERROR: gcc not found on PATH." >&2
  echo "Make sure you have installed the mingw64 toolchain:" >&2
  echo "  pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-jansson" >&2
  echo "and that this script is being run from a shell where /mingw64/bin is reachable." >&2
  exit 1
fi

JOBS=$(nproc 2>/dev/null || echo 4)

case "${1:-build}" in
  clean)
    echo ">> make clean"
    make clean
    echo ">> make -j${JOBS}"
    make -j"${JOBS}"
    ;;
  install)
    echo ">> make -j${JOBS}"
    make -j"${JOBS}"
    DEST="/d/JSON/jkdemometadata.exe"
    if [ -f "${DEST}" ]; then
      cp -f jkdemometadata.exe "${DEST}"
      echo ">> installed to ${DEST}"
    else
      # try Windows-native path under MSYS2 mount
      if [ -d "/d/JSON" ]; then
        cp -f jkdemometadata.exe "/d/JSON/"
        echo ">> installed to D:/JSON/jkdemometadata.exe"
      else
        echo "!! D:/JSON not mounted; exe is at $(pwd)/jkdemometadata.exe"
      fi
    fi
    ;;
  build|*)
    echo ">> make -j${JOBS}"
    make -j"${JOBS}"
    ;;
esac

ls -la jkdemometadata*
echo ">> done"
