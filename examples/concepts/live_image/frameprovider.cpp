#include "frameprovider.h"

#include <QPainter>
#include <QUrlQuery>
#include <QtMath>

// The response the Image holds while the render thread works. It lives on
// the GUI thread; the renderer hands the picture over with a queued call.
class RadarResponse : public qtui::ImageResponse {
    Q_OBJECT
public:
    QImage image() const override { return m_image; }
    QString errorString() const override { return m_error; }
    void cancel() override { m_cancelled = true; }

    void complete(const QImage& image) {
        if (m_cancelled) return;      // the Image moved on; it will delete us
        m_image = image;
        if (image.isNull()) m_error = QStringLiteral("radar: nothing to draw");
        emit finished();
    }

private:
    QImage m_image;
    QString m_error;
    bool m_cancelled = false;
};

// The worker on the render thread. One object, queued calls in order, so
// frames finish in the order they were requested.
class RadarRenderer : public QObject {
    Q_OBJECT
public:
    void render(int tick, const QSize& size, RadarResponse* response) {
        const QImage image = RadarProvider::paintRadar(tick, size);
        // Back to the GUI thread: the response and its signal live there.
        QMetaObject::invokeMethod(response, [response, image] { response->complete(image); },
                                  Qt::QueuedConnection);
    }
};

RadarProvider::RadarProvider(QObject* parent) : qtui::AsyncImageProvider(parent) {
    m_renderer = new RadarRenderer;
    m_renderer->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_renderer, &QObject::deleteLater);
    m_thread.setObjectName(QStringLiteral("radar-render"));
    m_thread.start();
}

RadarProvider::~RadarProvider() {
    m_thread.quit();
    m_thread.wait();
}

qtui::ImageResponse* RadarProvider::requestImageResponse(const QString& id, const QSize& requestedSize) {
    // id is "<name>?<query>": the name selects the picture, the query its parameters.
    const int query = id.indexOf(QLatin1Char('?'));
    const QString name = query < 0 ? id : id.left(query);
    const QUrlQuery parameters(query < 0 ? QString() : id.mid(query + 1));
    if (name != QLatin1String("radar"))
        return nullptr;               // unknown picture: the Image reports Error
    const int tick = parameters.queryItemValue(QStringLiteral("tick")).toInt();
    // requestedSize is the Image's renderSize: true pixels under kitty or
    // sixel, the glyph grid otherwise, so the sweep is crisp either way.
    const bool usable = requestedSize.isValid() && !requestedSize.isEmpty()
        && requestedSize.width() <= 4096 && requestedSize.height() <= 4096;
    const QSize target = usable ? requestedSize : QSize(256, 128);

    auto* response = new RadarResponse;   // owned by the requesting Image
    QMetaObject::invokeMethod(m_renderer, [renderer = m_renderer, tick, target, response] {
        renderer->render(tick, target, response);
    }, Qt::QueuedConnection);
    return response;
}

QImage RadarProvider::paintRadar(int tick, const QSize& size) {
    QImage image(size, QImage::Format_ARGB32);
    image.fill(QColor(11, 15, 20));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QPointF centre(size.width() / 2.0, size.height() / 2.0);
    const double radius = std::min(size.width(), size.height()) / 2.0 - 1;
    painter.setPen(QPen(QColor(30, 41, 59), 1));
    for (int ring = 1; ring <= 3; ++ring)
        painter.drawEllipse(centre, radius * ring / 3.0, radius * ring / 3.0);
    // The sweep: a wedge of fading green, rotating 12 degrees per tick.
    const double angle = tick * 12.0;
    for (int trail = 0; trail < 6; ++trail) {
        QColor colour(101, 230, 160, 200 - trail * 32);
        painter.setPen(Qt::NoPen);
        painter.setBrush(colour);
        painter.drawPie(QRectF(centre.x() - radius, centre.y() - radius, 2 * radius, 2 * radius),
                        int((-angle + trail * 8) * 16), 8 * 16);
    }
    return image;
}

#include "frameprovider.moc"
