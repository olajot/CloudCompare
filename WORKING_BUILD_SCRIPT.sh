sudo echo 'building ur application. ur welcome 😌'
cd build && \
cmake -DCMAKE_PREFIX_PATH=$(brew --prefix qt6) \
      -DCMAKE_CXX_FLAGS="-DCC_MAC_DEV_PATHS" \
      -DPLUGIN_STANDARD_QSEGMENTER=ON \
      .. && \
cmake --build . --target qSegmenter --parallel 7 && \
sudo cmake --install . && \
# shitty fix but it works (for now)
sudo install_name_tool -change /opt/homebrew/opt/qtbase/lib/QtWidgets.framework/Versions/A/QtWidgets @rpath/QtWidgets.framework/Versions/A/QtWidgets /usr/local/CloudCompare/CloudCompare.app/Contents/PlugIns/ccPlugins/libqSegmenter.dylib

sudo install_name_tool -change /opt/homebrew/opt/qtbase/lib/QtGui.framework/Versions/A/QtGui @rpath/QtGui.framework/Versions/A/QtGui /usr/local/CloudCompare/CloudCompare.app/Contents/PlugIns/ccPlugins/libqSegmenter.dylib

sudo install_name_tool -change /opt/homebrew/opt/qtbase/lib/QtCore.framework/Versions/A/QtCore @rpath/QtCore.framework/Versions/A/QtCore /usr/local/CloudCompare/CloudCompare.app/Contents/PlugIns/ccPlugins/libqSegmenter.dylib
