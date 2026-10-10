#pragma once

#include <Qtui/Image/ImageProvider.h>

#include <QThread>

class RadarRenderer;

// A dynamic picture, asynchronous: the QML side addresses the provider by
// URL, `Image { source: "image://frames/radar?tick=" + n }`, and every new URL
// becomes a request. The sweep is painted on a dedicated render thread;
// retainWhileLoading on the Image keeps the previous sweep visible until the
// new one is ready.
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
