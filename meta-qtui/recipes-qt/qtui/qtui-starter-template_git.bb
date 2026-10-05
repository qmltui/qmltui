SUMMARY = "Qtui Starter Template Application"
DESCRIPTION = "Boot to Qt style device setup application for Qtui on embedded Linux: \
Wi-Fi and network configuration for app deployment, plus device info pages"
HOMEPAGE = "https://github.com/yosriayed/Qtui"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${THISDIR}/../../../LICENSE;md5=a874259b434da54f70afe4ab6fd5f174"

DEPENDS = "qtbase qtdeclarative qtdeclarative-native qtui"
RDEPENDS:${PN} = "qtui"

PV = "1.0.0"

S = "${WORKDIR}/git"
B = "${WORKDIR}/build"

inherit qt6-cmake pkgconfig

PACKAGES =+ "${PN}-benchmark"

do_unpack[nostamp] = "1"
do_configure[nostamp] = "1"

python do_unpack() {
    import shutil
    import os
    
    src_dir = os.path.abspath(d.getVar('THISDIR') + '/../../../examples/apps/starter_template')
    s_dir = d.getVar('S')
    
    if os.path.exists(s_dir):
        shutil.rmtree(s_dir)
    os.makedirs(s_dir, exist_ok=True)
    
    for item in os.listdir(src_dir):
        if item in ['build', '.git']:
            continue
        src_path = os.path.join(src_dir, item)
        dst_path = os.path.join(s_dir, item)
        if os.path.isdir(src_path):
            shutil.copytree(src_path, dst_path, symlinks=True)
        elif os.path.isfile(src_path):
            shutil.copy2(src_path, dst_path)
}

FILES:${PN} += "${bindir}/qtui_starter_template"
FILES:${PN}-benchmark = "${bindir}/qtui_starter_template_benchmark"
RDEPENDS:${PN}-benchmark = "${PN}"
