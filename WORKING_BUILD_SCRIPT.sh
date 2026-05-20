#!/bin/bash
set -e

sudo echo # ask for password at start, not somewhere halfway

QT_PREFIX="$(brew --prefix qt6)"
LASZIP_PREFIX="$(brew --prefix laszip)"
CMAKE_FLAGS=(
  -DCMAKE_PREFIX_PATH="$QT_PREFIX;$LASZIP_PREFIX"
  -DCMAKE_CXX_FLAGS="-DCC_MAC_DEV_PATHS -I$LASZIP_PREFIX/include/laszip"
  -DPLUGIN_STANDARD_QSEGMENTER=ON
  -DPLUGIN_IO_QLAS=ON
  -DOPTION_BUILD_CCVIEWER=OFF
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)

fix_rpath() {
  local dylib="$1"
  sudo install_name_tool -change /opt/homebrew/opt/qtbase/lib/QtWidgets.framework/Versions/A/QtWidgets @rpath/QtWidgets.framework/Versions/A/QtWidgets "$dylib"
  sudo install_name_tool -change /opt/homebrew/opt/qtbase/lib/QtGui.framework/Versions/A/QtGui         @rpath/QtGui.framework/Versions/A/QtGui "$dylib"
  sudo install_name_tool -change /opt/homebrew/opt/qtbase/lib/QtCore.framework/Versions/A/QtCore       @rpath/QtCore.framework/Versions/A/QtCore "$dylib"
}

build() {
  local build_type="$1"
  local build_dir="build-$(echo "$build_type" | tr '[:upper:]' '[:lower:]')"

  echo "==> Configuring $build_type in $build_dir"
  mkdir -p "$build_dir"
  cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE="$build_type" "${CMAKE_FLAGS[@]}"

  echo "==> Building $build_type"
  cmake --build "$build_dir" --parallel 7

  echo "==> Installing $build_type"
  sudo cmake --install "$build_dir"

  echo "==> Fixing rpaths"
  fix_rpath /usr/local/CloudCompare/CloudCompare.app/Contents/PlugIns/ccPlugins/libqSegmenter.dylib
}

case "${1:-both}" in
  debug)   build Debug ;;
  release) build Release ;;
  both)    build Debug && build Release ;;
  *)       echo "Usage: $0 [debug|release|both]" && exit 1 ;;
esac

echo "Done, ur welcome 😘"