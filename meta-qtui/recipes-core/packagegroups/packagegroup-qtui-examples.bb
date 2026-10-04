SUMMARY = "Qtui All Examples Packagegroup"
DESCRIPTION = "Includes all Qtui demo and showcase applications"
LICENSE = "MIT"

PACKAGE_ARCH = "${MACHINE_ARCH}"
inherit packagegroup

RDEPENDS:${PN} = "\
    qtui-starter-template \
    qtui-examples \
"
