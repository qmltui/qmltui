#pragma once

#include <Qtui/Core/primitives/QtuiExport.h>

#include <Qtui/Core/items/Item.h>
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
#include <Qtui/Core/primitives/ColorMapper.h>

namespace qtui {

class QTUI_CORE_EXPORT TableView : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList columns READ columns WRITE setColumns NOTIFY columnsChanged)
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(int selectedRow READ selectedRow WRITE setSelectedRow NOTIFY selectedRowChanged)
    Q_PROPERTY(int currentRow READ selectedRow NOTIFY selectedRowChanged)
    Q_PROPERTY(int currentColumn READ currentColumn WRITE setCurrentColumn NOTIFY currentColumnChanged)
    Q_PROPERTY(bool alternatingRows READ alternatingRows WRITE setAlternatingRows NOTIFY alternatingRowsChanged)

    Q_PROPERTY(qtui::Color headerColor READ headerColor WRITE setHeaderColor NOTIFY headerColorChanged)
    Q_PROPERTY(qtui::Color headerBackgroundColor READ headerBackgroundColor WRITE setHeaderBackgroundColor NOTIFY headerBackgroundColorChanged)
    Q_PROPERTY(qtui::Color selectionColor READ selectionColor WRITE setSelectionColor NOTIFY selectionColorChanged)
    Q_PROPERTY(qtui::Color selectionBackgroundColor READ selectionBackgroundColor WRITE setSelectionBackgroundColor NOTIFY selectionBackgroundColorChanged)
    Q_PROPERTY(qtui::Color textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
    Q_PROPERTY(qtui::Color rowBackgroundColor READ rowBackgroundColor WRITE setRowBackgroundColor NOTIFY rowBackgroundColorChanged)
    Q_PROPERTY(qtui::Color alternatingRowBackgroundColor READ alternatingRowBackgroundColor WRITE setAlternatingRowBackgroundColor NOTIFY alternatingRowBackgroundColorChanged)

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

    Color headerBackgroundColor() const { return m_headerBackgroundColor; }
    void setHeaderBackgroundColor(Color color);

    Color selectionColor() const { return m_selectionColor; }
    void setSelectionColor(Color color);

    Color selectionBackgroundColor() const { return m_selectionBackgroundColor; }
    void setSelectionBackgroundColor(Color color);

    Color textColor() const { return m_textColor; }
    void setTextColor(Color color);

    Color rowBackgroundColor() const { return m_rowBackgroundColor; }
    void setRowBackgroundColor(Color color);

    Color alternatingRowBackgroundColor() const { return m_alternatingRowBackgroundColor; }
    void setAlternatingRowBackgroundColor(Color color);

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
    void headerBackgroundColorChanged();
    void selectionColorChanged();
    void selectionBackgroundColorChanged();
    void textColorChanged();
    void rowBackgroundColorChanged();
    void alternatingRowBackgroundColorChanged();

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
    void maybeFetchMore();
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
    QVariant modelHeader(int column, int role) const;
    void refreshModelHeader();
    static QString alignmentName(const QVariant &alignment);
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
    bool m_modelProvidesHeader = false;
    bool m_fetching = false;
    QHash<int, QByteArray> m_roleNames;
    QList<ColumnGeometry> m_geometry;
    mutable QSet<QString> m_widthErrors;
    mutable QSet<QString> m_formatErrors;
    QHash<QPersistentModelIndex, QHash<int, QVariant>> m_fetchedRoles;

    Color m_headerColor = "default";
    // Resolved caches mirror the Color members: an ANSI-named default must not be
    // written as literal RGB, or the terminal theme stops applying to it (setters
    // early-return when the color is unchanged, so the initializer is the only chance).
    uint32_t m_resolvedHeaderColor = ColorMapper::fromString(m_headerColor, 0);
    Color m_headerBackgroundColor = "transparent";
    uint32_t m_resolvedHeaderBackgroundColor = 0x00000000;

    Color m_selectionColor;
    uint32_t m_resolvedSelectionColor = ColorMapper::fromString(m_selectionColor, 0);
    Color m_selectionBackgroundColor;
    uint32_t m_resolvedSelectionBackgroundColor = ColorMapper::fromString(m_selectionBackgroundColor, 0);

    Color m_textColor = "default";
    uint32_t m_resolvedTextColor = ColorMapper::fromString(m_textColor, 0);
    Color m_rowBackgroundColor = "transparent";
    uint32_t m_resolvedRowBackgroundColor = 0x00000000;

    Color m_alternatingRowBackgroundColor = "default";
    uint32_t m_resolvedAltRowBackgroundColor = 0;

    bool m_showGrid = false;

    QJSValue m_columnWidthProvider;
    QJSValue m_cellFormat;
    QPointer<QQmlComponent> m_delegate;
    QPointer<QQmlComponent> m_rowDelegate;
    QHash<Position, DelegateRecord> m_createdDelegateItems;
};

} // namespace qtui
