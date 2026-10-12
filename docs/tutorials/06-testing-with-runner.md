# 06: Testing with runner

The `_test` executable shares the application's main.cpp and QML, and links
Qamui::Runner. `qamui_add_scenario_test` sets the viewport and runs a JavaScript
scenario. `expectSnapshot()` checks the complete committed frame; create or
refresh its baseline with `QAMUI_RUNNER_UPDATE_SNAPSHOTS=1 ctest --test-dir build
-R tutorial_06_smoke`. Inspect every refreshed baseline before keeping it.

The second scenario records a paced cast. With `CLOCK VIRTUAL`, `sleep()` and
`advanceTime()` advance Qamui time instantly; markers identify demonstration
steps. Recordings are listed in `result.json` under `artifacts.casts`.

Build with `cmake --build build --target qamui_tutorial_06_testing_with_runner` and run
`./build/examples/tutorials/06_testing_with_runner/qamui_tutorial_06_testing_with_runner`. Press Escape to quit.

<!-- include: examples/tutorials/06_testing_with_runner/Main.qml -->
```qml
pragma ComponentBehavior: Bound
import QtQml
import Qamui
import Qamui.Controls

Terminal {
    id: root
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }
    property string status: "Settings ready"
    component AppButton: Button { textColor: "cyan" }
    Column {
        x: 2; y: 1; spacing: 1
        Text { text: root.status }
        CheckBox { id: logging; text: "Logging"; focus: true }
        Text { text: logging.checked ? "Logging enabled" : "Logging disabled" }
        Switch { text: "Notifications" }
        Slider { id: volume; width: 30; from: 0; to: 100; value: 40 }
        ProgressBar { width: 30; from: 0; to: 100; value: volume.value }
        Text { text: "Volume: " + volume.value + "%" }
        AppButton { text: "Apply settings"; onClicked: confirmation.open() }
    }
    Dialog {
        id: confirmation
        title: "Apply settings?"
        message: "Save these settings now?"
        onAccepted: root.status = "Settings saved"
        onRejected: root.status = "Settings unchanged"
    }
}
```

<!-- include: examples/tutorials/06_testing_with_runner/test_tutorial.js -->
```javascript
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectSnapshot("snapshots/initial.txt");
    terminal.press("Space");
    terminal.expectText("Logging enabled");
    terminal.captureText("tutorial.txt");
}
```

<!-- include: examples/tutorials/06_testing_with_runner/main.cpp -->
```cpp
#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("QamuiTutorial06", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;
    return app.exec();
}
```

<!-- include: examples/tutorials/06_testing_with_runner/CMakeLists.txt -->
```cmake
qt_add_executable(qamui_tutorial_06_testing_with_runner main.cpp)
qt_add_qml_module(qamui_tutorial_06_testing_with_runner
    URI QamuiTutorial06 VERSION 1.0 QML_FILES Main.qml
    DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls)
target_link_libraries(qamui_tutorial_06_testing_with_runner PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)
if(TARGET Qamui::Runner)
    target_link_libraries(qamui_tutorial_06_testing_with_runner PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME tutorial_06_smoke TARGET qamui_tutorial_06_testing_with_runner
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_tutorial.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS "Tutorial;AgentSmoke")
    qamui_add_scenario(NAME tutorial_06_cast TARGET qamui_tutorial_06_testing_with_runner
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_record.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL RECORD tutorial.cast LABELS Tutorial)
endif()
```

[Tutorial index](index.md)

## Launch a scenario from the shell

When your normal application links `Qamui::Runner`, use the installed launcher
(or `build/tools/qamui-runner/qamui-runner` from a development build):

```bash
qamui-runner --script test.js --artifacts ./run-results -- ./myApp
qamui-runner --capture-after-ms 0 -- ./myApp
qamui-runner --script tour.js --clock virtual --record tour.cast -- ./myApp
```

The explicit artifact directory must be new or empty. Without `--artifacts`,
the launcher retains a unique temporary directory and prints its location.
Inspect `result.json` and the committed `final.txt`, `failure.txt`, or capture
files there. Exit codes are 0 (pass), 1 (scenario failure), and 2 (launcher or
process failure).

Build/install with `QAMUI_ENABLE_RUNNER=ON` to use this without repository tests.
The application needs a current Runner supporting the activation handshake;
C++-created terminals also need `Runner::startFromEnvironment()`. The launcher
starts a new process and does not inject Runner into an unprepared executable.
Existing `qamui_add_scenario()` tests continue using their original driver.
See the [launcher reference](../cpp-classes/runner.md#command-line-launcher) for
all options, deadlines, environment handling, and remote execution through SSH.
