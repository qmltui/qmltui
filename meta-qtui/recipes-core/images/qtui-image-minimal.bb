SUMMARY = "Qtui Minimal Headless & Console Embedded Linux Image"
DESCRIPTION = "An ultra-lean, fast-booting Linux image designed for Qtui terminal applications. \
Features zero GPU/display-server overhead, sub-2-second boot time, and boots directly into \
the Qtui starter application on the serial console or display."
LICENSE = "MIT"

inherit core-image

# Essential base system and networking
IMAGE_INSTALL = "\
    packagegroup-core-boot \
    packagegroup-qtui-base \
    packagegroup-qtui-tools \
    qtui-starter-template \
    qtui-autostart \
    ${CORE_IMAGE_EXTRA_INSTALL} \
"

# Keep the image minimal and compact
IMAGE_FEATURES += "ssh-server-dropbear"

# Do not include debug tweaks by default in minimal production image
# Users can add IMAGE_FEATURES:append = " debug-tweaks" in local.conf for empty password

# Overhead factor and disk sizing
IMAGE_OVERHEAD_FACTOR = "1.1"
IMAGE_ROOTFS_EXTRA_SPACE = "8192"

# SDK generation packages
TOOLCHAIN_HOST_TASK:append = " nativesdk-packagegroup-qt6-toolchain-host"
TOOLCHAIN_TARGET_TASK:append = " packagegroup-qtui-base qtui-dev"
