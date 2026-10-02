#pragma once

#include <QObject>
#include <QString>
#include <QQmlEngine>

namespace qtui {

class Item;

class Border : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Border is only available as a grouped property of Rectangle")

    Q_PROPERTY(QString style READ style WRITE setStyle NOTIFY styleChanged)
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)

public:
    explicit Border(Item* owner);
    ~Border() override;

    QString style() const { return m_style; }
    void setStyle(const QString& style);

    QString color() const { return m_color; }
    void setColor(const QString& color);

signals:
    void styleChanged();
    void colorChanged();

private:
    Item* m_owner;
    QString m_style = "none";
    QString m_color = "white";
};

} // namespace qtui
