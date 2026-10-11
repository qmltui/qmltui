# ImageProvider

`qamui::ImageProvider` supplies pictures to `Image` items from C++ under an
`image://` URL, in the shape of Qt Quick's `QQuickImageProvider`. Qt reserves
that class's base for Qt Quick, so Qamui keeps its own registry on the QML engine.

- **Header**: `src/Qamui/Image/ImageProvider.h`, installed as `<Qamui/Image/ImageProvider.h>`
- **Inherits**: `QObject`
- **Module**: `Qamui::Image`

## Public Functions

| Function | Description |
| :--- | :--- |
| `virtual QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) = 0` | Returns the picture for `id`, everything after `image://<provider>/` including the query. `requestedSize` is the requesting item's `renderSize`, true pixels under kitty or sixel and the glyph grid otherwise, or invalid while the item has no size; render to it for a crisp result and set `*size` to the natural size if it differs. Return a null image for an unknown id; the item then reports `Error`. |
| `static void add(QQmlEngine* engine, const QString& id, ImageProvider* provider)` | Registers the provider for every Image created by `engine`; the engine takes ownership. Adding again under the same id deletes the previous provider. |
| `static ImageProvider* find(const QQmlEngine* engine, const QString& id)` | The provider registered as `id`, or null. |
| `static void remove(QQmlEngine* engine, const QString& id)` | Removes and deletes the provider registered as `id`. |

## Detailed Description

Register a provider once, then refer to it from QML:

```cpp
class ScopeProvider : public qamui::ImageProvider {
    QImage requestImage(const QString& id, QSize* size, const QSize& requested) override {
        QImage frame(requested.isValid() ? requested : QSize(80, 24), QImage::Format_RGB32);
        frame.fill(Qt::black);
        // ... draw the current trace into `frame` ...
        *size = frame.size();
        return frame;
    }
};

QQmlApplicationEngine engine;
qamui::ImageProvider::add(&engine, QStringLiteral("frames"), new ScopeProvider);
```

```qml
Image { source: "image://frames/scope?frame=" + tick; renderMode: Image.Braille }
```

## Asynchronous providers

Subclass `AsyncImageProvider` instead when the picture is expensive to make.
`requestImageResponse(id, requestedSize)` returns an `ImageResponse` created
on the GUI thread; the picture may be produced anywhere, and the response
emits `finished()` on the GUI thread once `image()` is valid. Enable
`retainWhileLoading` on the Image to keep showing its current picture while
the response is pending (`status` is `Loading`), then adopt the new one without
flicker. By default the old picture is discarded while loading.
A response the item no longer wants receives
`cancel()` and is deleted; a late `finished()` after that is ignored.
Returning null from `requestImageResponse()` reports `Error` at once, as does
a finished response whose `image()` is null (its `errorString()` is logged).
`examples/concepts/live_image` renders its radar sweep this way on a dedicated
`QThread`.

Each change of `source` asks the provider again, so a counter in the query
drives a live picture. `examples/concepts/live_image` is a complete
application doing exactly this. For a picture the C++ side owns outright, assign
`Image::setImage()` (the `image` property) instead; it needs no URL and no
registry.
