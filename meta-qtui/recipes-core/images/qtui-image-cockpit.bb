SUMMARY = "Qtui Device Cockpit Embedded Linux Image"
DESCRIPTION = "A lean, fast-booting maintenance image that starts the Qtui Device Cockpit \
on the console: system overview, network details, systemd service management, and live \
journal, rendered as a terminal UI without any GPU or display-server overhead."
LICENSE = "MIT"

inherit core-image

IMAGE_INSTALL = "\
    packagegroup-core-boot \
    packagegroup-qtui-base \
    packagegroup-qtui-tools \
    qtui-device-cockpit \
    qtui-autostart \
    ${CORE_IMAGE_EXTRA_INSTALL} \
"

IMAGE_FEATURES += "ssh-server-dropbear"

IMAGE_OVERHEAD_FACTOR = "1.1"
IMAGE_ROOTFS_EXTRA_SPACE = "8192"

# Boot into the cockpit instead of the starter template.
qtui_cockpit_default_app() {
    sed -i 's|^QTUI_APP=.*|QTUI_APP="/usr/bin/qtui_device_cockpit"|' \
        ${IMAGE_ROOTFS}${sysconfdir}/default/qtui-app
}
ROOTFS_POSTPROCESS_COMMAND += "qtui_cockpit_default_app; "

# SDK generation packages
TOOLCHAIN_HOST_TASK:append = " nativesdk-packagegroup-qt6-toolchain-host"
TOOLCHAIN_TARGET_TASK:append = " packagegroup-qtui-base qtui-dev"
