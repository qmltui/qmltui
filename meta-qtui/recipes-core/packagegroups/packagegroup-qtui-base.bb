SUMMARY = "Qtui Base Runtime Packagegroup"
DESCRIPTION = "Minimum runtime packages required to execute Qtui applications on embedded Linux"
LICENSE = "MIT"

PACKAGE_ARCH = "${MACHINE_ARCH}"
inherit packagegroup

RDEPENDS:${PN} = "\
    qtbase \
    qtdeclarative \
    qtui \
    utf8proc \
    ncurses-terminfo-base \
    ca-certificates \
    gpm \
    tslib \
    tslib-calibrate \
    tslib-tests \
    libevdev \
    evtest \
    usbutils \
    i2c-tools \
    wpa-supplicant \
    iw \
    wireless-regdb-static \
    rfkill \
    kernel-modules \
    kernel-module-nxp-wlan \
    firmware-nxp-wifi-nxpiw612-sdio \
    kbd \
    kbd-consolefonts \
    kbd-keymaps \
    ncurses-terminfo \
"
