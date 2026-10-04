FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " \
    file://imx93-11x11-evk-lvds-it6263.dts \
"

do_configure:prepend() {
    if [ -f "${WORKDIR}/imx93-11x11-evk-lvds-it6263.dts" ]; then
        cp "${WORKDIR}/imx93-11x11-evk-lvds-it6263.dts" "${S}/arch/arm64/boot/dts/freescale/imx93-11x11-evk-lvds-it6263.dts"
    fi
}

do_deploy:append() {
    if [ -f "${DEPLOYDIR}/imx93-11x11-evk-lvds-it6263.dtb" ]; then
        cp -f "${DEPLOYDIR}/imx93-11x11-evk-lvds-it6263.dtb" "${DEPLOYDIR}/imx93-11x11-evk.dtb"
    fi
}
