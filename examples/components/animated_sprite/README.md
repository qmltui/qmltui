# AnimatedSprite Example

Demonstrates `QmlTui.Image.AnimatedSprite` using the classic 4-frame loading animation from the Qt Quick documentation.

The sprite sheet (`loading.png`, 256×64) contains 4 square frames of 64×64 pixels showing animated progress dots.

```qml
AnimatedSprite {
    source: "loading.png"
    frameWidth: 64
    frameHeight: 64
    frameCount: 4
    frameDuration: 500
    interpolate: false
}
```

## Interactive Controls

| Key | Action |
| --- | --- |
| `Space` | Toggle pause / resume |
| `R` | Toggle forward / reverse playback |
| `+` / `=` | Increase animation speed (reduce frame duration) |
| `-` | Decrease animation speed (increase frame duration) |
| `Q` / `Escape` | Quit application |

## Building and Running

Build with `QMLTUI_ENABLE_IMAGE=ON` and examples enabled:

```sh
cmake --build build --target qmltui_animated_sprite_demo
./build/examples/components/animated_sprite/qmltui_animated_sprite_demo
ctest --test-dir build -R '^animated_sprite_demo$' --output-on-failure
```

The scenario test (`test_animated_sprite_demo.js`) verifies deterministic frame progression under virtual time, pause/resume toggling, reverse playback, and half-block glyph rendering.
