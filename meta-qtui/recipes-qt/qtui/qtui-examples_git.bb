SUMMARY = "Qtui Example Applications Suite"
DESCRIPTION = "Showcase applications for Qtui including DIN-rail gateway, POS cashier, \
container fleet monitor, and markdown viewer."
HOMEPAGE = "https://github.com/yosriayed/Qtui"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${THISDIR}/../../../LICENSE;md5=a874259b434da54f70afe4ab6fd5f174"

DEPENDS = "qtbase qtdeclarative qtdeclarative-native qtui utf8proc"
RDEPENDS:${PN} = "qtui"

PV = "0.2.0"

S = "${WORKDIR}/git"
B = "${WORKDIR}/build"

inherit qt6-cmake pkgconfig

EXTRA_OECMAKE += "\
    -DQTUI_USE_SYSTEM_UTF8PROC=ON \
    -DQTUI_BUILD_TESTS=OFF \
    -DQTUI_BUILD_EXAMPLES=ON \
    -DQTUI_INSTALL_EXAMPLES=ON \
    -DQTUI_ENABLE_BEHAVIOR=ON \
"

do_unpack[nostamp] = "1"
do_configure[nostamp] = "1"

python do_unpack() {
    import shutil
    import os
    
    src_dir = os.path.abspath(d.getVar('THISDIR') + '/../../..')
    s_dir = d.getVar('S')
    
    if os.path.exists(s_dir):
        shutil.rmtree(s_dir)
    os.makedirs(s_dir, exist_ok=True)
    
    for item in ['src', 'examples', 'cmake', 'CMakeLists.txt', 'LICENSE']:
        src_path = os.path.join(src_dir, item)
        dst_path = os.path.join(s_dir, item)
        if os.path.isdir(src_path):
            shutil.copytree(src_path, dst_path, symlinks=True)
        elif os.path.isfile(src_path):
            shutil.copy2(src_path, dst_path)
}

# The device cockpit ships as its own package so lean images can install the
# maintenance console without the demo applications.
PACKAGES =+ "qtui-device-cockpit"

SUMMARY:qtui-device-cockpit = "Qtui device maintenance and commissioning console"
FILES:qtui-device-cockpit = "${bindir}/qtui_device_cockpit"
# Live providers use journalctl and the systemd/D-Bus system bus at runtime.
RDEPENDS:qtui-device-cockpit = "qtui systemd dbus"

FILES:${PN} += "\
    ${bindir}/qtui_din_gateway \
    ${bindir}/qtui_pos_cashier \
    ${bindir}/qtui_container_fleet \
    ${bindir}/qtui_markdown_viewer \
    ${bindir}/qtui_starter_template \
"
