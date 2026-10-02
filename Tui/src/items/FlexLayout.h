#pragma once

#include "Item.h"

namespace qtui {

class FlexLayout : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(FlexDirection flexDirection READ flexDirection WRITE setFlexDirection NOTIFY flexDirectionChanged)
    Q_PROPERTY(FlexWrap flexWrap READ flexWrap WRITE setFlexWrap NOTIFY flexWrapChanged)
    Q_PROPERTY(JustifyContent justifyContent READ justifyContent WRITE setJustifyContent NOTIFY justifyContentChanged)
    Q_PROPERTY(AlignItems alignItems READ alignItems WRITE setAlignItems NOTIFY alignItemsChanged)

public:
    enum FlexDirection {
        Row,
        Column,
        RowReverse,
        ColumnReverse
    };
    Q_ENUM(FlexDirection)

    enum FlexWrap {
        NoWrap,
        Wrap
    };
    Q_ENUM(FlexWrap)

    enum JustifyContent {
        FlexStart,
        FlexEnd,
        Center,
        SpaceBetween,
        SpaceAround,
        SpaceEvenly
    };
    Q_ENUM(JustifyContent)

    enum AlignItems {
        AlignStart,
        AlignEnd,
        AlignCenter,
        AlignStretch
    };
    Q_ENUM(AlignItems)

    explicit FlexLayout(QObject *parent = nullptr);
    ~FlexLayout() override;

    FlexDirection flexDirection() const { return m_flexDirection; }
    void setFlexDirection(FlexDirection dir);

    FlexWrap flexWrap() const { return m_flexWrap; }
    void setFlexWrap(FlexWrap wrap);

    JustifyContent justifyContent() const { return m_justifyContent; }
    void setJustifyContent(JustifyContent justify);

    AlignItems alignItems() const { return m_alignItems; }
    void setAlignItems(AlignItems align);

    void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) override;

signals:
    void flexDirectionChanged();
    void flexWrapChanged();
    void justifyContentChanged();
    void alignItemsChanged();

private:
    void doLayout();

    FlexDirection m_flexDirection = Row;
    FlexWrap m_flexWrap = NoWrap;
    JustifyContent m_justifyContent = FlexStart;
    AlignItems m_alignItems = AlignStretch;
};

} // namespace qtui
