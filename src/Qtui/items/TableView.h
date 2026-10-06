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
#include <QSet>
#include <utility>

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

    Q_PROPERTY(qtui::Color headerColor READ headerColor WRITE setHeaderColor NOTIFY headerColorChanged)
    Q_PROPERTY(qtui::Color headerBgColor READ headerBgColor WRITE setHeaderBgColor NOTIFY headerBgColorChanged)
    Q_PROPERTY(qtui::Color selectionColor READ selectionColor WRITE setSelectionColor NOTIFY selectionColorChanged)
    Q_PROPERTY(qtui::Color selectionBgColor READ selectionBgColor WRITE setSelectionBgColor NOTIFY selectionBgColorChanged)
    Q_PROPERTY(qtui::Color textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
    Q_PROPERTY(qtui::Color rowBgColor READ rowBgColor WRITE setRowBgColor NOTIFY rowBgColorChanged)
    Q_PROPERTY(qtui::Color alternatingRowBgColor READ alternatingRowBgColor WRITE setAlternatingRowBgColor NOTIFY alternatingRowBgColorChanged)

    Q_PROPERTY(bool showGrid READ showGrid WRITE setShowGrid NOTIFY showGridChanged)

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

    Q_INVOKABLE void forceLayout();

    Color headerColor() const { return m_headerColor; }
    void setHeaderColor(Color color);

    Color headerBgColor() const { return m_headerBgColor; }
    void setHeaderBgColor(Color color);

    Color selectionColor() const { return m_selectionColor; }
    void setSelectionColor(Color color);

    Color selectionBgColor() const { return m_selectionBgColor; }
    void setSelectionBgColor(Color color);

    Color textColor() const { return m_textColor; }
    void setTextColor(Color color);

    Color rowBgColor() const { return m_rowBgColor; }
    void setRowBgColor(Color color);

    Color alternatingRowBgColor() const { return m_alternatingRowBgColor; }
    void setAlternatingRowBgColor(Color color);

    bool showGrid() const { return m_showGrid; }
    void setShowGrid(bool show);


    QJSValue columnWidthProvider() const { return m_columnWidthProvider; }
    void setColumnWidthProvider(const QJSValue &provider);

    QJSValue cellFormat() const { return m_cellFormat; }
    void setCellFormat(const QJSValue &format);

    QQmlComponent* delegate() const { return m_delegate; }
    void setDelegate(QQmlComponent *delegate);
    QQmlComponent* rowDelegate() const { return m_rowDelegate; }
    void setRowDelegate(QQmlComponent *delegate);

    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) const override;
    void keyPressEvent(KeyEvent *event) override;
    void mousePressEvent(MouseEvent *event) override;

signals:
    void columnsChanged();
    void modelChanged();
    void selectedRowChanged();
    void currentColumnChanged();
    void alternatingRowsChanged();

    void headerColorChanged();
    void headerBgColorChanged();
    void selectionColorChanged();
    void selectionBgColorChanged();
    void textColorChanged();
    void rowBgColorChanged();
    void alternatingRowBgColorChanged();

    void showGridChanged();
    void columnWidthProviderChanged();
    void cellFormatChanged();
    void delegateChanged();
    void rowDelegateChanged();

    void rowActivated(int index);
    void headerClicked(int column);

protected:
    // Computes the scroll offset and creates, positions and shows delegate
    // items. Runs in the layout pass, never from paint().
    void updateLayout() override;

private:
    ClipRect clipRectForChildren(int absX, int absY,
                                const ClipRect &clipRect) const override;
    int rowCount() const;
    int columnCount() const;
    int headerRows() const;
    int computeFirstVisibleRow() const;
    void layoutDelegates();
    using Position = std::pair<int, int>;
    struct DelegateRecord {
        QPointer<Item> item;
        QPointer<QQmlComponent> component;
        QPersistentModelIndex source;
        QVariantMap values;
        QVariant modelData;
        QSet<int> dirtyRoles;
        bool allRolesDirty = true;
        bool loaded = false;
        bool current = false;
        int row = -1;
        int column = -1;
    };
    struct ColumnGeometry {
        int column;
        int x;
        int width;
        QString title;
        QString alignment;
    };
    QModelIndex sourceIndex(int row, int column) const;
    void updateDelegate(DelegateRecord &record, int row, int column);
    void updateDelegateState(DelegateRecord &record, int row, int column);
    void refreshDelegateStates(Position previous);
    void handleStructureChanged();
    void synchronizeSelection();
    void clearSelection();
    void handleDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                           const QList<int> &roles);
    QString cellText(int row, int col) const;
    void resolveColumns();
    int resolveColumnWidth(int column, const QVariantMap &descriptor);
    void callbackWarning(const QString &message, QSet<QString> &errors) const;

    void clearDelegateItems();
    void generateDelegateItems();
    static void destroyDelegate(DelegateRecord &record);

    QVariantList m_columns;
    QVariant m_modelVariant;
    QVariantList m_listModel;
    QPointer<QAbstractItemModel> m_cppModel;

    int m_selectedRow = -1;
    int m_currentColumn = -1;
    int m_firstVisibleRow = 0;
    bool m_alternatingRows = false;

    QPersistentModelIndex m_currentIndex;
    bool m_resetDelegates = false;
    bool m_structureDirty = false;
    bool m_geometryDirty = true;
    QHash<int, QByteArray> m_roleNames;
    QList<ColumnGeometry> m_geometry;
    mutable QSet<QString> m_widthErrors;
    mutable QSet<QString> m_formatErrors;
    QHash<QPersistentModelIndex, QHash<int, QVariant>> m_fetchedRoles;

    Color m_headerColor = "yellow";
    uint32_t m_resolvedHeaderColor = 0xFFFFFF00;
    Color m_headerBgColor = "transparent";
    uint32_t m_resolvedHeaderBgColor = 0x00000000;

    Color m_selectionColor = "white";
    uint32_t m_resolvedSelectionColor = 0xFFFFFFFF;
    Color m_selectionBgColor = "blue";
    uint32_t m_resolvedSelectionBgColor = 0xFF0000FF;

    Color m_textColor = "gray";
    uint32_t m_resolvedTextColor = 0xFFCCCCCC;
    Color m_rowBgColor = "transparent";
    uint32_t m_resolvedRowBgColor = 0x00000000;

    Color m_alternatingRowBgColor = "#1e1e2e";
    uint32_t m_resolvedAltRowBgColor = 0xFF1E1E2E;

    bool m_showGrid = false;

    QJSValue m_columnWidthProvider;
    QJSValue m_cellFormat;
    QPointer<QQmlComponent> m_delegate;
    QPointer<QQmlComponent> m_rowDelegate;
    QHash<Position, DelegateRecord> m_createdDelegateItems;
};

} // namespace qtui
