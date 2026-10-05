#pragma once

#include <Canvas.h>

#include <QPoint>
#include <QVariantList>

struct PlottingCanvasShapeForeign {
    Q_GADGET
    QML_FOREIGN(qtui::CanvasShape)
    QML_ANONYMOUS
};

namespace qtui {

class BraillePolyline : public CanvasShape {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList points READ points WRITE setPoints NOTIFY pointsChanged)

public:
    explicit BraillePolyline(QObject *parent = nullptr);

    QVariantList points() const { return m_points; }
    void setPoints(const QVariantList &points);

    void draw(Canvas *canvas) override;

signals:
    void pointsChanged();

private:
    QVariantList m_points;
    QList<QPoint> m_parsedPoints;
};

} // namespace qtui
