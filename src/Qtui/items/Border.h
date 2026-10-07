#pragma once

#include <QtuiExport.h>

#include <QObject>
#include <QString>
#include <QQmlEngine>

#include "../primitives/Color.h"

namespace qtui {

class Item;

class QTUI_CORE_EXPORT Border : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Border is only available as a grouped property of Rectangle")

    Q_PROPERTY(QString style READ style WRITE setStyle NOTIFY styleChanged)
    Q_PROPERTY(qtui::Color color READ color WRITE setColor NOTIFY colorChanged)

public:
    explicit Border(Item* owner);
    ~Border() override;

    QString style() const { return m_style; }
    void setStyle(const QString& style);

    Color color() const { return m_color; }
    void setColor(Color color);

signals:
    void styleChanged();
    void colorChanged();

private:
    Item* m_owner;
    QString m_style = "none";
    Color m_color = "default";
};

} // namespace qtui
