#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QObject>
#include <QQmlListProperty>
#include <QtQml/qqmlregistration.h>
#include <QList>
#include <cstdint>
#include <QPointer>
#include <QmlTui/Core/Items/GradientStop.h>

namespace qmltui {

// Multi-stop linear color gradient used by Rectangle.gradient and Text.gradient.
// Interpolates RGB colors across sorted GradientStops along Vertical or
// Horizontal orientation, shifted by `offset` (wrapping in [0.0, 1.0)).
class QMLTUI_CORE_EXPORT Gradient : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQmlListProperty<qmltui::GradientStop> stops READ stops CONSTANT)
    Q_PROPERTY(qreal offset READ offset WRITE setOffset NOTIFY offsetChanged)
    Q_PROPERTY(
        Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_CLASSINFO("DefaultProperty", "stops")

public:
    enum Orientation : uint8_t {
        Vertical,
        Horizontal,
    };
    Q_ENUM(Orientation)

    explicit Gradient(QObject* parent = nullptr);

    QQmlListProperty<GradientStop> stops() const;
    const QList<QPointer<GradientStop>>& stopsList() const { return m_stops; }

    qreal offset() const { return m_offset; }
    void setOffset(qreal offset);

    Orientation orientation() const { return m_orientation; }
    void setOrientation(Orientation orientation);

    void addStop(GradientStop* stop);
    void clearStops();

    // Evaluates the interpolated ARGB color at normalized position t in [0.0, 1.0].
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

    QList<QPointer<GradientStop>> m_stops;
    mutable QList<GradientStop*> m_sortedStops;
    mutable bool m_stopsDirty = true;
    qreal m_offset = 0.0;
    Orientation m_orientation = Vertical;
};

} // namespace qmltui
