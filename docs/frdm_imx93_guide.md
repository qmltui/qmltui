# NXP FRDM-i.MX93 Qtui Deployment & Development Guide

This guide walks through building, flashing, and running Qtui applications on the **NXP FRDM-i.MX93** development board using Yocto 5.0 LTS (Scarthgap) and Qt 6.8.

---

## 1. Prerequisites

- Host OS: Linux (Ubuntu 22.04 / 24.04, Debian 12, Fedora 39+, Arch)
- Python 3 with `kas` installed:
  ```sh
  pip install --user kas
  ```
- Tooling for flashing: `bmap-tools` (or `zstd` and `dd`)
- Serial terminal: `picocom`, `minicom`, or `screen`

---

## 2. Hardware Setup

1. **Board**: NXP FRDM-i.MX93 (14x14 package, Dual Cortex-A55).
2. **Debug Port**: Connect a USB Type-C cable from your PC to the onboard USB debug port (`J14`). This provides the serial UART interface (`/dev/ttyUSB0` / `/dev/ttyACM0`).
3. **Power**: Connect the 5V/3A USB-C power supply to the power port (`J1`).
4. **Boot Switches**: Ensure boot mode switches are configured for **MicroSD Boot** (check board silkscreen for SW1/SW2 MicroSD setting).

---

## 3. Building the Minimal Image with Kas

From the root of the `Qtui` repository:

```sh
kas build meta-qtui/kas/frdm-imx93.yml
```

To build the **Device Cockpit** maintenance image instead — it boots straight
into the `qtui_device_cockpit` console (system overview, network, systemd
services, live journal) — override the target:

```sh
kas build --target qtui-image-cockpit meta-qtui/kas/frdm-imx93.yml
```

### What Kas does automatically:
- Clones `poky` (Scarthgap 5.0 LTS).
- Clones `meta-openembedded` (`meta-oe`).
- Clones `meta-qt6` (branch `6.8`).
- Clones `meta-freescale` and `meta-arm`.
- Registers the in-tree `meta-qtui` layer.
- Builds the optimized `qtui-minimal` distro with zero GPU/Wayland overhead.
- Compiles the Linux kernel, U-Boot, Qt 6.8 Core/Qml, and `qtui_starter_template`.
- Produces compressed SD card image artifacts under:
  `build/tmp/deploy/images/imx93-14x14-lpddr4x-evk/qtui-image-minimal.wic.zst`

---

## 4. Flashing the MicroSD Card

Insert a MicroSD card (>= 8GB) into your host machine. Identify its device node (`lsblk`, e.g. `/dev/sdX`):

```sh
# Flash with bmaptool (recommended for speed)
bmaptool copy build/tmp/deploy/images/imx93-14x14-lpddr4x-evk/qtui-image-minimal.wic.zst /dev/sdX

# Or using zstdcat + dd
zstdcat build/tmp/deploy/images/imx93-14x14-lpddr4x-evk/qtui-image-minimal.wic.zst | sudo dd of=/dev/sdX bs=4M status=progress conv=fsync
```

---

## 5. Booting and First Run

1. Insert the MicroSD card into the FRDM-i.MX93 slot.
2. Open a serial terminal on your host PC:
   ```sh
   picocom -b 115200 /dev/ttyUSB0
   ```
3. Power on the board.
4. Within **< 2 seconds**, the Linux kernel boots and the `qtui-app.service` launches `qtui_starter_template` directly on your terminal window.

### Interaction:
- Press **[Space]** to increment the interactive counter and update the status bar.
- Press **[Q]** or **[Escape]** to quit the application.

---

## 6. Changing the Autostart Application

The boot service is controlled by `/etc/default/qtui-app`. To switch to the Industrial DIN-rail Gateway demo or a custom app:

```sh
# On the running board or via SSH:
vi /etc/default/qtui-app
```

Update `QTUI_APP`:
```sh
QTUI_APP="/usr/bin/qtui_din_gateway"
```

Restart the service:
```sh
systemctl restart qtui-app
```

---

## 7. Cross-Compiling Custom Apps with the SDK

### 1. Build and Install the SDK
On your host PC:
```sh
kas build meta-qtui/kas/frdm-imx93.yml -c populate_sdk
./build/tmp/deploy/sdk/poky-qtui-glibc-x86_64-qtui-image-minimal-cortexa55-imx93-toolchain-*.sh
```

### 2. Cross-Compile a CMake Project
```sh
# Source the toolchain environment
source /opt/poky-qtui/1.0.0/environment-setup-cortexa55-poky-linux

# Cross-compile your application
cd my_app/
cmake -B build-target
cmake --build build-target

# Deploy binary to the board via SCP / SFTP over Ethernet
scp build-target/my_app root@<board-ip>:/usr/bin/
```
