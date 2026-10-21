# Live Image

Streams procedural `QImage` frames from a background `QThread` into a QML `Image` item using `qmltui::AsyncImageProvider`.

- **`RadarProvider`** (`frameprovider.h`, `frameprovider.cpp`) is registered with `qmltui::ImageProvider::add(&engine, "frames", new RadarProvider)` and renders radar sweeps at the `requestedSize` (`Image.renderSize`) on a worker thread.
- **Flicker-free updates**: `Image` sets `source: "image://frames/radar?tick=" + root.tick` and `retainWhileLoading: true` so the previous frame remains visible while the next `ImageResponse` is rendering.
- **Build requirement**: Included when `QMLTUI_ENABLE_IMAGE=ON`.

```bash
cmake --build build --target qmltui_live_image_demo
./build/examples/recipes/live_image/qmltui_live_image_demo
ctest --test-dir build -R '^live_image_smoke$' --output-on-failure
```
