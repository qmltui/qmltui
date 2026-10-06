#pragma once

#include <Item.h>
#include <QString>

namespace qtui {

class BigText : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(qtui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qtui::Color bgColor READ bgColor WRITE setBgColor NOTIFY bgColorChanged)

public:
    explicit BigText(QObject *parent = nullptr);
    ~BigText() override = default;

    QString text() const { return m_text; }
    void setText(const QString &text);

    Color color() const { return m_color; }
    void setColor(Color color);

    Color bgColor() const { return m_bgColor; }
    void setBgColor(Color color);

    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) const override;

signals:
    void textChanged();
    void colorChanged();
    void bgColorChanged();

private:
    QString m_text;

    Color m_color = "yellow";
    uint32_t m_resolvedColor = 0xFFFFFF00;

    Color m_bgColor = "transparent";
    uint32_t m_resolvedBgColor = 0x00000000;
};

} // namespace qtui
