SUMMARY = "Retained-mode terminal UI framework for Qt 6"
DESCRIPTION = "Qtui exposes terminal items, layouts, controls, views, animation, and widgets \
as QML modules while using the Qt event loop, rendering high-performance ANSI/VT terminal UIs \
without requiring 3D GPU acceleration, Wayland, or X11."
HOMEPAGE = "https://github.com/yosriayed/Qtui"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a874259b434da54f70afe4ab6fd5f174"

DEPENDS = "qtbase qtdeclarative qtdeclarative-native utf8proc"

inherit qt6-cmake pkgconfig

EXTRA_OECMAKE += "\
    -DQTUI_USE_SYSTEM_UTF8PROC=ON \
    -DQTUI_BUILD_TESTS=OFF \
    -DQTUI_BUILD_EXAMPLES=OFF \
    -DQTUI_ENABLE_BEHAVIOR=ON \
    -DQTUI_ENABLE_TEST_RUNNER=ON \
"

PACKAGECONFIG ??= "terminalhost"
PACKAGECONFIG[terminalhost] = "-DQTUI_BUILD_TERMINAL_HOST=ON -DQTUI_USE_SYSTEM_LIBVTERM=ON,-DQTUI_BUILD_TERMINAL_HOST=OFF,libvterm"

FILES_SOLIBSDEV = ""
SOLIBS = ".so*"

# QML modules and shared libraries packaging
FILES:${PN} += "\
    ${libdir}/libQtui*.so* \
    ${libdir}/qml/Qtui \
"

FILES:${PN}-dev = "\
    ${includedir}/qtui \
    ${libdir}/cmake/Qtui \
"

INSANE_SKIP:${PN} += "dev-so"

FILES:${PN}-dbg += "\
    ${libdir}/qml/Qtui/*/.debug \
    ${libdir}/qml/Qtui/*/*/.debug \
"
