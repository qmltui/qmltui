# Hello QmlTui Starter Example

Minimal 3-file `QmlTui::Core`-only application (`CMakeLists.txt`, `main.cpp`, `Main.qml`) plus a headless `Runner` scenario (`test_helloworld.js`). You can copy this directory out of the repository and use it directly as a standalone project, or start a full project repository with `QmlTui.Controls`, a C++ `QML_ELEMENT` controller, `CMakePresets.json`, editor configs, `AGENTS.md`, and CI from **[`qmltui/template`](https://github.com/qmltui/template)** ([Use this template](https://github.com/qmltui/template/generate)).

## Build and Run Standalone (`FetchContent`)

Copy `examples/helloworld/` into a new directory, then configure, build, and run:

```sh
# System Qt 6.8+ on PATH:
cmake -S . -B build -G Ninja

# Or point CMake at a specific Qt 6.8+ kit:
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.x/gcc_64

cmake --build build
./build/hello_qmltui
```

Press **Enter** to increment the counter, **R** to reset, or **Q** / **Escape** to quit.

## Run the Headless `Runner` Scenario

Run the smoke test with `-LE Demo` (which excludes long-running `.cast` / `.gif` recording scenarios labeled `Demo`):

```sh
ctest --test-dir build -LE Demo --output-on-failure
```

## Use an Installed `QmlTui` Package (`find_package`)

If you installed QmlTui with `cmake --install build --prefix /your/prefix`, replace the `FetchContent_Declare` / `FetchContent_MakeAvailable` block in `CMakeLists.txt` with `find_package(QmlTui 0.6 REQUIRED COMPONENTS Core OPTIONAL_COMPONENTS Runner)` (`CMakeLists.txt` already detects `IMPORTED` targets and automatically omits `DEPENDENCIES TARGET QmlTui::Core` when using `find_package`; see [Build Configuration and Installation](../../docs/build-configuration.md)).

## Build for WebAssembly (`qt-cmake`)

`CMakeLists.txt` calls `qmltui_wasm_app(hello_qmltui)` when built with a Qt for WebAssembly `wasm_singlethread` kit, generating `hello_qmltui.html` alongside `hello_qmltui.js` and `hello_qmltui.wasm`:

```sh
source ~/emsdk/emsdk_env.sh
/path/to/Qt/6.11.x/wasm_singlethread/bin/qt-cmake -S . -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build-wasm
python3 -m http.server -d build-wasm 8000
```

Open `http://localhost:8000/hello_qmltui.html` in a browser (see the [WebAssembly Guide](../../docs/guides/webassembly.md)).
