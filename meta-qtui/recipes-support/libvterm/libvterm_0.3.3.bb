SUMMARY = "An abstract library implementation of a terminal emulator"
HOMEPAGE = "http://www.leonerd.org.uk/code/libvterm/"
DESCRIPTION = "An abstract library implementation of a terminal emulator with support for Unicode and modern terminal extensions."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=be5681ffe0dc58ccc9756bc6260fe0cd"

SRC_URI = "http://www.leonerd.org.uk/code/libvterm/libvterm-${PV}.tar.gz"
SRC_URI[sha256sum] = "09156f43dd2128bd347cbeebe50d9a571d32c64e0cf18d211197946aff7226e0"

S = "${WORKDIR}/libvterm-${PV}"

inherit pkgconfig

do_compile() {
    cd ${S}
    ${CC} ${CFLAGS} ${LDFLAGS} -fPIC -shared -std=c99 -Wall -Iinclude \
        src/encoding.c \
        src/keyboard.c \
        src/mouse.c \
        src/parser.c \
        src/pen.c \
        src/screen.c \
        src/state.c \
        src/unicode.c \
        src/vterm.c \
        -Wl,-soname,libvterm.so.0 \
        -o libvterm.so.0.3.3

    ln -sf libvterm.so.0.3.3 libvterm.so.0
    ln -sf libvterm.so.0.3.3 libvterm.so

    # Generate pkg-config file
    sed -e "s|@LIBDIR@|${libdir}|g" \
        -e "s|@INCDIR@|${includedir}|g" \
        vterm.pc.in > vterm.pc
}

do_install() {
    cd ${S}
    install -d ${D}${includedir}
    install -m 0644 include/vterm.h ${D}${includedir}/vterm.h
    install -m 0644 include/vterm_keycodes.h ${D}${includedir}/vterm_keycodes.h

    install -d ${D}${libdir}
    install -m 0755 libvterm.so.0.3.3 ${D}${libdir}/libvterm.so.0.3.3
    ln -sf libvterm.so.0.3.3 ${D}${libdir}/libvterm.so.0
    ln -sf libvterm.so.0.3.3 ${D}${libdir}/libvterm.so

    install -d ${D}${libdir}/pkgconfig
    install -m 0644 vterm.pc ${D}${libdir}/pkgconfig/vterm.pc
}

BBCLASSEXTEND = "native nativesdk"
