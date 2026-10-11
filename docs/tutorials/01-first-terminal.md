# 01: First terminal

Create a `Terminal` root and place a `Text` item at a cell coordinate. Escape
is explicitly handled: Terminal does not supply an application quit policy.
`main.cpp` creates the Qt event loop and loads the QML module. No QtQuick import
or display server is required. The standalone quickstart demonstrates a
[FetchContent consumer](../../examples/quickstart/CMakeLists.txt).

Build with `cmake --build build --target qamui_tutorial_01_first_terminal` and run
`./build/examples/tutorials/01_first_terminal/qamui_tutorial_01_first_terminal`. Press Escape to quit.

<!-- include: examples/tutorials/01_first_terminal/Main.qml -->
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
    Text { x: 2; y: 1; text: "Hello, terminal!" }
}
```

<!-- include: examples/tutorials/01_first_terminal/test_tutorial.js -->
```javascript
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Hello, terminal!");
    terminal.captureText("tutorial.txt");
}
```

<!-- include: examples/tutorials/01_first_terminal/main.cpp -->
```cpp
#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("QamuiTutorial01", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;
    return app.exec();
}
```

<!-- include: examples/tutorials/01_first_terminal/CMakeLists.txt -->
```cmake
qt_add_executable(qamui_tutorial_01_first_terminal main.cpp)
qt_add_qml_module(qamui_tutorial_01_first_terminal
    URI QamuiTutorial01 VERSION 1.0 QML_FILES Main.qml
    DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls)
target_link_libraries(qamui_tutorial_01_first_terminal PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)
if(TARGET Qamui::Runner)
    qt_add_executable(qamui_tutorial_01_first_terminal_test main.cpp)
    qt_add_qml_module(qamui_tutorial_01_first_terminal_test
        URI QamuiTutorial01 VERSION 1.0 QML_FILES Main.qml
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/QamuiTutorial01"
        DEPENDENCIES TARGET Qamui::Core TARGET Qamui::Controls)
    target_link_libraries(qamui_tutorial_01_first_terminal_test PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::Runner)
    qamui_add_scenario_test(NAME tutorial_01_smoke TARGET qamui_tutorial_01_first_terminal_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_tutorial.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS "Tutorial;AgentSmoke")
endif()
```

[Tutorial index](index.md)
