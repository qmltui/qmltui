# Device Cockpit — design and milestone plan

A skinnable maintenance and commissioning console for headless embedded Linux
devices, rendered over the serial console or SSH. Think "BIOS setup screen for
your product": network configuration, service health, live logs, firmware
update, and bus diagnostics — declarative QML on top of Qtui, with real Qt
backends (QtDBus, QtSerialBus) instead of shell scripts and `dialog(1)`.

It starts life as an in-tree Qtui app (this directory) and is structured so it
can graduate to its own repository once the page API settles.

## Why this app

- Every embedded product team hand-rolls this UI today, badly.
- No existing TUI framework has native D-Bus, CAN, and Modbus access; Qt does.
- It exercises and showcases Qtui exactly where it beats ratatui/Textual:
  model/view over live system data, retained-mode updates over a serial line,
  and agent-testable scenarios in CI without hardware.
- It reuses proven patterns from `din_gateway` and `system_dashboard`.

## Architecture

Three layers, strictly separated so pages are testable without a device:

```
┌────────────────────────────────────────────────────────┐
│ Shell (QML)                                            │
│  header · page navigation · footer keybar · toasts     │
│  theme singleton · page registry                       │
├────────────────────────────────────────────────────────┤
│ Pages (QML, one directory per page)                    │
│  Overview · Network · Services · Logs · Update · Bus   │
├────────────────────────────────────────────────────────┤
│ Providers (C++ QObject/QAbstractItemModel per domain)  │
│  interface class + real impl + mock impl               │
│  SystemInfo · Network · Services · Journal · Update    │
└────────────────────────────────────────────────────────┘
```

### Shell

- `Terminal` root in frame renderer mode; `TabHeader` (Qtui.Extras) or a
  custom nav rail for page switching, `StackLayout` hosting pages.
- Header: product name, hostname, clock, connection badge. Footer: active
  keybindings for the focused page (each page exposes a `keyHints` list).
- `ToastManager` for async results ("service restarted", "IP applied").
- Branding via a theme singleton (colors, product name), following
  `din_gateway/GatewayTheme.qml`; later sourced from a config file.

### Pages

Each page is one QML file plus optional sub-components, receiving its provider
via a required property. Pages never touch the system directly — no `/proc`
reads, no D-Bus calls from QML. That keeps every page runnable against mocks.

Page registry starts as a static list in `Main.qml` (name, icon char, source
component, provider). A dynamic plugin mechanism (config-driven `Loader`) is
deferred until at least two products consume the cockpit.

### Providers

One C++ interface per domain with two implementations: the real backend and a
mock that ships in-tree and drives CI and demos. Selection via
`QTUI_COCKPIT_MOCK=1` (same pattern the din_gateway backend simulates, made
explicit). Big or fast-changing collections are `QAbstractListModel`s with
role-scoped `dataChanged`, not `QVariantList` snapshots — that is what makes
Qtui's views cheap over a 115200-baud serial line.

| Provider | Real backend | Notes |
| --- | --- | --- |
| SystemInfoProvider | `/proc`, `/sys/class/hwmon`, `statvfs` | CPU, memory, temps, disks, uptime |
| NetworkProvider | NetworkManager D-Bus (`org.freedesktop.NetworkManager`) | interface list/state, DHCP vs static IPv4 apply; read-only degraded mode when NM is absent; connman backend later behind the same interface |
| ServicesProvider | systemd D-Bus (`org.freedesktop.systemd1`) | list units, start/stop/restart/enable, unit properties |
| JournalProvider | `journalctl -f -o json` via `QProcess` (M1); `sd-journal` via libsystemd later | live follow, unit + priority filter |
| UpdateProvider (M2) | RAUC D-Bus (`de.pengutronix.rauc`) | slot status, install bundle with progress |
| BusProvider (M3) | QtSerialBus: socketcan + Modbus RTU/TCP master | live frames, DBC decode is a stretch goal |

### Privilege model

M1 assumes the cockpit runs as root on the device console (it replaces a getty)
and documents that. Polkit-mediated operation for unprivileged SSH sessions is
M2+; the D-Bus backends already go through the system bus, so the seam exists.

### Deployment

- systemd unit running the cockpit on `ttyS0`/`tty1` as the getty alternative
  (`TTYPath=`, `StandardInput=tty`, `Restart=always`).
- Desktop quickstart: `QTUI_COCKPIT_MOCK=1 ./device_cockpit` in any terminal.

### Testing

Every page ships a `qtui_add_scenario_test` scenario running against the mock
providers: navigate to the page, assert rendered values from known mock data,
drive one interaction (e.g. restart a service, apply a static IP), assert the
toast and resulting state. Real-backend smoke tests are manual/on-target only.

## Milestone 1 — MVP

Goal: a usable console with four pages, running over serial and in mock mode
on a desktop, with CI scenarios.

Status: implemented. Logs polls one-shot `journalctl --after-cursor` instead
of `-f`, whose pipe output is unreliable on some hosts; the live Network
backend is read-only until the NetworkManager backend lands. On-target
validation on embedded targets is pending a device run.

1. **Shell**: navigation, header, footer keybar, theme, toasts. Keyboard-first
   (tab/arrow navigation, single-key page switching); mouse works where Qtui
   gives it for free.
2. **Overview page**: `Gauge`/`Sparkline` for CPU, memory, temperature;
   uptime, disk usage bars, per-interface network badges.
3. **Network page**: interface `ListView` + detail `Panel` (IP, gateway, DNS,
   MAC, link state); DHCP↔static toggle with apply/confirm `Dialog` via
   NetworkManager; clean read-only fallback without NM.
4. **Services page**: systemd unit `TableView` (name, load, active, sub,
   description), filter `TextInput`, start/stop/restart with confirm dialog,
   `StatusBadge` for states.
5. **Logs page**: journald live follow on `StreamView`, priority
   colorization, unit filter, pause/resume, "jump to service logs" action from
   the Services page.
6. **Mocks + CI**: mock impl for all four providers; one scenario per page.
7. **Deployment**: systemd getty-replacement unit, README with desktop
   instructions.

Explicitly out of M1: firmware update, CAN/Modbus, Wi-Fi configuration,
polkit, config-file page registry, dynamic theming.

## Milestone 2 — productization

- Update page (RAUC): slot overview, bundle install with `ProgressBar`,
  reboot prompt.
- Config file (`/etc/device-cockpit.conf`): enabled pages, branding, journal
  defaults.
- Polkit support for unprivileged sessions; hide actions the caller can't do.
- `sd-journal` backend replacing the `journalctl` subprocess.

## Milestone 3 — differentiators

- Bus diagnostics page: socketcan live frame table with filters and tx, Modbus
  register poller; DBC signal decode as stretch.
- Embedded D-Bus explorer page (`TreeView` over introspection, method invoke,
  signal monitor) — designed to also build standalone as its own tool.

## Risks

- **NetworkManager vs connman on embedded targets**: mitigated by the provider
  interface; M1 ships NM plus read-only fallback, connman is a follow-up impl.
- **journald access/permissions**: M1 subprocess approach inherits journalctl's
  own permission handling; root assumption covers the console case.
- **Serial-line bandwidth**: retained-mode diffing should keep updates small;
  validate early on real 115200-baud serial, throttle sparkline/log refresh if
  needed.
