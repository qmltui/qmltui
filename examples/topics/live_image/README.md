# Live Image

A Qamui topic example.

Build with `cmake --build build --target qamui_live_image_demo`.
Run `./build/examples/topics/live_image/qamui_live_image_demo`.
See [main.cpp](main.cpp) for startup and local QML files for the interface.

The example is included when its optional modules are enabled.
Run its scenario with `ctest --test-dir build -R live_image --output-on-failure`.
