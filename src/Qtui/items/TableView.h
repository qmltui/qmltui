#pragma once

#include "Item.h"
#include <QVariantList>
#include <QVariantMap>
#include <QAbstractItemModel>
#include <QPointer>
#include <QPersistentModelIndex>
#include <QJSValue>
#include <QQmlComponent>
#include <QHash>
#include <QPair>

namespace qtui {

class TableView : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList columns READ columns WRITE setColumns NOTIFY columnsChanged)
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(int selectedRow READ selectedRow WRITE setSelectedRow NOTIFY selectedRowChanged)
    Q_PROPERTY(int currentRow READ selectedRow NOTIFY selectedRowChanged)
    Q_PROPERTY(int currentColumn READ currentColumn WRITE setCurrentColumn NOTIFY currentColumnChanged)
    Q_PROPERTY(bool alternatingRows READ alternatingRows WRITE setAlternatingRows NOTIFY alternatingRowsChanged)

    Q_PROPERTY(int sortColumn READ sortColumn WRITE setSortColumn NOTIFY sortColumnChanged)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged)

    Q_PROPERTY(QString headerColor READ headerColor WRITE setHeaderColor NOTIFY headerColorChanged)
    Q_PROPERTY(QString headerBgColor READ headerBgColor WRITE setHeaderBgColor NOTIFY headerBgColorChanged)
    Q_PROPERTY(QString selectionColor READ selectionColor WRITE setSelectionColor NOTIFY selectionColorChanged)
    Q_PROPERTY(QString selectionBgColor READ selectionBgColor WRITE setSelectionBgColor NOTIFY selectionBgColorChanged)
    Q_PROPERTY(QString textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
    Q_PROPERTY(QString rowBgColor READ rowBgColor WRITE setRowBgColor NOTIFY rowBgColorChanged)
    Q_PROPERTY(QString alternatingRowBgColor READ alternatingRowBgColor WRITE setAlternatingRowBgColor NOTIFY alternatingRowBgColorChanged)

    Q_PROPERTY(bool showGrid READ showGrid WRITE setShowGrid NOTIFY showGridChanged)
    Q_PROPERTY(QString borderStyle READ borderStyle WRITE setBorderStyle NOTIFY borderStyleChanged)
    Q_PROPERTY(QString borderColor READ borderColor WRITE setBorderColor NOTIFY borderColorChanged)

    Q_PROPERTY(QJSValue columnWidthProvider READ columnWidthProvider WRITE setColumnWidthProvider NOTIFY columnWidthProviderChanged)
    Q_PROPERTY(QJSValue cellFormat READ cellFormat WRITE setCellFormat NOTIFY cellFormatChanged)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(QQmlComponent* rowDelegate READ rowDelegate WRITE setRowDelegate NOTIFY rowDelegateChanged)

public:
    explicit TableView(QObject *parent = nullptr);
    ~TableView() override;

    QVariantList columns() const { return m_columns; }
    void setColumns(const QVariantList &columns);

    QVariant model() const { return m_modelVariant; }
    void setModel(const QVariant &model);

    int selectedRow() const { return m_selectedRow; }
    void setSelectedRow(int row);

    int currentColumn() const { return m_currentColumn; }
    void setCurrentColumn(int col);

    bool alternatingRows() const { return m_alternatingRows; }
    void setAlternatingRows(bool alt);

    int sortColumn() const { return m_sortColumn; }
    void setSortColumn(int col);

    Qt::SortOrder sortOrder() const { return m_sortOrder; }
    void setSortOrder(Qt::SortOrder order);

    Q_INVOKABLE void sortByColumn(int column, Qt::SortOrder order = Qt::AscendingOrder);

    QString headerColor() const { return m_headerColor; }
    void setHeaderColor(const QString &color);

    QString headerBgColor() const { return m_headerBgColor; }
    void setHeaderBgColor(const QString &color);

    QString selectionColor() const { return m_selectionColor; }
    void setSelectionColor(const QString &color);

    QString selectionBgColor() const { return m_selectionBgColor; }
    void setSelectionBgColor(const QString &color);

    QString textColor() const { return m_textColor; }
    void setTextColor(const QString &color);

    QString rowBgColor() const { return m_rowBgColor; }
    void setRowBgColor(const QString &color);

    QString alternatingRowBgColor() const { return m_alternatingRowBgColor; }
    void setAlternatingRowBgColor(const QString &color);

    bool showGrid() const { return m_showGrid; }
    void setShowGrid(bool show);

    QString borderStyle() const { return m_borderStyle; }
    void setBorderStyle(const QString &style);

    QString borderColor() const { return m_borderColor; }
    void setBorderColor(const QString &color);

    QJSValue columnWidthProvider() const { return m_columnWidthProvider; }
    void setColumnWidthProvider(const QJSValue &provider);

    QJSValue cellFormat() const { return m_cellFormat; }
    void setCellFormat(const QJSValue &format);

    QQmlComponent* delegate() const { return m_delegate; }
    void setDelegate(QQmlComponent *delegate);
    QQmlComponent* rowDelegate() const { return m_rowDelegate; }
    void setRowDelegate(QQmlComponent *delegate);

    void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) override;
    void keyPressEvent(KeyEvent *event) override;
    void mousePressEvent(MouseEvent *event) override;

signals:
    void columnsChanged();
    void modelChanged();
    void selectedRowChanged();
    void currentColumnChanged();
    void alternatingRowsChanged();

    void sortColumnChanged();
    void sortOrderChanged();

    void headerColorChanged();
    void headerBgColorChanged();
    void selectionColorChanged();
    void selectionBgColorChanged();
    void textColorChanged();
    void rowBgColorChanged();
    void alternatingRowBgColorChanged();

    void showGridChanged();
    void borderStyleChanged();
    void borderColorChanged();
    void columnWidthProviderChanged();
    void cellFormatChanged();
    void delegateChanged();
    void rowDelegateChanged();

    void rowActivated(int index);

private:
    int rowCount() const;
    QString cellText(int row, int col) const;
    int columnWidth(int col) const;
    QString columnAlignment(int col) const;
    void sortModel();

    void clearDelegateItems();
    void generateDelegateItems();
    Item* createSingleDelegateItem(int row, int column);
    void updateDelegateViewport(int firstRow, int lastRow, int firstCol, int lastCol);

    QVariantList m_columns;
    QVariant m_modelVariant;
    QVariantList m_listModel;
    QPointer<QAbstractItemModel> m_cppModel;

    int m_selectedRow = -1;
    int m_currentColumn = -1;
    bool m_alternatingRows = false;

    int m_sortColumn = -1;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;

    QString m_headerColor = "yellow";
    uint32_t m_resolvedHeaderColor = 0xFFFFFF00;
    QString m_headerBgColor = "transparent";
    uint32_t m_resolvedHeaderBgColor = 0x00000000;

    QString m_selectionColor = "white";
    uint32_t m_resolvedSelectionColor = 0xFFFFFFFF;
    QString m_selectionBgColor = "blue";
    uint32_t m_resolvedSelectionBgColor = 0xFF0000FF;

    QString m_textColor = "gray";
    uint32_t m_resolvedTextColor = 0xFFCCCCCC;
    QString m_rowBgColor = "transparent";
    uint32_t m_resolvedRowBgColor = 0x00000000;

    QString m_alternatingRowBgColor = "#1e1e2e";
    uint32_t m_resolvedAltRowBgColor = 0xFF1E1E2E;

    bool m_showGrid = false;
    QString m_borderStyle = "none";
    QString m_borderColor = "gray";
    uint32_t m_resolvedBorderColor = 0xFF808080;

    QJSValue m_columnWidthProvider;
    QJSValue m_cellFormat;
    QPointer<QQmlComponent> m_delegate;
    QPointer<QQmlComponent> m_rowDelegate;
    QHash<QPair<int, int>, Item*> m_createdDelegateItems;
};

} // namespace qtui
