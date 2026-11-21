#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include <QObject>
#include <QQmlEngine>
#include <QPointer>
#include <QPoint>
#include <QRect>
#include <QMetaObject>
#include <algorithm>
#include <memory>

#include <QmlTui/Core/Items/Anchors.h>

namespace qmltui {

class State;
class Transition;
class StateController;
class ItemPaintTraversal;
struct ItemPrivate;

// Axis-aligned bounding rectangle in absolute terminal cell coordinates.
struct ClipRect {
    int x = 0;
    int y = 0;
    int width = 99999;
    int height = 99999;

    bool isEmpty() const { return width <= 0 || height <= 0; }

    bool contains(int cx, int cy) const {
        return cx >= x && cx < x + width && cy >= y && cy < y + height;
    }

    ClipRect intersected(const ClipRect& other) const {
        int nx = (std::max)(x, other.x);
        int ny = (std::max)(y, other.y);
        int nrx = (std::min)(x + width, other.x + other.width);
        int nby = (std::min)(y + height, other.y + other.height);
        return ClipRect{nx, ny, (std::max)(0, nrx - nx), (std::max)(0, nby - ny)};
    }
};

class Terminal;
class FocusScope;
class AnchorLine;

// Base visual and layout element for all QmlTui terminal items.
//
// Provides integer cell geometry (x, y, width, height), stacking order (z),
// keyboard focus management (focus, activeFocus, focusable), declarative states
// and transitions, anchors, and the virtual rendering contract (updateLayout,
// paint, paintOverlay). Main/engine thread only.
class QMLTUI_CORE_EXPORT Item : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Item)
    QML_ELEMENT

public:
    enum class DirtyFlag : quint8 {
        None = 0,
        SelfDirty = 1 << 0,
        DescendantDirty = 1 << 1,
        LayoutDirty = 1 << 2,
        DescendantLayoutDirty = 1 << 4,
    };
    Q_DECLARE_FLAGS(DirtyFlags, DirtyFlag)

    Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(qreal z READ z WRITE setZ NOTIFY zChanged)
    // Defaults to implicitWidth / implicitHeight until explicitly assigned.
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)
    Q_PROPERTY(
        int implicitWidth READ implicitWidth WRITE setImplicitWidth NOTIFY implicitWidthChanged)
    Q_PROPERTY(
        int implicitHeight READ implicitHeight WRITE setImplicitHeight NOTIFY implicitHeightChanged)
    // Invisible items do not paint or receive keyboard/mouse input.
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    // Disabled items remain visible but ignore keyboard and mouse input.
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    // Requests keyboard focus; activeFocus reflects whether this item actually holds it.
    Q_PROPERTY(bool focus READ focus WRITE setFocus NOTIFY focusChanged)
    Q_PROPERTY(bool activeFocus READ activeFocus NOTIFY activeFocusChanged)
    // How this item accepts keyboard/mouse focus (Tab, Click, Strong, None).
    Q_PROPERTY(
        Qt::FocusPolicy focusPolicy READ focusPolicy WRITE setFocusPolicy NOTIFY focusPolicyChanged)
    Q_PROPERTY(qmltui::Item* parent READ parentItem WRITE setParentItem NOTIFY parentChanged)
    Q_PROPERTY(qmltui::Anchors* anchors READ anchors CONSTANT)
    Q_PROPERTY(qmltui::AnchorLine* left READ left CONSTANT)
    Q_PROPERTY(qmltui::AnchorLine* right READ right CONSTANT)
    Q_PROPERTY(qmltui::AnchorLine* top READ top CONSTANT)
    Q_PROPERTY(qmltui::AnchorLine* bottom READ bottom CONSTANT)
    Q_PROPERTY(qmltui::AnchorLine* horizontalCenter READ horizontalCenter CONSTANT)
    Q_PROPERTY(qmltui::AnchorLine* verticalCenter READ verticalCenter CONSTANT)
    Q_PROPERTY(bool clip READ clip WRITE setClip NOTIFY clipChanged)
    // Uniform inner padding in cells; side-specific padding overrides when >= 0.
    Q_PROPERTY(int padding READ padding WRITE setPadding NOTIFY paddingChanged)
    Q_PROPERTY(int leftPadding READ leftPadding WRITE setLeftPadding NOTIFY leftPaddingChanged)
    Q_PROPERTY(int rightPadding READ rightPadding WRITE setRightPadding NOTIFY rightPaddingChanged)
    Q_PROPERTY(int topPadding READ topPadding WRITE setTopPadding NOTIFY topPaddingChanged)
    Q_PROPERTY(
        int bottomPadding READ bottomPadding WRITE setBottomPadding NOTIFY bottomPaddingChanged)
    // Default property holding both visual child Items and non-visual QObject children.
    Q_PROPERTY(QQmlListProperty<QObject> data READ data)
    Q_PROPERTY(QQmlListProperty<qmltui::Item> children READ childrenProperty NOTIFY childrenChanged)
    Q_PROPERTY(QQmlListProperty<qmltui::State> states READ states)
    Q_PROPERTY(QQmlListProperty<qmltui::Transition> transitions READ transitions)
    Q_PROPERTY(QString state READ state WRITE setState NOTIFY stateChanged)
    Q_CLASSINFO("DefaultProperty", "data")

public:
    explicit Item(QObject* parent = nullptr);
    ~Item() override;

    QQmlListProperty<QObject> data() const;
    QQmlListProperty<qmltui::Item> childrenProperty() const;
    QQmlListProperty<qmltui::State> states() const;
    QQmlListProperty<qmltui::Transition> transitions() const;
    QString state() const;
    void setState(const QString& newState);
    void evaluateStateConditions();
    const QList<qmltui::Item*>& childItems() const;
    const QList<qmltui::Item*>& stackingChildren() const;

    int x() const;
    void setX(int x);

    int y() const;
    void setY(int y);
    qreal z() const;
    void setZ(qreal z);

    virtual int width() const;
    virtual void setWidth(int width);

    virtual int height() const;
    virtual void setHeight(int height);

    virtual int implicitWidth() const;
    void setImplicitWidth(int w);

    virtual int implicitHeight() const;
    void setImplicitHeight(int h);

    bool visible() const;
    virtual void setVisible(bool visible);

    bool enabled() const;
    void setEnabled(bool enabled);
    bool isEffectivelyEnabled() const;
    bool isEffectivelyVisible() const;

    bool focus() const;
    virtual void setFocus(bool focus);

    bool activeFocus() const;
    void setActiveFocus(bool active);

    Qt::FocusPolicy focusPolicy() const;
    void setFocusPolicy(Qt::FocusPolicy policy);

    Q_INVOKABLE void forceActiveFocus();

    virtual bool isFocusScope() const { return false; }
    FocusScope* enclosingFocusScope() const;

    Terminal* terminal() const;
    Q_INVOKABLE QPoint absolutePosition() const;
    virtual QPoint contentOffset() const;

    Item* parentItem() const;
    virtual void setParentItem(Item* parentItem);
    bool isDescendantOf(const Item* ancestor) const noexcept;
    Anchors* anchors() const;
    const Anchors* anchorsIfAny() const noexcept;
    AnchorLine* left() const;
    AnchorLine* right() const;
    AnchorLine* top() const;
    AnchorLine* bottom() const;
    AnchorLine* horizontalCenter() const;
    AnchorLine* verticalCenter() const;

    bool clip() const;
    void setClip(bool c);

    int padding() const;
    void setPadding(int p);

    virtual int leftPadding() const;
    void setLeftPadding(int p);

    virtual int rightPadding() const;
    void setRightPadding(int p);

    virtual int topPadding() const;
    void setTopPadding(int p);

    virtual int bottomPadding() const;
    void setBottomPadding(int p);

    virtual void keyPressEvent(class KeyEvent* event);
    virtual void keyReleaseEvent(class KeyEvent* event);
    // Returns true when unmodified printable keystrokes should be delivered to
    // this item instead of triggering single-key Shortcuts (e.g. editable
    // TextInput/TextArea or running PtyView).
    virtual bool consumesPrintableKeys() const { return false; }
    virtual void mousePressEvent(class MouseEvent* event);
    virtual void mouseReleaseEvent(class MouseEvent* event);
    virtual void mouseMoveEvent(class MouseEvent* event);
    virtual void wheelEvent(class MouseEvent* event);
    bool filtersChildMouseEvents() const;
    void setFiltersChildMouseEvents(bool filter);
    virtual bool childMouseEventFilter(Item* item, class MouseEvent* event);

    // Cheap type tests for the framework's own hot paths (per item per frame),
    // set by the Popup and Terminal constructors instead of using RTTI.
    bool isPopup() const;
    bool isTerminalRoot() const;
    bool hasExplicitX() const;
    bool hasExplicitY() const;
    bool hasExplicitWidth() const;
    bool hasExplicitHeight() const;
    bool ignoresParentPadding() const;
    void setDefaultX(int x);
    void setDefaultY(int y);

    DirtyFlags dirtyFlags() const;
    void markDirty(DirtyFlags flags);
    void markDirty() { markDirty(DirtyFlag::SelfDirty); }
    void clearDirty(DirtyFlags flags);
    void clearDirty() { clearDirty(DirtyFlags(DirtyFlag::SelfDirty) | DirtyFlag::DescendantDirty); }

signals:
    void childrenChanged();
    void xChanged();
    void yChanged();
    void zChanged();
    void widthChanged();
    void heightChanged();
    void implicitWidthChanged();
    void implicitHeightChanged();
    void visibleChanged();
    void parentChanged();
    // The item or an ancestor was attached to or detached from a parent, so the
    // resolved terminal() may have changed. Unlike parentChanged, an ancestor's
    // reparenting (a Repeater or Loader inserting finished content) reaches the
    // whole subtree. Emitted per item; connect on the item whose terminal-derived
    // state (hover registration, tool tips) needs refreshing.
    void ancestryChanged();
    void enabledChanged();
    void focusChanged();
    void activeFocusChanged();
    void focusPolicyChanged();
    void paddingChanged();
    void leftPaddingChanged();
    void rightPaddingChanged();
    void topPaddingChanged();
    void bottomPaddingChanged();
    void clipChanged();
    void stateChanged();
    void dirty(); // For bubbling up render requests

private:
    friend class ItemPaintTraversal;
    friend class Repeater;
    friend class FocusScope;

    StateController* ensureStateController() const;
    void clearFocusFlagAndNotify();
    void layoutSubtree();
    void paintSubtree(TextGridBuffer& buffer, int absX, int absY,
                      const ClipRect& clipRect = ClipRect());
    QRect localSelfBounds() const;
    const QRect& ensureSubtreeBounds() const;
    void clearPaintDirtySubtree();
    void damageLastPaintedSubtree();
    void collectDirtyDamage(int absX, int absY, const QRect& ancestorClip, QRect& outDamage,
                            bool& outFullDamage) const;
    void invalidateStackingOrder();
    void scheduleChildGeometryInvalidation();
    void registerChildItem(Item* childItem);
    void unregisterChildItem(QObject* childObject);
    // Emits ancestryChanged() on this item and every visual descendant.
    void notifySubtreeAncestryChanged();
    void reorderChildItemsAfter(Item* sibling, const QList<Item*>& items);
    static void appendData(QQmlListProperty<QObject>* list, QObject* item);
    static qsizetype dataCount(QQmlListProperty<QObject>* list);
    static QObject* dataAt(QQmlListProperty<QObject>* list, qsizetype index);
    static void clearData(QQmlListProperty<QObject>* list);

protected:
    // `focus: true` set while a subtree is still detached from its Terminal is
    // recorded on the item; these apply it once the subtree is attached.
    Item* findFocusRequest() const;
    void applyPendingFocus();
    enum class ItemFlag : quint8 {
        IsPopup = 1 << 0,
        IsTerminalRoot = 1 << 1,
        ExplicitX = 1 << 2,
        ExplicitY = 1 << 3,
        IgnoresParentPadding = 1 << 4,
        InDestructor = 1 << 5,
        ScrollbarOverhang = 1 << 6,
        BoundedSelfPaint = 1 << 7,
    };
    void setItemFlag(ItemFlag flag, bool on = true);
    void invalidateSubtreeBounds();

    void childEvent(QChildEvent* event) override;

    // Emits implicitWidthChanged()/implicitHeightChanged() only when the
    // effective implicit size differs from the last reported value. Containers
    // whose implicit size derives from their children call this instead of
    // emitting unconditionally.
    void notifyImplicitSizeIfChanged();

    // Recomputes geometry or child item arrangements during the layout pass
    // before rendering when LayoutDirty is set. Container subclasses (Row,
    // Column, GridLayout, views) override this to position children.
    virtual void updateLayout();

    // Paints the item into the terminal text grid buffer during the frame commit
    // pass.
    //
    // Subclassing contract:
    // - Coordinate system: (absX, absY) is this item's top-left origin in
    //   0-indexed absolute buffer columns and rows.
    // - Constness: paint() must not mutate item state, recompute layouts, or
    //   emit signals. Compute caches in property setters or updateLayout().
    // - Clipping: all writes must stay within clipRect.
    // - Performance: avoid heap allocations, string formatting, or Unicode width
    //   scans on the paint path.
    virtual void paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const;

    // Paints an overlay pass after the standard paint() traversal of all items
    // has completed (used for focus rings, scrollbars, tooltips, and popups).
    // Follows the same const, clipping, and performance rules as paint().
    virtual void paintOverlay(TextGridBuffer& buffer, int absX, int absY,
                              const ClipRect& clipRect) const;

    // Clipping rectangle applied to this item's children: intersects clipRect
    // with this item's bounds when clip() is true, or returns clipRect unchanged.
    virtual ClipRect clipRectForChildren(int absX, int absY, const ClipRect& clipRect) const;

private:
    std::unique_ptr<ItemPrivate> m_d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Item::DirtyFlags)

} // namespace qmltui
