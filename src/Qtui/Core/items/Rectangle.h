#pragma once

#include <Qtui/Core/primitives/QtuiExport.h>

#include <Qtui/Core/items/Item.h>
#include <Qtui/Core/items/Border.h>

namespace qtui {

class Gradient;

class QTUI_CORE_EXPORT Rectangle : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qtui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qtui::Border* border READ border CONSTANT)
    Q_PROPERTY(qtui::Gradient* gradient READ gradient WRITE setGradient NOTIFY gradientChanged)

public:
    explicit Rectangle(QObject *parent = nullptr);
    ~Rectangle() override;

    Color color() const { return m_color; }
    void setColor(Color color);

    Gradient* gradient() const { return m_gradient; }
    void setGradient(Gradient* gradient);

    uint32_t resolvedBackgroundColor() const { return m_resolvedBackgroundColor; }

    Border* border() { return &m_border; }

    int leftPadding() const override;
    int rightPadding() const override;
    int topPadding() const override;
    int bottomPadding() const override;

    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) const override;

signals:
    void colorChanged();
    void gradientChanged();

private:
    Color m_color = "transparent";
    uint32_t m_resolvedBackgroundColor = 0x00000000;
    Border m_border;
    Gradient* m_gradient = nullptr;
};

} // namespace qtui
