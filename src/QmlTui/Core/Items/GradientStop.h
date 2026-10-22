#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <cstdint>

#include <QmlTui/Core/Primitives/Color.h>

namespace qmltui {

// Color stop at normalized position [0.0, 1.0] inside a Gradient.
class QMLTUI_CORE_EXPORT GradientStop : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qmltui::Color color READ color WRITE setColor NOTIFY colorChanged)

public:
    explicit GradientStop(QObject* parent = nullptr);

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
    Color m_color = Color::fromAnsiIndex(15);
    uint32_t m_resolvedColor = Color::resolve(m_color, 0xFFFFFFFF);
};

} // namespace qmltui
