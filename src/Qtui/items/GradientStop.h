#pragma once

#include <QObject>
#include <QString>
#include <cstdint>
#include <QtQml/qqmlregistration.h>

#include "../primitives/Color.h"

namespace qtui {

class GradientStop : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qtui::Color color READ color WRITE setColor NOTIFY colorChanged)

public:
    explicit GradientStop(QObject *parent = nullptr);

    qreal position() const { return m_position; }
    void setPosition(qreal pos);

    Color color() const { return m_color; }
    void setColor(Color color);

    uint32_t resolvedColor() const { return m_resolvedColor; }

signals:
    void positionChanged();
    void colorChanged();
    void updated();

private:
    qreal m_position = 0.0;
    Color m_color = "white";
    uint32_t m_resolvedColor = 0xFFFFFFFF;
};

} // namespace qtui
