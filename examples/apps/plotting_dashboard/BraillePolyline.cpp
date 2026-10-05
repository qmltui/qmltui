#include "BraillePolyline.h"

#include <QJSValue>
#include <QVariantMap>

namespace qtui {

BraillePolyline::BraillePolyline(QObject *parent)
    : CanvasShape(parent)
{
}

void BraillePolyline::setPoints(const QVariantList &points)
{
    if (m_points == points)
        return;

    m_parsedPoints.clear();

    // The plotting example uses a packed [x0, y0, x1, y1, ...] list.  Keep
    // accepting object-form points for API compatibility, but avoid creating
    // and introspecting a JS object for every animated sample.
    const bool packed = points.size() >= 2 &&
                        points.at(0).userType() != qMetaTypeId<QJSValue>() &&
                        !points.at(0).canConvert<QVariantMap>();
    if (packed) {
        m_parsedPoints.reserve(points.size() / 2);
        for (qsizetype i = 0; i + 1 < points.size(); i += 2)
            m_parsedPoints.append(QPoint(points.at(i).toInt(), points.at(i + 1).toInt()));
        m_points = points;
        emit pointsChanged();
        if (parentItem())
            parentItem()->markDirty();
        return;
    }

    m_parsedPoints.reserve(points.size());

    for (const QVariant &value : points) {
        int x = 0;
        int y = 0;
        if (value.userType() == qMetaTypeId<QJSValue>()) {
            const QJSValue jsValue = value.value<QJSValue>();
            x = jsValue.property(QStringLiteral("x")).toInt();
            y = jsValue.property(QStringLiteral("y")).toInt();
        } else if (value.canConvert<QVariantMap>()) {
            const QVariantMap point = value.toMap();
            x = point.value(QStringLiteral("x")).toInt();
            y = point.value(QStringLiteral("y")).toInt();
        }
        m_parsedPoints.append(QPoint(x, y));
    }

    m_points = points;
    emit pointsChanged();
    if (parentItem())
        parentItem()->markDirty();
}

void BraillePolyline::draw(Canvas *canvas)
{
    if (!canvas || m_parsedPoints.isEmpty())
        return;

    const uint32_t color = resolvedColor(canvas);
    for (qsizetype index = 0; index < m_parsedPoints.size(); ++index) {
        const QPoint point = m_parsedPoints.at(index);
        const int cellX = point.x() / 2;
        const int leftPixelX = cellX * 2;

        canvas->setPixelColorHex(leftPixelX, point.y(), color);

        if (index + 1 >= m_parsedPoints.size())
            continue;

        const QPoint nextPoint = m_parsedPoints.at(index + 1);
        if (nextPoint.x() / 2 != cellX + 1)
            continue;

        const int midpointY = (point.y() + nextPoint.y()) / 2;
        canvas->setPixelColorHex(leftPixelX + 1, midpointY, color);
    }
}

} // namespace qtui
