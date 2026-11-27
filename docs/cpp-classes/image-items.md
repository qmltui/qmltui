# Image, AnimatedSprite, and Core Item Headers

In addition to [`Item`](item.md), [`Rectangle`](rectangle.md), [`Text`](text.md), and [`MouseArea`](mousearea.md), QmlTui installs public C++ headers for the raster image module (`QmlTui::Image`) and all built-in Core visual items (`QmlTui::Core`).

## `qmltui::Image` and `qmltui::AnimatedSprite`

- **Headers**:
  - `<QmlTui/Image/Image.h>`
  - `<QmlTui/Image/AnimatedSprite.h>`
  - `<QmlTui/Image/ImageProvider.h>` (documented in [ImageProvider](imageprovider.md))
- **CMake Option**: `-DQMLTUI_ENABLE_IMAGE=ON`
- **Target**: `QmlTui::Image`

### `qmltui::Image`

Inherits `qmltui::Item`. Renders a `QUrl` (`source()`, supporting local files, `qrc:`, `http(s):`, and `image://` providers) or an in-memory `QImage` (`image()` / `setImage(const QImage&)`).

- **Enums**:
  - `Image::FillMode`: `Stretch`, `PreserveAspectFit`, `PreserveAspectCrop`, `Tile`, `Pad`
  - `Image::RenderMode`: `HalfBlock`, `Braille`, `ASCII`, `Kitty`, `Sixel`, `Auto`
  - `Image::Status`: `Null`, `Ready`, `Loading`, `Error`
- **Key C++ Methods**:
  - `QImage image() const` / `void setImage(const QImage& image)`: Pushes rendered `QImage` frames directly from C++ without touching disk or a URL provider.
  - `QRect sourceClipRect() const` / `void setSourceClipRect(const QRect&)` / `void resetSourceClipRect()`: Crops a pixel sub-rectangle before scaling.
  - `QSize renderSize() const`: Returns the active target pixel or subpixel resolution (`columns * cellPixelWidth` for Kitty/Sixel, `1x2` per cell for `HalfBlock`, `2x4` for `Braille`, `1x1` for `ASCII`) so C++ frame producers can render at exact target resolution.

See the [Image QML type reference](../qml-types/image.md) for transparency rules and protocol fallback behavior.

### `qmltui::AnimatedSprite`

Inherits `qmltui::Image` and `QQmlParserStatus`. Steps `sourceClipRect` across a sprite sheet on the QmlTui clock (deterministic under `Runner` virtual time).

- **Enums**: `AnimatedSprite::Loops` (`Infinite = -1`), `AnimatedSprite::FinishBehavior` (`FinishAtInitialFrame`, `FinishAtFinalFrame`).
- **Playback Methods**: `start()`, `stop()`, `restart()`, `pause()`, `resume()`, `advance(int frames = 1)`, and signal `finished()`.

See the [AnimatedSprite QML type reference](../qml-types/animatedsprite.md).

## Installed `QmlTui/Core/Items` Headers

Every Core visual item header below is installed under `<QmlTui/Core/Items/...>` and exported by `QmlTui::Core`:

| Header | Class | QML Type Reference |
| :--- | :--- | :--- |
| `<QmlTui/Core/Items/Anchors.h>`, `<QmlTui/Core/Items/AnchorLine.h>` | `qmltui::Anchors`, `qmltui::AnchorLine` | [Anchors](../qml-types/anchors.md), [AnchorLine](../qml-types/anchorline.md) |
| `<QmlTui/Core/Items/Border.h>` | `qmltui::Border` | [Border](../qml-types/border.md) |
| `<QmlTui/Core/Items/Gradient.h>`, `<QmlTui/Core/Items/GradientStop.h>` | `qmltui::Gradient`, `qmltui::GradientStop` | [Gradient](../qml-types/gradient.md), [GradientStop](../qml-types/gradientstop.md) |
| `<QmlTui/Core/Items/Row.h>`, `<QmlTui/Core/Items/Column.h>`, `<QmlTui/Core/Items/LinearLayout.h>`, `<QmlTui/Core/Items/GridLayout.h>`, `<QmlTui/Core/Items/StackLayout.h>`, `<QmlTui/Core/Items/Layout.h>` | `Row`, `Column`, `RowLayout`, `ColumnLayout`, `GridLayout`, `StackLayout`, `Layout` | [Row](../qml-types/row.md), [Column](../qml-types/column.md), [GridLayout](../qml-types/gridlayout.md), [StackLayout](../qml-types/stacklayout.md) |
| `<QmlTui/Core/Items/FocusScope.h>`, `<QmlTui/Core/Items/Popup.h>` | `qmltui::FocusScope`, `qmltui::Popup` | [FocusScope](../qml-types/focusscope.md), [Popup](../qml-types/popup.md) |
| `<QmlTui/Core/Items/TextInput.h>`, `<QmlTui/Core/Items/TextArea.h>` | `qmltui::TextInput`, `qmltui::TextArea` | [TextInput](../qml-types/textinput.md), [TextArea](../qml-types/textarea.md) |
| `<QmlTui/Core/Items/ScrollView.h>`, `<QmlTui/Core/Items/ListView.h>`, `<QmlTui/Core/Items/GridView.h>`, `<QmlTui/Core/Items/TableView.h>`, `<QmlTui/Core/Items/TreeView.h>` | `ScrollView`, `ListView`, `GridView`, `TableView`, `TreeView` | [ListView](../qml-types/listview.md), [TableView](../qml-types/tableview.md), [TreeView](../qml-types/treeview.md) |
| `<QmlTui/Core/Items/Canvas.h>` | `qmltui::Canvas`, `CanvasShape`, `CanvasRectangle`, `Circle`, `Line`, `Point`, `Polygon` | [Canvas](../qml-types/canvas.md) |
| `<QmlTui/Core/Items/Loader.h>`, `<QmlTui/Core/Items/Repeater.h>` | `qmltui::Loader`, `qmltui::Repeater` | [Loader](../qml-types/loader.md), [Repeater](../qml-types/repeater.md) |
| `<QmlTui/Core/Items/Scrollback.h>` | `qmltui::Scrollback` | [Scrollback](../qml-types/scrollback.md) |

## Additional Installed Headers (`Core/Engine`, `Core/Primitives`, `Controls`)

The following QML-backing and utility headers are also installed as part of the public header set (verified at configure time by `cmake/QmlTuiPublicHeaders.cmake`):

| Header | Target | Purpose |
| :--- | :--- | :--- |
| `<QmlTui/Core/Engine/Animation.h>`, `<QmlTui/Core/Engine/State.h>`, `<QmlTui/Core/Engine/StateGroup.h>`, `<QmlTui/Core/Engine/Transition.h>` | `QmlTui::Core` | QML-backing C++ types for declarative animations and state machines. |
| `<QmlTui/Core/Engine/Clipboard.h>`, `<QmlTui/Core/Engine/Keys.h>` | `QmlTui::Core` | QML-backing C++ types for the `Clipboard` singleton and `Keys` attached object. |
| `<QmlTui/Core/Primitives/QmlTuiGlobal.h>`, `<QmlTui/Core/Primitives/QmlTuiExport.h>` | `QmlTui::Core` | `QmlTui` QML singleton helpers (`span`, `color`, `rgba`, `ansi`) and shared/static symbol export macros. |
| `<QmlTui/Controls/Sparkline.h>`, `<QmlTui/Controls/TabBar.h>`, `<QmlTui/Controls/ToolTip.h>` | `QmlTui::Controls` | C++-backed controls (`qmltui::Sparkline`, `qmltui::TabBar`, `qmltui::ToolTip`). |
