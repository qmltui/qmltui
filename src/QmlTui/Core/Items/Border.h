#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QObject>
#include <QString>
#include <QQmlEngine>

#include <QmlTui/Core/Primitives/Color.h>

namespace qmltui {

class Item;

// Grouped property on Rectangle (`border { style: ...; color: ... }`) drawing
// Unicode or ASCII box-drawing characters around the rectangle perimeter.
class QMLTUI_CORE_EXPORT Border : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Border is only available as a grouped property of Rectangle")

public:
    enum Style : uint8_t {
        None = 0,
        Single,
        Double,
        Rounded,
        Ascii,
    };
    Q_ENUM(Style)

    Q_PROPERTY(Style style READ style WRITE setStyle NOTIFY styleChanged)
    Q_PROPERTY(qmltui::Color color READ color WRITE setColor NOTIFY colorChanged)

    explicit Border(Item* owner);
    ~Border() override;

    Style style() const { return m_style; }
    void setStyle(Style style);
    void setStyle(const QString& style);

    Color color() const { return m_color; }
    void setColor(Color color);

    static Style styleFromString(QStringView str, Style defaultStyle = None);
    static QString styleToString(Style style);

signals:
    void styleChanged();
    void colorChanged();

private:
    Item* m_owner;
    Style m_style = None;
    Color m_color;
};

} // namespace qmltui
