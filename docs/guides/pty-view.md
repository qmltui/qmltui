# Embedded PTY Shells (`QmlTui.PtyView`)

`QmlTui.PtyView` provides the [`PtyView`](../qml-types/ptyview.md) item, an embedded terminal emulator backed by a Unix pseudo-terminal (PTY) and `libvterm`. Use it when your QmlTui application needs to host an interactive login shell, SSH session, build task, or external CLI tool inside a layout pane.

> [!NOTE]
> If your application only needs to stream its own output into the parent terminal's scrollback history while keeping a live prompt at the bottom, use [`Terminal.Inline` and `Scrollback`](inline-mode.md) instead—that mode works on all platforms and does not require a PTY.

---

## 1. Enabling `QmlTui.PtyView` in CMake

`QmlTui.PtyView` is supported on **Linux and macOS** and is opt-in via `-DQMLTUI_ENABLE_PTY_VIEW=ON`. By default, CMake fetches a pinned copy of `libvterm`; pass `-DQMLTUI_USE_SYSTEM_LIBVTERM=ON` for offline or distro-packaged builds:

```bash
cmake -S . -B build \
  -DQMLTUI_ENABLE_PTY_VIEW=ON \
  -DQMLTUI_USE_SYSTEM_LIBVTERM=OFF
```

Link `qmltui::pty_view` in your application's `CMakeLists.txt` and import `QmlTui.PtyView` in QML:

```cmake
qt_add_qml_module(my_app
    URI MyApp
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::pty_view
)
target_link_libraries(my_app PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::pty_view)
```

---

## 2. Spawning Processes Without Shell Expansion

`PtyView` executes `program` and `arguments` directly via `execvp` without invoking an intermediate shell or expanding shell metacharacters:

- **`program` & `arguments`**: Executable path/name and string list of arguments. Call `pty.defaultUserShell()` to resolve the current user's login shell.
- **`workingDirectory`**: Initial working directory for the child process.
- **`environment` & `inheritEnvironment`**: Map of extra environment variables (`{ "MY_VAR": "1" }`), merged with the parent environment when `inheritEnvironment: true` (default).
- **`terminalType`**: Sets `TERM` inside the PTY (defaults to `"xterm-256color"`, alongside `COLORTERM=truecolor` and `TERM_PROGRAM=qmltui`).
- **`autoStart`**: Automatically starts the child process when the QML component completes, or call `start()`, `terminate()` (`SIGTERM`), and `kill()` (`SIGKILL`) explicitly.

---

## 3. Complete QML Example: Key Interception, `sendText()`, and Scrollback

Because an active `PtyView` forwards ordinary keystrokes (`Q`, `Escape`, arrows, `Ctrl+C`) to the child PTY, intercept host application shortcuts before the PTY consumes them using `Keys.priority: Keys.BeforeItem`:

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.PtyView

Terminal {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 1

        Text {
            text: "PID: " + pty.processId + " | State: " + (pty.running ? "Running" : "Stopped (" + pty.exitCode + ")")
                  + " | Title: " + (pty.title || "shell")
            color: "cyan"
            bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            border {
                style: pty.activeFocus ? Border.Double : Border.Single
                color: pty.activeFocus ? "cyan" : "gray"
            }

            PtyView {
                id: pty
                anchors {
                    fill: parent
                    margins: 1
                }
                focus: true
                program: defaultUserShell()
                arguments: ["-l"]
                autoStart: true

                Keys.priority: Keys.BeforeItem
                Keys.onPressed: event => {
                    if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_Q) {
                        pty.terminate();
                        Qt.quit();
                        event.accepted = true;
                    } else if (event.key === Qt.Key_F1) {
                        pty.sendText("echo 'Hello from QmlTui'\n");
                        event.accepted = true;
                    } else if (event.key === Qt.Key_PageUp) {
                        pty.scrollOffset = Math.min(pty.scrollbackSize, pty.scrollOffset + 5);
                        event.accepted = true;
                    } else if (event.key === Qt.Key_Escape && pty.scrollOffset > 0) {
                        pty.scrollToBottom();
                        event.accepted = true;
                    }
                }

                onFinished: exitCode => Qt.quit()
            }
        }
    }
}
```

---

## 4. Security Defaults & OSC Permissions

Untrusted terminal output can emit Operating System Command (OSC) sequences. `PtyView` restricts side-effecting sequences by default:

| Property | Default | Behavior |
| :--- | :--- | :--- |
| `allowTitleChanges` | `true` | Updates the read-only `title` property when the child emits OSC 0 / OSC 2 window title sequences. |
| `allowClipboardWrite` | `false` | When enabled, OSC 52 clipboard requests emit `clipboardWriteRequested(string text)` instead of writing silently. |
| `allowNotifications` | `false` | When enabled, OSC 9 / OSC 777 desktop notification requests emit `notificationRequested(string title, string message)`. |

See the [PtyView QML Reference](../qml-types/ptyview.md), the [PtyView C++ Reference](../cpp-classes/ptyview.md), and the interactive Gallery example (`./build/examples/gallery/qmltui_gallery --example PtyView`).
