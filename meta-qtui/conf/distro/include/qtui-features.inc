# Qtui distro feature optimizations
# Removes heavy 3D GPU, display compositor, and multimedia bloat for headless/console TUI operation

# Strip GPU, windowing, audio, and test-suite overhead
DISTRO_FEATURES:remove = "\
    x11 \
    wayland \
    opengl \
    vulkan \
    3g \
    bluetooth \
    pulseaudio \
    alsa \
    ptest \
"

# Disable ptest and internal test suite builds
PTEST_ENABLED = "0"
EXTRA_OECMAKE:append:class-target = " -DFEATURE_tests=OFF"

# Keep essential features for networking, systemd, and terminal utilities
DISTRO_FEATURES:append = " ipv4 ipv6 systemd pam pci usbhost"

# Minimize qtbase: build only Core, headless Gui (for QColor/QImage), and Qml runtime
PACKAGECONFIG:remove:pn-qtbase = "\
    opengl \
    gl \
    gles2 \
    egl \
    eglfs \
    eglfs-egldevice \
    gbm \
    kms \
    vulkan \
    directfb \
    x11 \
    xcb \
    wayland \
    widgets \
    dbus \
    accessibility \
    cups \
    fontconfig \
    glib \
"

PACKAGECONFIG:append:pn-qtbase = " no-opengl linuxfb"

# Ensure qtdeclarative builds cleanly for QML engine
PACKAGECONFIG:append:pn-qtdeclarative = " qml-network"
