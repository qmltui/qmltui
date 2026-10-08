#include "frameprovider.h"

#include <QPainter>
#include <QUrlQuery>
#include <QtMath>

QImage FrameProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize) {
    // id is "<name>?<query>": the name selects the picture, the query its parameters.
    const int query = id.indexOf(QLatin1Char('?'));
    const QString name = query < 0 ? id : id.left(query);
    const QUrlQuery parameters(query < 0 ? QString() : id.mid(query + 1));
    if (name != QLatin1String("radar"))
        return QImage();   // unknown picture: the Image reports Error
    const int tick = parameters.queryItemValue(QStringLiteral("tick")).toInt();
    const QSize target = requestedSize.isValid() ? requestedSize : QSize(64, 32);
    QImage image = paintRadar(tick, target);
    *size = image.size();
    return image;
}

QImage FrameProvider::paintRadar(int tick, const QSize& size) {
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
