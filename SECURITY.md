# Security Policy

## Supported Versions

Security fixes are applied to the latest `0.6.x` release series on `main`.

| Version | Supported |
| :--- | :--- |
| `0.6.x` | Yes |
| `< 0.6.0` | No |

## Scope

Security-relevant areas in QmlTui include:

- **Terminal input and escape-sequence decoding (`InputDecoder`)**: Parsing untrusted VT, CSI, OSC, DCS, and APC byte streams without out-of-bounds access or unbounded memory growth.
- **Embedded PTY subshells (`QmlTui.PtyView`)**: Process execution boundaries (`execvp` without shell interpolation; `PtyView::start()` passes `command` + `arguments` directly or splits a whitespace-delimited `command` string via `QProcess::splitCommand()` when `arguments` is empty—applications must pass explicit `arguments` for untrusted input) and escape-sequence handling from child processes via `libvterm`.
- **Terminal image loading (`QmlTui.Image`)**: Network (`http`/`https`), SVG, and raster image decoding (enforcing a 32 MiB response limit, 15 s transfer timeout, and `QNetworkRequest::NoLessSafeRedirectPolicy`) and Kitty/Sixel payload framing.
- **Headless scenario execution (`qmltui::Runner`)**: Local artifact path containment within `--artifacts`.

## Reporting a Vulnerability

Please **do not** open a public GitHub issue for suspected security vulnerabilities.

Instead, report vulnerabilities privately through [GitHub Private Vulnerability Reporting](https://github.com/qmltui/qmltui/security/advisories/new) on `qmltui/qmltui`, including:

1. Affected QmlTui commit or version, OS, Qt 6 version, and terminal emulator.
2. A minimal QML/C++ reproduction or raw byte input sequence.
3. Expected vs. observed impact.

You can expect an initial acknowledgment within 7 days and coordinated disclosure once a fix is available.
