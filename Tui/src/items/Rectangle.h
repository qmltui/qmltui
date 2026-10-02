#pragma once

#include "Item.h"
#include "Border.h"

namespace qtui {

class Rectangle : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QString backgroundColor READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qtui::Border* border READ border CONSTANT)

public:
    explicit Rectangle(QObject *parent = nullptr);
    ~Rectangle() override;

    QString color() const { return m_color; }
    void setColor(const QString &color);

    uint32_t resolvedBgColor() const { return m_resolvedBgColor; }

    Border* border() { return &m_border; }

    int leftPadding() const override;
    int rightPadding() const override;
    int topPadding() const override;
    int bottomPadding() const override;

    void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) override;

signals:
    void colorChanged();

private:
    QString m_color = "transparent";
    uint32_t m_resolvedBgColor = 0x00000000;
    Border m_border;
};

} // namespace qtui
