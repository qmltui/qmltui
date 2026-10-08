# ImageProvider

`qtui::ImageProvider` supplies pictures to `Image` items from C++ under an
`image://` URL, in the shape of Qt Quick's `QQuickImageProvider`. Qt reserves
that class's base for Qt Quick, so Qtui keeps its own registry on the QML engine.

- **Header**: `src/Qtui/Image/ImageProvider.h`, installed as `<Qtui/Image/ImageProvider.h>`
- **Inherits**: `QObject`
- **Module**: `Qtui::Image`

## Public Functions

| Function | Description |
| :--- | :--- |
| `virtual QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) = 0` | Returns the picture for `id`, everything after `image://<provider>/` including the query. `requestedSize` is the requesting item's pixel grid when it has a size, else invalid; set `*size` to the natural size if it differs. Return a null image for an unknown id; the item then reports `Error`. |
| `static void add(QQmlEngine* engine, const QString& id, ImageProvider* provider)` | Registers the provider for every Image created by `engine`; the engine takes ownership. Adding again under the same id deletes the previous provider. |
| `static ImageProvider* find(const QQmlEngine* engine, const QString& id)` | The provider registered as `id`, or null. |
| `static void remove(QQmlEngine* engine, const QString& id)` | Removes and deletes the provider registered as `id`. |

## Detailed Description

Register a provider once, then refer to it from QML:

```cpp
class ScopeProvider : public qtui::ImageProvider {
    QImage requestImage(const QString& id, QSize* size, const QSize& requested) override {
        QImage frame(requested.isValid() ? requested : QSize(80, 24), QImage::Format_RGB32);
        frame.fill(Qt::black);
        // ... draw the current trace into `frame` ...
        *size = frame.size();
        return frame;
    }
};

QQmlApplicationEngine engine;
qtui::ImageProvider::add(&engine, QStringLiteral("frames"), new ScopeProvider);
```

```qml
Image { source: "image://frames/scope?frame=" + tick; renderMode: Image.Braille }
```

Each change of `source` asks the provider again, so a counter in the query
drives a live picture. `examples/concepts/live_image` is a complete
application doing exactly this next to an `Image.image` binding. For a picture the C++ side owns outright, assign
`Image::setImage()` (the `image` property) instead; it needs no URL and no
registry.
