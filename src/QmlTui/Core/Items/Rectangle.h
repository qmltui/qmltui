#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/Border.h>
#include <QmlTui/Core/Items/Gradient.h>
#include <QPointer>

namespace qmltui {

/**
 * @brief Filled or bordered rectangular terminal area with optional gradient.
 *
 * Renders a rectangular region with a fill color or vertical/horizontal gradient,
 * and an optional outer border drawn with Unicode box-drawing characters.
 *
 * @thread_safety Main/engine thread only.
 * @see Border, Gradient, Item
 */
class QMLTUI_CORE_EXPORT Rectangle : public Item {
    Q_OBJECT
    QML_ELEMENT

    /// @brief Fill color of the rectangle. Defaults to "transparent".
    Q_PROPERTY(qmltui::Color color READ color WRITE setColor NOTIFY colorChanged)
    /// @brief Attached border configuration (style: "single", "double", "rounded", "heavy", "none", color).
    Q_PROPERTY(qmltui::Border* border READ border CONSTANT)
    /// @brief Optional gradient fill applied across the interior.
    Q_PROPERTY(qmltui::Gradient* gradient READ gradient WRITE setGradient NOTIFY gradientChanged)

public:
    explicit Rectangle(QObject *parent = nullptr);
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

    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) const override;

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
