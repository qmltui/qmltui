#!/bin/sh
# /usr/bin/qtui-launcher.sh
# Launcher script for Qtui applications on embedded Linux

# 1. Suppress all kernel console logging to avoid corrupting the TUI display
echo 1 > /proc/sys/kernel/printk 2>/dev/null || true
dmesg -n 1 2>/dev/null || true

# 2. Load environment defaults before preparing network interfaces.
if [ -f /etc/default/qtui-app ]; then
    . /etc/default/qtui-app
fi

QTUI_APP="${QTUI_APP:-/usr/bin/qtui_starter_template}"
QTUI_ARGS="${QTUI_ARGS:-}"
QTUI_WIFI_LOW_LATENCY="${QTUI_WIFI_LOW_LATENCY:-0}"
export QTUI_WIFI_LOW_LATENCY

# 3. Switch Linux virtual console to UTF-8 mode and load high-DPI font
/usr/bin/kbd_mode -u -C /dev/tty1 >/dev/null 2>&1 || true
/usr/bin/setfont /usr/share/consolefonts/latarcyrheb-sun32.psfu.gz -C /dev/tty1 >/dev/null 2>&1 || \
/usr/bin/setfont /usr/share/consolefonts/Lat2-Terminus16.psfu.gz -C /dev/tty1 >/dev/null 2>&1 || true
printf "\033%%G" > /dev/tty1 2>/dev/null || true
printf "\033%%G" 2>/dev/null || true

# 4. Prepare persistent wpa_supplicant configuration directory and files
mkdir -p /var/run/wpa_supplicant /etc/wpa_supplicant
if [ ! -f /etc/wpa_supplicant/wpa_supplicant.conf ]; then
    printf "ctrl_interface=/var/run/wpa_supplicant\nctrl_interface_group=0\nupdate_config=1\n" > /etc/wpa_supplicant/wpa_supplicant.conf
fi
if [ ! -f /etc/wpa_supplicant/wpa_supplicant-mlan0.conf ]; then
    cp -f /etc/wpa_supplicant/wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant-mlan0.conf 2>/dev/null || true
fi
if [ ! -f /etc/wpa_supplicant/wpa_supplicant-wlan0.conf ]; then
    cp -f /etc/wpa_supplicant/wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant-wlan0.conf 2>/dev/null || true
fi

# 5. Start wpa_supplicant daemon in background as soon as wireless hardware finishes initialization
(
    for i in $(seq 1 30); do
        if [ -d /sys/class/net/mlan0 ] || [ -d /sys/class/net/wlan0 ]; then
            iface="mlan0"
            [ -d /sys/class/net/wlan0 ] && iface="wlan0"
            ip link set $iface up >/dev/null 2>&1 || true
            if [ "$QTUI_WIFI_LOW_LATENCY" = "1" ]; then
                iw dev $iface set power_save off >/dev/null 2>&1 || true
            fi
            if ! pidof wpa_supplicant >/dev/null 2>&1; then
                wpa_supplicant -B -qq -i $iface -c /etc/wpa_supplicant/wpa_supplicant-$iface.conf -C /var/run/wpa_supplicant >/dev/null 2>&1 || true
            fi
            break
        fi
        sleep 0.5
    done
) &

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
