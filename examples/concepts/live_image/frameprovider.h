#pragma once

#include <Qtui/Image/ImageProvider.h>

// Shape 2 of a dynamic picture: a provider the QML side addresses by URL.
// `Image { source: "image://frames/radar?tick=" + n }` asks for a new sweep
// whenever n changes; the id carries the parameters, the provider keeps no
// state, and it renders to the pixel grid the item reports.
class FrameProvider : public qtui::ImageProvider {
public:
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

    // A radar sweep whose angle follows `tick`.
    static QImage paintRadar(int tick, const QSize& size);
};
