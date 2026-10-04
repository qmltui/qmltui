SUMMARY = "Qtui Application Autostart and TTY Management"
DESCRIPTION = "Systemd service and SysVinit scripts to launch a Qtui application on boot \
directly onto a serial UART or Linux framebuffer console."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "\
    file://qtui-app.service \
    file://qtui-app@.service \
    file://qtui-app \
    file://qtui-app.default \
    file://qtui-launcher.sh \
    file://autologin.conf \
    file://25-wlan.network \
    file://wpa_supplicant-wlan0.conf \
    file://vconsole.conf \
    file://dropbear.service \
"

S = "${WORKDIR}"

inherit systemd update-rc.d

SYSTEMD_SERVICE:${PN} = "qtui-app.service dropbear.service"
SYSTEMD_AUTO_ENABLE = "enable"

INITSCRIPT_NAME = "qtui-app"
INITSCRIPT_PARAMS = "defaults 99"

do_install() {
    # Install launcher script
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/qtui-launcher.sh ${D}${bindir}/qtui-launcher.sh

    # Install default config
    install -d ${D}${sysconfdir}/default
    install -m 0644 ${WORKDIR}/qtui-app.default ${D}${sysconfdir}/default/qtui-app

    # Install vconsole.conf for console font
    install -m 0644 ${WORKDIR}/vconsole.conf ${D}${sysconfdir}/vconsole.conf

    # Install Systemd service and serial auto-login drop-in
    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install -d ${D}${systemd_system_unitdir}
        install -m 0644 ${WORKDIR}/qtui-app.service ${D}${systemd_system_unitdir}/qtui-app.service

        # Getty-replacement template for serial consoles; instances are
        # enabled per image or on the device, e.g. qtui-app@ttyLP0.service.
        install -m 0644 ${WORKDIR}/qtui-app@.service ${D}${systemd_system_unitdir}/qtui-app@.service

        install -d ${D}${systemd_system_unitdir}/serial-getty@.service.d
        install -m 0644 ${WORKDIR}/autologin.conf ${D}${systemd_system_unitdir}/serial-getty@.service.d/autologin.conf

        # Install systemd-networkd Wi-Fi DHCP configuration
        install -d ${D}${sysconfdir}/systemd/network
        install -m 0644 ${WORKDIR}/25-wlan.network ${D}${sysconfdir}/systemd/network/25-wlan.network

        # Install Dropbear standalone daemon service
        install -m 0644 ${WORKDIR}/dropbear.service ${D}${systemd_system_unitdir}/dropbear.service

        # Install default wpa_supplicant template
        install -d ${D}${sysconfdir}/wpa_supplicant
        install -m 0600 ${WORKDIR}/wpa_supplicant-wlan0.conf ${D}${sysconfdir}/wpa_supplicant/wpa_supplicant-wlan0.conf
    fi

    # Install SysVinit script
    if ${@bb.utils.contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
        install -d ${D}${sysconfdir}/init.d
        install -m 0755 ${WORKDIR}/qtui-app ${D}${sysconfdir}/init.d/qtui-app
    fi
}

FILES:${PN} += "\
    ${bindir}/qtui-launcher.sh \
    ${sysconfdir}/default/qtui-app \
    ${sysconfdir}/vconsole.conf \
    ${systemd_system_unitdir}/qtui-app.service \
    ${systemd_system_unitdir}/qtui-app@.service \
    ${systemd_system_unitdir}/dropbear.service \
    ${systemd_system_unitdir}/serial-getty@.service.d/autologin.conf \
    ${sysconfdir}/systemd/network/25-wlan.network \
    ${sysconfdir}/wpa_supplicant/wpa_supplicant-wlan0.conf \
    ${sysconfdir}/init.d/qtui-app \
"
