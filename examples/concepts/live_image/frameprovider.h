#pragma once

#include <Qtui/Image/ImageProvider.h>

#include <QThread>

class RadarRenderer;

// Shape 2 of a dynamic picture, asynchronous: the QML side addresses the
// provider by URL, `Image { source: "image://frames/radar?tick=" + n }`, and
// every new URL becomes a request. The sweep is painted on a dedicated render
// thread; the Image keeps showing the previous sweep until the new one is
// ready, so a slow renderer never stalls the terminal's frame loop.
class RadarProvider : public qtui::AsyncImageProvider {
    Q_OBJECT
public:
    explicit RadarProvider(QObject* parent = nullptr);
    ~RadarProvider() override;

    qtui::ImageResponse* requestImageResponse(const QString& id, const QSize& requestedSize) override;

    // A radar sweep whose angle follows `tick`. Pure: safe on any thread.
    static QImage paintRadar(int tick, const QSize& size);

private:
    QThread m_thread;
    RadarRenderer* m_renderer = nullptr;   // lives on m_thread
};
