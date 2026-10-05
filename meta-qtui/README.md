# OpenEmbedded / Yocto BSP Layer for Qtui (`meta-qtui`)

This layer provides minimal Linux images, recipes, autostart services, and cross-compilation toolchains to build and run **Qtui** retained-mode terminal applications across various embedded boards.

---

## Key Benefits for Embedded Linux

- **Zero GPU Overhead**: Qtui applications render rich, reactive QML user interfaces directly to character terminals (`/dev/ttyLP0`, `/dev/tty1`, or SSH) using ANSI/VT escape sequences. No OpenGL, Vulkan, DRM/KMS display drivers, or Wayland/Weston compositors are required.
- **Ultra-Compact Rootfs**: `qtui-image-minimal` produces a rootfs under 40 MB.
- **Sub-2-Second Boot**: Instant startup from power-on directly into your Qtui application.
- **Low RAM Footprint**: Operates in ~10–20 MB total system RAM.
- **Turnkey Kas Automation**: 1-command reproducible builds for NXP FRDM-i.MX93, QEMU ARM64, and Raspberry Pi 4.

---

## Supported Boards & Machines

| Board | Machine Name | Kas Manifest | Console TTY |
| :--- | :--- | :--- | :--- |
| **NXP FRDM-i.MX93** | `imx93-14x14-lpddr4x-evk` | `kas/frdm-imx93.yml` | `/dev/ttyLP0` (115200 baud) |
| **NXP i.MX93 EVK** | `imx93-11x11-lpddr4x-evk` | `kas/imx93-evk.yml` | `/dev/ttyLP0` (115200 baud) |
| **QEMU ARM64** | `qemuarm64` | `kas/qemuarm64.yml` | Standard I/O |
| **Raspberry Pi 4** | `raspberrypi4-64` | `kas/rpi4.yml` | `/dev/ttyS0` / `/dev/ttyAMA0` |

---

## Quickstart with Kas

Install `kas`:
```sh
pip install kas
```

### 1. Build for NXP FRDM-i.MX93
```sh
kas build meta-qtui/kas/frdm-imx93.yml
```

### 2. Build for QEMU ARM64 & Run Locally
```sh
kas build meta-qtui/kas/qemuarm64.yml
runqemu qemuarm64 nographic
```

---

## Flashing the Image (NXP FRDM-i.MX93)

After the build completes, write the `.wic.zst` image to a microSD card:
```sh
# Locate the generated image
cd build/tmp/deploy/images/imx93frdm/

# Flash using bmaptool (fastest)
bmaptool copy qtui-image-minimal.wic.zst /dev/sdX

# Or using dd
zstdcat qtui-image-minimal.wic.zst | sudo dd of=/dev/sdX bs=4M status=progress conv=fsync
```

Insert the microSD card into the FRDM-i.MX93 board, connect the USB-C debug cable, and open your serial terminal:
```sh
picocom -b 115200 /dev/ttyUSB0
```

The system will boot up directly into the `qtui_starter_template` application.

---

## Generating the Cross-Compilation SDK

To build custom Qtui applications on your developer workstation:

```sh
# Generate the SDK
kas build meta-qtui/kas/frdm-imx93.yml -c populate_sdk

# Install the SDK on your host machine
./build/tmp/deploy/sdk/poky-qtui-glibc-x86_64-qtui-image-minimal-cortexa55-imx93-toolchain-*.sh
```

### Cross-Compiling a Custom Application with CMake:
```sh
# Source the SDK environment
source /opt/poky-qtui/1.0.0/environment-setup-cortexa55-poky-linux

# Configure and build your custom Qtui app
cmake -B build-cross -S my_qtui_app
cmake --build build-cross
```

For complete documentation on building external apps, automated testing (Host, CI & On-Target), and fast iteration without reflashing, see the **[Embedded Development & Testing Guide](docs/development-and-testing-workflow.md)**.

### Fast Dev-Loop Tool (`qtui-dev.sh`):
```sh
# Build and run any external project live on /dev/tty1 over SSH in < 2 seconds:
./meta-qtui/scripts/qtui-dev.sh dev /path/to/my_qtui_app 192.168.1.35
```

---

## Configuring the Autostart Service

The boot application and target terminal are configured in `/etc/default/qtui-app`:

```sh
# Target executable
QTUI_APP="/usr/bin/qtui_starter_template"

# Additional arguments
QTUI_ARGS=""

# Target terminal:
# /dev/ttyLP0 for USB-C serial debug console
# /dev/tty1 for physical LCD / HDMI framebuffer console
QTUI_TTY="/dev/ttyLP0"

# Terminal emulation
TERM="xterm-256color"
COLORTERM="truecolor"

# Optional: trade higher Wi-Fi power use for steadier SSH latency
QTUI_WIFI_LOW_LATENCY="0"
```

To switch to a different app (e.g. `qtui_din_gateway`), edit `/etc/default/qtui-app` and restart the service:
```sh
systemctl restart qtui-app
```

The starter manages association through `wpa_supplicant`; `systemd-networkd`
alone manages DHCP, addresses, and routes. Do not launch a second
`qtui_starter_template` over SSH while the boot service is running. The
production executable rejects that with exit status 2. The demo image includes
the non-mutating, Testing-linked `qtui_starter_template_benchmark` instead;
the minimal production image intentionally does not include it.

To opt into the low-latency Wi-Fi policy, set `QTUI_WIFI_LOW_LATENCY="1"` in
`/etc/default/qtui-app` and restart `qtui-app`. The launcher disables interface
power saving, and the starter reapplies the policy after association. Leave it
disabled for battery-sensitive deployments.

### Measuring TUI latency over SSH

Run the host-side harness after installing the demo image:

```sh
meta-qtui/scripts/measure-ssh-tui-latency.py \
    root@192.168.1.35 --output qtui-latency.json
```

It first runs a deterministic on-device benchmark, then keeps one SSH PTY open
for at least 30 alternating F1/F2 samples. The JSON report separates startup,
first-byte, settled-frame, and render-diff sizes from normal-interval ping
RTT/loss and records kernel, Wi-Fi link/RSSI, power-save, image, and executable
metadata. Local p95 above 20 ms is a failure. SSH settled-frame p95 above
75 ms is reported as a warning because radio and network conditions are not a
CI-stable property.

---

## Local Development with `devtool`

To test local in-tree code modifications without pushing to git:
```sh
kas shell meta-qtui/kas/frdm-imx93.yml -c "devtool modify qtui"
```
Edit your C++ or QML code in `Qtui/`, then rebuild:
```sh
kas shell meta-qtui/kas/frdm-imx93.yml -c "bitbake qtui-image-minimal"
```
