#pragma once

#include <Qamui/Core/primitives/QamuiExport.h>

#include <Qamui/Core/items/Item.h>

#include <QHash>

namespace qamui {

class QAMUI_CORE_EXPORT Column : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)

public:
    explicit Column(QObject *parent = nullptr);
    ~Column() override;

    int spacing() const { return m_spacing; }
    void setSpacing(int spacing);

    int implicitWidth() const override;
    int implicitHeight() const override;
    void updateLayout() override;
    void layoutChildren();

signals:
    void spacingChanged();

private:
    // Like Qt Quick's Column, only y is owned by the layout. A child's x is
    // its own offset from the left padding, so the layout remembers that
    // offset and re-adds the padding instead of overwriting x.
    struct Placement {
        int ownX = 0;
        QMetaObject::Connection xChanged;
        QMetaObject::Connection destroyed;
    };
    Placement track(Item *childItem);

    QHash<const QObject*, Placement> m_placements;
    bool m_positioning = false;
    int m_spacing = 0;
};

} // namespace qamui
