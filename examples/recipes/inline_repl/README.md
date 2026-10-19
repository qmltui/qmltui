# Inline Repl

A QmlTui recipe example.

Build with `cmake --build build --target qmltui_inline_repl_demo`.
Run `./build/examples/recipes/inline_repl/qmltui_inline_repl_demo`.
See [main.cpp](main.cpp) for startup and local QML files for the interface.

The example is included when its optional modules are enabled.
Run its scenario with `ctest --test-dir build -R inline_repl --output-on-failure`.
