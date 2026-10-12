#pragma once

#include <Qamui/Core/Primitives/QamuiExport.h>

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <cstdint>

#include <Qamui/Core/Primitives/Color.h>
#include <Qamui/Core/Primitives/ColorMapper.h>

namespace qamui {

class QAMUI_CORE_EXPORT GradientStop : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qamui::Color color READ color WRITE setColor NOTIFY colorChanged)

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
    // Resolved caches mirror the Color members: an ANSI-named default must not be
    // written as literal RGB, or the terminal theme stops applying to it (setters
    // early-return when the color is unchanged, so the initializer is the only chance).
    uint32_t m_resolvedColor = ColorMapper::fromString(m_color, 0xFFFFFFFF);
};

} // namespace qamui
