#pragma once

#include <Qtui/Core/primitives/QtuiExport.h>

#include <Qtui/Core/items/Item.h>

namespace qtui {

class QTUI_CORE_EXPORT LinearLayout : public Item {
    Q_OBJECT
    Q_PROPERTY(int spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)

public:
    enum Direction { Horizontal, Vertical };

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

class QTUI_CORE_EXPORT RowLayout : public LinearLayout {
    Q_OBJECT
    QML_NAMED_ELEMENT(RowLayout)
public:
    explicit RowLayout(QObject *parent = nullptr) : LinearLayout(Horizontal, parent) {}
};

class QTUI_CORE_EXPORT ColumnLayout : public LinearLayout {
    Q_OBJECT
    QML_NAMED_ELEMENT(ColumnLayout)
public:
    explicit ColumnLayout(QObject *parent = nullptr) : LinearLayout(Vertical, parent) {}
};

} // namespace qtui
