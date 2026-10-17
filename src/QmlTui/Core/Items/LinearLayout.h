#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <cstdint>
#include <QmlTui/Core/Items/Item.h>

namespace qmltui {

// Shared box-layout base that distributes main-axis space across visible
// child items according to their Layout attached constraints and stretch factors.
class QMLTUI_CORE_EXPORT LinearLayout : public Item {
    Q_OBJECT
    Q_PROPERTY(int spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)

public:
    enum Direction : uint8_t { Horizontal, Vertical, };

    explicit LinearLayout(Direction direction, QObject *parent = nullptr);
    int spacing() const { return m_spacing; }
    void setSpacing(int spacing);
    int implicitWidth() const override;
    int implicitHeight() const override;
    void updateLayout() override;
    void layoutChildren();

signals:
    void spacingChanged();

private:
    int preferredMainSize(Item *item) const;
    int minimumMainSize(Item *item) const;
    int maximumMainSize(Item *item) const;

    Direction m_direction;
    int m_spacing = 0;
};

// Horizontal LinearLayout (`RowLayout` in QML).
class QMLTUI_CORE_EXPORT RowLayout : public LinearLayout {
    Q_OBJECT
    QML_NAMED_ELEMENT(RowLayout)
public:
    explicit RowLayout(QObject *parent = nullptr) : LinearLayout(Horizontal, parent) {}
};

// Vertical LinearLayout (`ColumnLayout` in QML).
class QMLTUI_CORE_EXPORT ColumnLayout : public LinearLayout {
    Q_OBJECT
    QML_NAMED_ELEMENT(ColumnLayout)
public:
    explicit ColumnLayout(QObject *parent = nullptr) : LinearLayout(Vertical, parent) {}
};

} // namespace qmltui
