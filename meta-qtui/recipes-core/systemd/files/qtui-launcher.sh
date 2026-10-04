#!/bin/sh
# /usr/bin/qtui-launcher.sh
# Launcher script for Qtui applications on embedded Linux

# 1. Suppress all kernel console logging to avoid corrupting the TUI display
echo 1 > /proc/sys/kernel/printk 2>/dev/null || true
dmesg -n 1 2>/dev/null || true

# 2. Switch Linux virtual console to UTF-8 mode and load high-DPI font
/usr/bin/kbd_mode -u -C /dev/tty1 >/dev/null 2>&1 || true
/usr/bin/setfont /usr/share/consolefonts/latarcyrheb-sun32.psfu.gz -C /dev/tty1 >/dev/null 2>&1 || \
/usr/bin/setfont /usr/share/consolefonts/Lat2-Terminus16.psfu.gz -C /dev/tty1 >/dev/null 2>&1 || true
printf "\033%%G" > /dev/tty1 2>/dev/null || true
printf "\033%%G" 2>/dev/null || true

# 3. Prepare wpa_supplicant configuration directory and files
mkdir -p /var/run/wpa_supplicant /etc/wpa_supplicant
if [ ! -f /etc/wpa_supplicant/wpa_supplicant.conf ]; then
    printf "ctrl_interface=/var/run/wpa_supplicant\nctrl_interface_group=0\nupdate_config=1\n" > /etc/wpa_supplicant/wpa_supplicant.conf
fi
cp -f /etc/wpa_supplicant/wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant-mlan0.conf 2>/dev/null || true
cp -f /etc/wpa_supplicant/wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant-wlan0.conf 2>/dev/null || true

# 4. Bring up wireless interfaces completely silently
rfkill unblock wifi >/dev/null 2>&1 || true
rfkill unblock all >/dev/null 2>&1 || true
ip link set mlan0 up >/dev/null 2>&1 || ip link set wlan0 up >/dev/null 2>&1 || true

# 5. Start wpa_supplicant daemon completely silently in background
if ! pidof wpa_supplicant >/dev/null 2>&1; then
    wpa_supplicant -B -qq -i mlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf -C /var/run/wpa_supplicant >/dev/null 2>&1 || \
    wpa_supplicant -B -qq -i wlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf -C /var/run/wpa_supplicant >/dev/null 2>&1 || true
fi

# 6. Load environment defaults
if [ -f /etc/default/qtui-app ]; then
    . /etc/default/qtui-app
fi

QTUI_APP="${QTUI_APP:-/usr/bin/qtui_starter_template}"
QTUI_ARGS="${QTUI_ARGS:-}"
export TERM="linux"
export COLORTERM="truecolor"
export LANG="C.UTF-8"
export LC_ALL="C.UTF-8"
export QML_IMPORT_PATH="/usr/lib/qml:/usr/lib/qt6/qml"
export QML2_IMPORT_PATH="/usr/lib/qml:/usr/lib/qt6/qml"

# Verify application exists and is executable
if [ ! -x "$QTUI_APP" ]; then
    echo "Qtui Launcher: Application $QTUI_APP not found or not executable!" >&2
    exit 1
fi

# 7. Clear screen completely and launch pure terminal application
clear > /dev/tty1 2>/dev/null || clear 2>/dev/null || true
exec "$QTUI_APP" $QTUI_ARGS
