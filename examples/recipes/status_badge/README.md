# StatusBadge

A copyable pure-QML status pill component (`StatusBadge.qml`) built from `Rectangle`, `Row`, and `Text`.

- **Presets and variants**: Supports `status` presets (`"success"`, `"warning"`, `"error"`, `"info"`) and `variant` styles (`"subtle"` bracketed `[✓ SUCCESS]`, `"outlined"` 3-row bordered box, `"filled"` inverted background, and `"plain"`), with overrides for `text`, `icon`, `badgeColor`, and `badgeBackgroundColor`.
- **Admission boundary**: Lives in `examples/recipes/` rather than `QmlTui.Controls` because it requires only public QML primitives; copy `StatusBadge.qml` into your application module to customize it.

```bash
cmake --build build --target qmltui_status_badge_demo
./build/examples/recipes/status_badge/qmltui_status_badge_demo
ctest --test-dir build -R '^status_badge_smoke$' --output-on-failure
```
