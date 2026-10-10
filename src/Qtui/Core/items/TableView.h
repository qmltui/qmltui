#pragma once

#include <Qtui/Core/primitives/QtuiExport.h>

#include <Qtui/Core/items/Item.h>
#include <QVariantList>
#include <QVariantMap>
#include <QAbstractItemModel>
#include <QPointer>
#include <QPersistentModelIndex>
#include <QJSValue>
#include <optional>
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
    Q_PROPERTY(bool stretchLastColumn READ stretchLastColumn WRITE setStretchLastColumn NOTIFY stretchLastColumnChanged)

    Q_PROPERTY(qtui::Color headerColor READ headerColor WRITE setHeaderColor NOTIFY headerColorChanged)
    Q_PROPERTY(qtui::Color headerBackgroundColor READ headerBackgroundColor WRITE setHeaderBackgroundColor NOTIFY headerBackgroundColorChanged)
    Q_PROPERTY(qtui::Color selectionColor READ selectionColor WRITE setSelectionColor NOTIFY selectionColorChanged)
    // Keep the model's own foreground and background on the selected row,
    // letting the selection show through the background alone. A table whose
    // cells carry meaning in their colour -- a diff, a heat map, a byte that
    // just changed -- otherwise loses exactly the row being inspected.
    Q_PROPERTY(bool selectionKeepsCellColors READ selectionKeepsCellColors
               WRITE setSelectionKeepsCellColors NOTIFY selectionKeepsCellColorsChanged)
    Q_PROPERTY(qtui::Color selectionBackgroundColor READ selectionBackgroundColor WRITE setSelectionBackgroundColor NOTIFY selectionBackgroundColorChanged)
    Q_PROPERTY(qtui::Color textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
    Q_PROPERTY(qtui::Color rowBackgroundColor READ rowBackgroundColor WRITE setRowBackgroundColor NOTIFY rowBackgroundColorChanged)
    Q_PROPERTY(qtui::Color alternatingRowBackgroundColor READ alternatingRowBackgroundColor WRITE setAlternatingRowBackgroundColor NOTIFY alternatingRowBackgroundColorChanged)
    Q_PROPERTY(qtui::Color gridColor READ gridColor WRITE setGridColor NOTIFY gridColorChanged)

    Q_PROPERTY(bool showColumnGrid READ showColumnGrid WRITE setShowColumnGrid NOTIFY showColumnGridChanged)
    Q_PROPERTY(bool showRowGrid READ showRowGrid WRITE setShowRowGrid NOTIFY showRowGridChanged)
    Q_PROPERTY(bool showOuterBorder READ showOuterBorder WRITE setShowOuterBorder NOTIFY showOuterBorderChanged)
    Q_PROPERTY(QString gridStyle READ gridStyle WRITE setGridStyle NOTIFY gridStyleChanged)

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

    bool stretchLastColumn() const { return m_stretchLastColumn; }
    void setStretchLastColumn(bool stretch);

    Q_INVOKABLE void forceLayout();

    Color headerColor() const { return m_headerColor; }
    void setHeaderColor(Color color);

    Color headerBackgroundColor() const { return m_headerBackgroundColor; }
    void setHeaderBackgroundColor(Color color);

    Color selectionColor() const { return m_selectionColor; }
    void setSelectionColor(Color color);
    bool selectionKeepsCellColors() const { return m_selectionKeepsCellColors; }
    void setSelectionKeepsCellColors(bool keep);

    Color selectionBackgroundColor() const { return m_selectionBackgroundColor; }
    void setSelectionBackgroundColor(Color color);

    Color textColor() const { return m_textColor; }
    void setTextColor(Color color);

    Color rowBackgroundColor() const { return m_rowBackgroundColor; }
    void setRowBackgroundColor(Color color);

    Color alternatingRowBackgroundColor() const { return m_alternatingRowBackgroundColor; }
    void setAlternatingRowBackgroundColor(Color color);

    Color gridColor() const { return m_gridColor; }
    void setGridColor(Color color);

    bool showColumnGrid() const { return m_showColumnGrid; }
    void setShowColumnGrid(bool show);

    bool showRowGrid() const { return m_showRowGrid; }
    void setShowRowGrid(bool show);
    bool showOuterBorder() const { return m_showOuterBorder; }
    void setShowOuterBorder(bool show);

    QString gridStyle() const { return m_gridStyle; }
    void setGridStyle(const QString &style);


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
    int implicitWidth() const override;

signals:
    void columnsChanged();
    void modelChanged();
    void selectedRowChanged();
    void currentColumnChanged();
    void alternatingRowsChanged();
    void stretchLastColumnChanged();

    void headerColorChanged();
    void headerBackgroundColorChanged();
    void selectionColorChanged();
    void selectionKeepsCellColorsChanged();
    void selectionBackgroundColorChanged();
    void textColorChanged();
    void rowBackgroundColorChanged();
    void alternatingRowBackgroundColorChanged();
    void gridColorChanged();

    void showColumnGridChanged();
    void showRowGridChanged();
    void showOuterBorderChanged();
    void gridStyleChanged();
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
    void paintOverlay(TextGridBuffer &buffer, int absX, int absY,
                      const ClipRect &clipRect) const override;

private:
    ClipRect clipRectForChildren(int absX, int absY,
                                const ClipRect &clipRect) const override;
    int rowCount() const;
    int columnCount() const;
    int headerRows() const;
    struct ViewGeometry {
        int left;
        int top;
        int width;
        int height;
        int bodyTop;
        int bodyHeight;
        int stride;
        int capacity;
        int right() const { return left + width; }
        int bottom() const { return top + height; }
    };
    ViewGeometry viewGeometry() const;
    int computeFirstVisibleRow() const;
    // Recovers m_firstVisibleRow from m_topIndex after a structural change.
    void synchronizeViewport();
    // Back to the top: a replaced or reset model makes the old offset
    // meaningless, which the derived version handled for free.
    void resetViewport();
    void layoutDelegates();
    void maybeFetchMore();
    using Position = std::pair<int, int>;
    struct DelegateRecord {
        QPointer<Item> item;
        QPointer<Item> container;
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
        int leftPadding = 0;
        int rightPadding = 0;
        QString headerAlignment;
        // Styling the column declares for itself. Resolved once here rather
        // than per cell, so a column with a fixed ink costs nothing to paint.
        // hasForeground/hasBackground distinguish "not set" from "set to the
        // value that happens to be 0".
        uint32_t foreground = 0;
        uint32_t background = 0;
        bool hasForeground = false;
        bool hasBackground = false;
        // A column whose colour depends on the row supplies a function
        // instead of a value. It is called per cell, so only the columns
        // that need it pay anything.
        QJSValue foregroundFn;
        QJSValue backgroundFn;
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
    void refreshModelRoles();
    // Calls a column's colour function for one cell and parses what it
    // returns. Empty when the function returns nothing or fails, which leaves
    // the cell to the model's role: a view can decide some rows and defer on
    // the rest.
    std::optional<uint32_t> columnColor(const QJSValue &callback, int row, int column,
                                        const QString &text) const;
    void refreshModelHeader();
    static QString alignmentName(const QVariant &alignment);
    void resolveColumns();
    int resolveColumnWidth(int column, const QVariantMap &descriptor);
    void updateImplicitWidth();
    void callbackWarning(const QString &message, QSet<QString> &errors) const;

    void clearDelegateItems();
    void generateDelegateItems();
    static void destroyDelegate(DelegateRecord &record);

    int m_calculatedImplicitWidth = 0;
    QVariantList m_columns;
    QVariant m_modelVariant;
    QVariantList m_listModel;
    QPointer<QAbstractItemModel> m_cppModel;

    int m_selectedRow = -1;
    int m_currentColumn = -1;
    int m_firstVisibleRow = 0;
    bool m_selectionKeepsCellColors = false;
    bool m_alternatingRows = false;
    bool m_stretchLastColumn = false;

    QPersistentModelIndex m_currentIndex;

    // Anchors the top visible row to a model row rather than an index, the
    // same way m_currentIndex anchors the selection. Rows removed from or
    // inserted above the viewport renumber everything below them, and Qt
    // shifts a persistent index across that for free; without it a scrolled
    // view walks forward through the data every time the model changes.
    QPersistentModelIndex m_topIndex;
    bool m_resetDelegates = false;
    bool m_structureDirty = false;
    bool m_geometryDirty = true;
    bool m_modelProvidesHeader = false;
    bool m_fetching = false;
    QHash<int, QByteArray> m_roleNames;
    // Role names as QStrings and the standard names the model claims, both
    // resolved when the model is set. updateDelegate() runs for every visible
    // cell of every frame, so neither is rebuilt there.
    QHash<int, QString> m_roleNameStrings;
    // A NumericItemRoles claim mask; see NumericItemRoles_p.h.
    quint16 m_claimedNames = 0;
    QList<ColumnGeometry> m_geometry;
    QList<int> m_separators;
    mutable QSet<QString> m_widthErrors;
    // Colour strings a column function returned, already parsed. Callbacks
    // return a handful of distinct colours across thousands of cells.
    mutable QHash<QString, uint32_t> m_columnColorCache;
    mutable QSet<QString> m_columnColorErrors;
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

    Color m_gridColor;
    uint32_t m_resolvedGridColor = 0;

    bool m_showColumnGrid = false;
    bool m_showRowGrid = false;
    bool m_showOuterBorder = false;
    QString m_gridStyle = QStringLiteral("single");

    QJSValue m_columnWidthProvider;
    QJSValue m_cellFormat;
    QPointer<QQmlComponent> m_delegate;
    QPointer<QQmlComponent> m_rowDelegate;
    QHash<Position, DelegateRecord> m_createdDelegateItems;
};

} // namespace qtui
