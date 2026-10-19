# Live Image

A QmlTui recipe example.

Build with `cmake --build build --target qmltui_live_image_demo`.
Run `./build/examples/recipes/live_image/qmltui_live_image_demo`.
See [main.cpp](main.cpp) for startup and local QML files for the interface.

The example is included when its optional modules are enabled.
Run its scenario with `ctest --test-dir build -R live_image --output-on-failure`.
