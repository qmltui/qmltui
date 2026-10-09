#pragma once

#include <Qtui/Core/primitives/QtuiExport.h>

#include <QObject>
#include <QQmlListProperty>
#include <QtQml/qqmlregistration.h>
#include <QString>
#include <QList>
#include <cstdint>
#include <Qtui/Core/items/GradientStop.h>

namespace qtui {

class QTUI_CORE_EXPORT Gradient : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQmlListProperty<qtui::GradientStop> stops READ stops CONSTANT)
    Q_PROPERTY(qreal offset READ offset WRITE setOffset NOTIFY offsetChanged)
    Q_PROPERTY(Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_CLASSINFO("DefaultProperty", "stops")

public:
    enum Orientation {
        Vertical,
        Horizontal
    };
    Q_ENUM(Orientation)

    explicit Gradient(QObject *parent = nullptr);

    QQmlListProperty<GradientStop> stops();
    const QList<GradientStop*>& stopsList() const { return m_stops; }

    qreal offset() const { return m_offset; }
    void setOffset(qreal offset);

    Orientation orientation() const { return m_orientation; }
    void setOrientation(Orientation orientation);

    void addStop(GradientStop* stop);
    void clearStops();

    uint32_t colorAt(qreal t) const;

signals:
    void offsetChanged();
    void orientationChanged();
    void updated();

private:
    static void appendStop(QQmlListProperty<GradientStop>* list, GradientStop* stop);
    static qsizetype countStops(QQmlListProperty<GradientStop>* list);
    static GradientStop* stopAt(QQmlListProperty<GradientStop>* list, qsizetype index);
    static void clearStops(QQmlListProperty<GradientStop>* list);

    QList<GradientStop*> m_stops;
    qreal m_offset = 0.0;
    Orientation m_orientation = Vertical;
};

} // namespace qtui
