#pragma once

#include <QObject>
#include <QQmlListProperty>
#include <QtQml/qqmlregistration.h>
#include <QString>
#include <QList>
#include <cstdint>
#include "GradientStop.h"

namespace qtui {

class Gradient : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQmlListProperty<qtui::GradientStop> stops READ stops CONSTANT)
    Q_PROPERTY(qreal offset READ offset WRITE setOffset NOTIFY offsetChanged)
    Q_CLASSINFO("DefaultProperty", "stops")

public:
    explicit Gradient(QObject *parent = nullptr);

    QQmlListProperty<GradientStop> stops();
    const QList<GradientStop*>& stopsList() const { return m_stops; }

    qreal offset() const { return m_offset; }
    void setOffset(qreal offset);

    uint32_t colorAt(qreal t) const;

signals:
    void offsetChanged();
    void updated();

private:
    static void appendStop(QQmlListProperty<GradientStop>* list, GradientStop* stop);
    static qsizetype countStops(QQmlListProperty<GradientStop>* list);
    static GradientStop* stopAt(QQmlListProperty<GradientStop>* list, qsizetype index);
    static void clearStops(QQmlListProperty<GradientStop>* list);

    QList<GradientStop*> m_stops;
    qreal m_offset = 0.0;
};

} // namespace qtui
