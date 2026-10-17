#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/Border.h>
#include <QmlTui/Core/Items/Gradient.h>
#include <QPointer>

namespace qmltui {

// Filled or bordered rectangular terminal item with optional gradient fill.
// When border.style is not Border.None, the border adds 1 cell of implicit
// padding on each side unless overridden on the Item.
class QMLTUI_CORE_EXPORT Rectangle : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qmltui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qmltui::Border* border READ border CONSTANT)
    Q_PROPERTY(qmltui::Gradient* gradient READ gradient WRITE setGradient NOTIFY gradientChanged)

public:
    explicit Rectangle(QObject* parent = nullptr);
    ~Rectangle() override;

    Color color() const { return m_color; }
    void setColor(Color color);

    Gradient* gradient() const { return m_gradient.data(); }
    void setGradient(Gradient* gradient);

    uint32_t resolvedBackgroundColor() const { return m_resolvedBackgroundColor; }

    Border* border() const { return const_cast<Border*>(&m_border); }

    int leftPadding() const override;
    int rightPadding() const override;
    int topPadding() const override;
    int bottomPadding() const override;

    void paint(TextGridBuffer& buffer, int absX, int absY,
               const ClipRect& clipRect = ClipRect()) const override;

signals:
    void colorChanged();
    void gradientChanged();

private:
    Color m_color = "transparent";
    uint32_t m_resolvedBackgroundColor = Color::TransparentTag;
    Border m_border;
    QPointer<Gradient> m_gradient;
};

} // namespace qmltui
