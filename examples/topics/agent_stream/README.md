# Agent Stream

A Qamui topic example.

Build with `cmake --build build --target qamui_agent_stream_demo`.
Run `./build/examples/topics/agent_stream/qamui_agent_stream_demo`.
See [main.cpp](main.cpp) for startup and local QML files for the interface.

The example is included when its optional modules are enabled.
Run its scenario with `ctest --test-dir build -R agent_stream --output-on-failure`.

The banner defaults to a five-row half-block Canvas logo. Set `logoRenderMode`
on `BannerCard` to `Canvas.Braille` for the alternative braille logo. Both use
`Circle` and `Polygon` shapes and the same white-and-green colors.
