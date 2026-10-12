# 01: First terminal

Create a `Terminal` root and place a `Text` item at a cell coordinate. Escape
is explicitly handled: Terminal does not supply an application quit policy.
`main.cpp` creates the Qt event loop and loads the QML module. No QtQuick import
or display server is required. The standalone quickstart demonstrates a
[FetchContent consumer](../../examples/quickstart/CMakeLists.txt).

Build with `cmake --build build --target qmltui_tutorial_01_first_terminal` and run
`./build/examples/tutorials/01_first_terminal/qmltui_tutorial_01_first_terminal`. Press Escape to quit.

<!-- include: examples/tutorials/01_first_terminal/Main.qml -->
```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

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
    engine.loadFromModule("QmlTuiTutorial01", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;
    return app.exec();
}
```

<!-- include: examples/tutorials/01_first_terminal/CMakeLists.txt -->
```cmake
qt_add_executable(qmltui_tutorial_01_first_terminal main.cpp)
qt_add_qml_module(qmltui_tutorial_01_first_terminal
    URI QmlTuiTutorial01 VERSION 1.0 QML_FILES Main.qml
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls)
target_link_libraries(qmltui_tutorial_01_first_terminal PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)
if(TARGET qmltui::runner)
    target_link_libraries(qmltui_tutorial_01_first_terminal PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME tutorial_01_smoke TARGET qmltui_tutorial_01_first_terminal
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_tutorial.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS "Tutorial;AgentSmoke")
endif()
```

[Tutorial index](index.md)
