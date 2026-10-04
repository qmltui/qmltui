# Starter Template — Boot to Qtui Device Setup

A Boot to Qt style device setup application for embedded Linux, rendered
entirely in the terminal. It is the default boot application of the
`meta-qtui` Yocto images: it gets the board on the network so you can deploy
and iterate on your own Qtui applications, and doubles as a starting point
for writing them.

```
┌ Qt  Boot to Qtui · Device Setup · hostname ── [ 📶 SSID ]  12:34 ┐
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
  `iw`/`wpa_cli`/`udhcpc`), a Remote Access panel showing whether an SSH
  server (`sshd`/`dropbear`) is running together with a ready-to-copy
  `ssh root@<ip>` deploy command (click Copy — uses OSC52, so it lands in
  your local clipboard even over serial), an overview of every interface
  with its assigned IP, and an event log.
- **About** (`AboutPage.qml`) — device identity (hostname, uptime, memory,
  CPU), software versions, and a network summary.

`Theme.qml` is a QML singleton holding the Qt-brand palette (Qt green on dark
navy) shared by all pages.

## Building

Configured as part of the Qtui examples, or standalone:

```sh
cmake -B build -DCMAKE_PREFIX_PATH=<qtui-install-prefix>
cmake --build build
./build/qtui_starter_template
```

## Testing

`test_setup.js` is a scenario smoke test (see `Qtui::Testing`) covering the
network page, password entry, and page navigation:

```sh
ctest -R starter_template_setup
```
