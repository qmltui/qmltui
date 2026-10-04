# Device Cockpit

A maintenance and commissioning console for headless embedded Linux devices,
rendered over the serial console or SSH. See [DESIGN.md](DESIGN.md) for the
architecture and milestone plan; this directory currently contains the four
Milestone 1 pages — Overview, Network, Services, and Logs — each backed by a
mock provider (for tests and demos) and a live one.

## Run it

From the Qtui build tree, in any terminal:

```sh
# Simulated device (no root, deterministic values)
QTUI_COCKPIT_MOCK=1 ./build/examples/apps/device_cockpit/qtui_device_cockpit

# Live system (reads /proc, /sys, mounts, and network interfaces)
./build/examples/apps/device_cockpit/qtui_device_cockpit
```

Keys: `F1`–`F4` (or `1`–`4`) switch pages, `Q` quits.

## Structure

- `CockpitBackend` — QML singleton; picks mock or live providers once at
  startup (`QTUI_COCKPIT_MOCK=1` selects the mocks).
- Provider interfaces the pages bind to, each with a deterministic mock and a
  live implementation:
  - `SystemInfoProvider` — `/proc`, `/sys`, mounts, interfaces (Overview).
  - `ServiceProvider` — systemd manager over the system D-Bus; read-only
    fallback when the bus is unreachable (Services).
  - `LogProvider` — journal tail via polled `journalctl --after-cursor`
    (Logs).
  - `NetworkProvider` — interface details; live backend is read-only until a
    NetworkManager/networkd backend lands, the mock is fully configurable
    (Network).
- `Main.qml` — shell: header, tab navigation, page stack, footer key bar,
  toast overlay.
- `OverviewPage.qml` — system identity, CPU/memory gauges, storage, network.
- `NetworkPage.qml` — interface list, IPv4 details, static/DHCP apply flow.
- `ServicesPage.qml` — filterable unit table, start/stop/restart with
  confirmation.
- `LogsPage.qml` — live journal with follow toggle and clear.
- `test_*.js` — one agent scenario per page, run by ctest as
  `device_cockpit_<page>`.

## Test

The application executable carries the scenario runner when Qtui is built
with `QTUI_BUILD_TESTS=ON`, so it is driven directly through the `QTUI_*`
environment variables. Scenario runs default to the mock providers; set
`QTUI_COCKPIT_MOCK=0` to run one against the live system.

```sh
ctest --test-dir build -R device_cockpit_overview --output-on-failure

# Ad hoc: capture one frame, or run any scenario file
QTUI_INSPECT=1 ./build/examples/apps/device_cockpit/qtui_device_cockpit
QTUI_SCRIPT=examples/apps/device_cockpit/test_cockpit.js \
    ./build/examples/apps/device_cockpit/qtui_device_cockpit
```

The scenario asserts the rendered mock values and page switching without any
hard-coded coordinates, and captures the Overview frame as an artifact.

## Deploy on a device

`meta-qtui` packages the cockpit and boots it on the console:

- `qtui-device-cockpit` — the binary, split out of the `qtui-examples` recipe
  so lean images do not pull in the demos.
- `qtui-image-cockpit` — a minimal image whose `qtui-app` launcher starts the
  cockpit on `tty1` at boot:

  ```sh
  kas build --target qtui-image-cockpit meta-qtui/kas/frdm-imx93.yml
  ```

- For a serial console instead of (or in addition to) the framebuffer
  console, the `qtui-autostart` package installs a getty-replacement
  template; on the device:

  ```sh
  systemctl disable serial-getty@ttyLP0.service
  systemctl enable --now qtui-app@ttyLP0.service
  ```

The launched application is `/etc/default/qtui-app` (`QTUI_APP=`), so any
image using `qtui-autostart` can switch to the cockpit by editing one line.
See [docs/frdm_imx93_guide.md](../../../docs/frdm_imx93_guide.md) for the
full flash-and-boot walkthrough.
