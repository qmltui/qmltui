#pragma once

#include <Qtui/Core/primitives/QtuiExport.h>

#include <QImage>
#include <QObject>
#include <QSize>
#include <QString>

class QQmlEngine;

namespace qtui {

// Supplies pictures to Image items from C++ under an `image://` URL, the same
// shape as Qt Quick's QQuickImageProvider (whose Qt base class is reserved
// for Qt Quick, so Qtui keeps its own registry). Install one on the engine
// and refer to it from QML:
//
//     qtui::ImageProvider::add(&engine, QStringLiteral("frames"), new ScopeProvider);
//     Image { source: "image://frames/scope?frame=" + tick }
//
// `id` is everything after `image://<provider>/`, query included, so a
// changing suffix requests a new picture. The engine owns the provider once
// added. Return a null QImage for an unknown id; the item reports Error.
//
// For a picture the C++ side owns outright, assign Image::setImage() instead;
// the provider suits QML-declared items whose pixels come from C++.
class QTUI_IMAGE_EXPORT ImageProvider : public QObject {
    Q_OBJECT
public:
    explicit ImageProvider(QObject* parent = nullptr);
    ~ImageProvider() override;

    // The picture for `id`. `requestedSize` is the item's current pixel grid
    // when it has a size (columns by rows times the rows per cell of its
    // render mode), otherwise invalid; a provider may ignore it. Set `*size`
    // to the picture's natural size when it differs from what is returned.
    virtual QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) = 0;

    // Registers `provider` as `image://<id>/...` for every Image created by
    // `engine`; the engine takes ownership. A second add() under the same id
    // replaces (and deletes) the first.
    static void add(QQmlEngine* engine, const QString& id, ImageProvider* provider);
    static ImageProvider* find(const QQmlEngine* engine, const QString& id);
    // Removes and deletes the provider registered as `id`, if any.
    static void remove(QQmlEngine* engine, const QString& id);
};

// A picture that arrives later, the shape of Qt Quick's QQuickImageResponse.
// An AsyncImageProvider returns one per request and emits finished() on the
// GUI thread once image() is valid (a null image means failure, see
// errorString()). The requesting Image owns the response and deletes it after
// finished(); if it loses interest first it calls cancel() and then deletes
// it, so a late finished() is harmless. Rendering may happen on any thread;
// only the object itself and its signal must live on the GUI thread.
class QTUI_IMAGE_EXPORT ImageResponse : public QObject {
    Q_OBJECT
public:
    explicit ImageResponse(QObject* parent = nullptr);
    ~ImageResponse() override;

    virtual QImage image() const = 0;
    virtual QString errorString() const { return {}; }
    virtual void cancel() {}

signals:
    void finished();
};

// A provider whose pictures are produced off the GUI thread. The Image keeps
// showing its current picture while a request is pending (status Loading),
// then adopts the response, so a live feed never flickers and a slow renderer
// never stalls a frame. requestImage() is not used for this kind of provider.
class QTUI_IMAGE_EXPORT AsyncImageProvider : public ImageProvider {
    Q_OBJECT
public:
    explicit AsyncImageProvider(QObject* parent = nullptr);
    ~AsyncImageProvider() override;

    // Called on the GUI thread; return a response created on it. Returning
    // null reports Error at once.
    virtual ImageResponse* requestImageResponse(const QString& id, const QSize& requestedSize) = 0;

    QImage requestImage(const QString&, QSize*, const QSize&) final { return QImage(); }
};

} // namespace qtui
