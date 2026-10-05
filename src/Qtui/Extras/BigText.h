#pragma once

#include <Item.h>
#include <QString>

namespace qtui {

class BigText : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QString bgColor READ bgColor WRITE setBgColor NOTIFY bgColorChanged)

public:
    explicit BigText(QObject *parent = nullptr);
    ~BigText() override = default;

    QString text() const { return m_text; }
    void setText(const QString &text);

    QString color() const { return m_color; }
    void setColor(const QString &color);

    QString bgColor() const { return m_bgColor; }
    void setBgColor(const QString &color);

    void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) override;

signals:
    void textChanged();
    void colorChanged();
    void bgColorChanged();

private:
    QString m_text;

    QString m_color = "yellow";
    uint32_t m_resolvedColor = 0xFFFFFF00;

    QString m_bgColor = "transparent";
    uint32_t m_resolvedBgColor = 0x00000000;

};

} // namespace qtui
