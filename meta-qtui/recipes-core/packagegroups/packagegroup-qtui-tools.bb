SUMMARY = "Qtui Terminal Diagnostic & Development Tools"
DESCRIPTION = "Useful terminal diagnostic, multiplexing, and remote debugging utilities"
LICENSE = "MIT"

PACKAGE_ARCH = "${MACHINE_ARCH}"
inherit packagegroup

RDEPENDS:${PN} = "\
    tmux \
    htop \
    evtest \
    dropbear \
    openssh-sftp-server \
    fbset \
"
