SUMMARY = "Qtui Showcase & Demo Embedded Linux Image"
DESCRIPTION = "A full showcase image featuring all Qtui example applications (DIN gateway, \
POS cashier, container fleet monitor, markdown viewer), development tools, and debug tweaks."
LICENSE = "MIT"

inherit core-image

IMAGE_INSTALL = "\
    packagegroup-core-boot \
    packagegroup-qtui-base \
    packagegroup-qtui-tools \
    packagegroup-qtui-examples \
    qtui-starter-template-benchmark \
    qtui-autostart \
    ${CORE_IMAGE_EXTRA_INSTALL} \
"

# Development conveniences
IMAGE_FEATURES += "\
    ssh-server-dropbear \
    debug-tweaks \
    package-management \
"

IMAGE_OVERHEAD_FACTOR = "1.2"
IMAGE_ROOTFS_EXTRA_SPACE = "16384"

# SDK generation configuration
TOOLCHAIN_HOST_TASK:append = " nativesdk-packagegroup-qt6-toolchain-host"
TOOLCHAIN_TARGET_TASK:append = " packagegroup-qtui-base packagegroup-qtui-examples qtui-dev"
