# Starter Template — Boot to Qamui Device Setup

A Boot to Qt style device setup application for embedded Linux, rendered
entirely in the terminal. It gets the board on the network so you can deploy
and iterate on your own Qamui applications, and doubles as a starting point
for writing them.

```
┌ Qt  Boot to Qamui · Device Setup · hostname ── [ 📶 SSID ]  12:34 ┐
│ [F1] Network │ [F2] About                                        │
├───────────────────────────────┬──────────────────────────────────┤
│ 📶 Wireless Networks          │ 🔐 Wi-Fi Authentication          │
│   ssid list + signal bars     │ 🌐 Interfaces & assigned IPs     │
│   [R] Rescan  [Enter] Connect │ 📋 System & connection logs      │
├───────────────────────────────┴──────────────────────────────────┤
│ [↑/↓] Select AP  [Enter] Connect  [S/R] Scan  [Q/Esc] Quit      │
└───────────────────────────────────────────────────────────────────┘
```

## Pages

- **Network** (`NetworkPage.qml`) — the boot page. Wi-Fi scanner and
  connection manager backed by `WifiManager` (C++ singleton wrapping
  `iw`/`wpa_cli`), a Remote Access panel showing whether an SSH
  server (`sshd`/`dropbear`) is running together with a ready-to-copy
  `ssh root@<ip>` deploy command (click Copy — uses OSC52, so it lands in
  your local clipboard even over serial), an overview of every interface
  with its assigned IP, and an event log.
- **About** (`AboutPage.qml`) — device identity (hostname, uptime, memory,
  CPU), software versions, and a network summary.

`Theme.qml` is a QML singleton holding the Qt-brand palette (Qt green on dark
navy) shared by all pages.

## Building

Configured as part of the Qamui examples, or standalone:

```sh
cmake -B build -DCMAKE_PREFIX_PATH=<qamui-install-prefix>
cmake --build build
./build/qamui_starter_template
```

Only one production instance should run at a time. The executable holds
`/run/qamui_starter_template.lock` and exits with status 2 when the boot
service already owns it. For development over SSH, use the separately
packaged `qamui_starter_template_benchmark`; it observes network state and
does not scan, associate, or request address changes.

## Network ownership

The starter owns Wi-Fi association through `wpa_supplicant`. It performs all
external commands asynchronously so scans and association cannot block input
or frame commits. `systemd-networkd` is the sole owner of DHCP, addresses, and
routes; after association the starter only asks `networkctl` to reconfigure
the interface.

The implementation also provides deterministic development backends through
`QAMUI_STARTER_BACKEND=mock|observe|live`. Production defaults to `live`; the
scenario runner selects `mock`, while the latency benchmark selects `observe`.

## Testing

`test_setup.js` is a scenario smoke test (see `Qamui::Runner`) covering the
network page, password entry, and page navigation:

```sh
ctest -R '^(starter_template_setup|starter_template_latency)$'
```

The setup scenario runs against deterministic mock networks. The latency
scenario performs 30 page switches and fails if local key-to-committed-frame
p95 exceeds 20 ms. Neither test runner is linked into the production binary.
