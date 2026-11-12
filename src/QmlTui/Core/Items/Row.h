#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Items/Item.h>

#include <QHash>

namespace qmltui {

// Positions visible child items horizontally with optional spacing, preserving
// each child's own y offset relative to topPadding.
class QMLTUI_CORE_EXPORT Row : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)

public:
    explicit Row(QObject* parent = nullptr);
    ~Row() override;

    int spacing() const { return m_spacing; }
    void setSpacing(int spacing);

    int implicitWidth() const override;
    int implicitHeight() const override;
    void updateLayout() override;
    void layoutChildren();

signals:
    void spacingChanged();

private:
    // Like Qt Quick's Row, only x is owned by the layout. A child's y is its
    // own offset from the top padding, so the layout remembers that offset
    // and re-adds the padding instead of overwriting y.
    struct Placement {
        int ownY = 0;
        QMetaObject::Connection yChanged;
        QMetaObject::Connection destroyed;
    };
    Placement track(Item* childItem);

    QHash<const QObject*, Placement> m_placements;
    bool m_positioning = false;
    int m_spacing = 0;
};

} // namespace qmltui
