# Calendar

A QmlTui recipe example.

Build with `cmake --build build --target qmltui_calendar_demo`.
Run `./build/examples/recipes/calendar/qmltui_calendar_demo`.
See [main.cpp](main.cpp) for startup and local QML files for the interface.

The example is included when its optional modules are enabled.
Run its scenario with `ctest --test-dir build -R calendar --output-on-failure`.
