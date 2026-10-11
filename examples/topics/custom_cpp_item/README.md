# Custom Cpp Item

A Qamui topic example.

Build with `cmake --build build --target qamui_custom_cpp_item_demo`.
Run `./build/examples/topics/custom_cpp_item/qamui_custom_cpp_item_demo`.
See [main.cpp](main.cpp) for startup and local QML files for the interface.

The example is included when its optional modules are enabled.
Run its scenario with `ctest --test-dir build -R custom_cpp_item --output-on-failure`.
