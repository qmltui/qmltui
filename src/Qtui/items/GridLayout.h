#pragma once

#include "Item.h"

namespace qtui {

class GridLayout : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int columns READ columns WRITE setColumns NOTIFY columnsChanged)
    Q_PROPERTY(int rows READ rows WRITE setRows NOTIFY rowsChanged)
    Q_PROPERTY(int columnSpacing READ columnSpacing WRITE setColumnSpacing NOTIFY columnSpacingChanged)
    Q_PROPERTY(int rowSpacing READ rowSpacing WRITE setRowSpacing NOTIFY rowSpacingChanged)
    Q_PROPERTY(Flow flow READ flow WRITE setFlow NOTIFY flowChanged)

public:
    enum Flow { LeftToRight, TopToBottom };
    Q_ENUM(Flow)

    explicit GridLayout(QObject *parent = nullptr);
    ~GridLayout() override;

    int columns() const { return m_columns; }
    void setColumns(int cols);

    int rows() const { return m_rows; }
    void setRows(int r);

    int columnSpacing() const { return m_columnSpacing; }
    void setColumnSpacing(int spacing);

    int rowSpacing() const { return m_rowSpacing; }
    void setRowSpacing(int spacing);

    Flow flow() const { return m_flow; }
    void setFlow(Flow flow);

    int implicitWidth() const override;
    int implicitHeight() const override;

    void render(TextGridBuffer &buffer, int absX, int absY,
                const ClipRect &clipRect = ClipRect()) override;

signals:
    void columnsChanged();
    void rowsChanged();
    void columnSpacingChanged();
    void rowSpacingChanged();
    void flowChanged();

private:
    void layoutChildren();

    int m_columns = -1;
    int m_rows = -1;
    int m_columnSpacing = 0;
    int m_rowSpacing = 0;
    Flow m_flow = LeftToRight;
};

} // namespace qtui
