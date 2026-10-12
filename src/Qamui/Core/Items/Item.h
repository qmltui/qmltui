#pragma once

#include <Qamui/Core/Primitives/QamuiExport.h>

#include <Qamui/Core/Primitives/TextGridBuffer.h>
#include <QObject>
#include <QQmlEngine>
#include <vector>
#include <utility>
#include <QMap>
#include <QPointer>
#include <QPoint>
#include <QHash>
#include <QMetaObject>
#include <algorithm>
#include <optional>

#include <Qamui/Core/Items/Anchors.h>
#include <Qamui/Core/Items/Border.h>

namespace qamui {

class State;
class Transition;
class StateController;
class Animation;
class ItemPaintTraversal;

struct ClipRect {
    int x = 0;
    int y = 0;
    int width = 99999;
    int height = 99999;

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

class Keys;
class Terminal;
class AnchorLine;

/**
 * @brief Base visual and layout item for all Qamui terminal elements.
 *
 * Equivalent to QQuickItem in Qt Quick. Provides integer cell geometry (x, y,
 * width, height), stacking order (z), keyboard focus management (focus,
 * activeFocus, focusable), declarative states and transitions, anchors,
 * and the virtual rendering contract (paint, paintOverlay).
 *
 * Developers subclass Item in C++ to implement custom terminal components.
 *
 * @thread_safety Main/engine thread only. Subclasses must run on the thread owning the Qamui engine.
 * @see Rectangle, Text, TableView, Terminal
 */
class QAMUI_CORE_EXPORT Item : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Item)
  QML_ELEMENT

public:
  enum class DirtyFlag : quint8 {
    None = 0,
    SelfDirty = 1 << 0,
    DescendantDirty = 1 << 1,
    LayoutDirty = 1 << 2,
  };
  Q_DECLARE_FLAGS(DirtyFlags, DirtyFlag)

  /// @brief Horizontal position relative to parent in terminal character columns.
  Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
  /// @brief Vertical position relative to parent in terminal character rows.
  Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)
  /// @brief Stacking order relative to siblings. Items with higher z render on top.
  Q_PROPERTY(qreal z READ z WRITE setZ NOTIFY zChanged)
  /// @brief Width of the item in integer terminal cell columns. Defaults to implicitWidth.
  Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
  /// @brief Height of the item in integer terminal cell rows. Defaults to implicitHeight.
  Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)
  /// @brief Natural content width in columns when width is unassigned.
  Q_PROPERTY(int implicitWidth READ implicitWidth WRITE setImplicitWidth NOTIFY
                 implicitWidthChanged)
  /// @brief Natural content height in rows when height is unassigned.
  Q_PROPERTY(int implicitHeight READ implicitHeight WRITE setImplicitHeight
                 NOTIFY implicitHeightChanged)
  /// @brief Whether the item is rendered. Invisible items do not receive focus or paint.
  Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
  /// @brief Whether the item is interactive. Disabled items ignore mouse and key input.
  Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
  /// @brief Whether this item requests keyboard focus within its focus scope.
  Q_PROPERTY(bool focus READ focus WRITE setFocus NOTIFY focusChanged)
  /// @brief Read-only. True if this item currently holds active keyboard input focus.
  Q_PROPERTY(bool activeFocus READ activeFocus NOTIFY activeFocusChanged)
  /// @brief Whether this item participates in Tab and Shift+Tab focus navigation cycling.
  Q_PROPERTY(
      bool focusable READ focusable WRITE setFocusable NOTIFY focusableChanged)
  /// @brief Visual parent item. Setting this updates the visual scene hierarchy.
  Q_PROPERTY(qamui::Item *parent READ parentItem WRITE setParentItem NOTIFY parentChanged)
  /// @brief Anchoring specification for alignment relative to sibling or parent edges.
  Q_PROPERTY(qamui::Anchors *anchors READ anchors CONSTANT)
  /// @brief Left edge anchor line.
  Q_PROPERTY(qamui::AnchorLine* left READ left CONSTANT)
  /// @brief Right edge anchor line.
  Q_PROPERTY(qamui::AnchorLine* right READ right CONSTANT)
  /// @brief Top edge anchor line.
  Q_PROPERTY(qamui::AnchorLine* top READ top CONSTANT)
  /// @brief Bottom edge anchor line.
  Q_PROPERTY(qamui::AnchorLine* bottom READ bottom CONSTANT)
  /// @brief Horizontal center anchor line.
  Q_PROPERTY(qamui::AnchorLine* horizontalCenter READ horizontalCenter CONSTANT)
  /// @brief Vertical center anchor line.
  Q_PROPERTY(qamui::AnchorLine* verticalCenter READ verticalCenter CONSTANT)
  /// @brief Whether visual children are clipped to this item's bounding box.
  Q_PROPERTY(bool clip READ clip WRITE setClip NOTIFY clipChanged)
  /// @brief Uniform padding in cells inside this item's boundary.
  Q_PROPERTY(int padding READ padding WRITE setPadding NOTIFY paddingChanged)
  /// @brief Left padding in cell columns.
  Q_PROPERTY(int leftPadding READ leftPadding WRITE setLeftPadding NOTIFY
                 leftPaddingChanged)
  /// @brief Right padding in cell columns.
  Q_PROPERTY(int rightPadding READ rightPadding WRITE setRightPadding NOTIFY
                 rightPaddingChanged)
  /// @brief Top padding in cell rows.
  Q_PROPERTY(int topPadding READ topPadding WRITE setTopPadding NOTIFY
                 topPaddingChanged)
  /// @brief Bottom padding in cell rows.
  Q_PROPERTY(int bottomPadding READ bottomPadding WRITE setBottomPadding NOTIFY
                 bottomPaddingChanged)
  /// @brief Default property containing visual children and non-visual data objects.
  Q_PROPERTY(QQmlListProperty<QObject> data READ data)
  /// @brief Visual child items of this item.
  Q_PROPERTY(QQmlListProperty<qamui::Item> children READ childrenProperty NOTIFY childrenChanged)
  /// @brief List of declarative states defined on this item.
  Q_PROPERTY(QQmlListProperty<qamui::State> states READ states)
  /// @brief List of declarative transitions between states.
  Q_PROPERTY(QQmlListProperty<qamui::Transition> transitions READ transitions)
  /// @brief Current state name of the item. Assigning transitions to that state.
  Q_PROPERTY(QString state READ state WRITE setState NOTIFY stateChanged)
  Q_CLASSINFO("DefaultProperty", "data")

public:
  explicit Item(QObject *parent = nullptr);
  virtual ~Item();

  QQmlListProperty<QObject> data() const;
  QQmlListProperty<qamui::Item> childrenProperty() const;
  QQmlListProperty<qamui::State> states() const;
  QQmlListProperty<qamui::Transition> transitions() const;
  QString state() const;
  void setState(const QString& newState);
  void evaluateStateConditions();
  const QList<qamui::Item *> &childItems() const { return m_visualChildren; }
  const QList<qamui::Item *> &stackingChildren() const;

  int x() const { return m_x; }
  void setX(int x);

  int y() const { return m_y; }
  void setY(int y);
  qreal z() const { return m_z; }
  void setZ(qreal z);

  virtual int width() const;
  virtual void setWidth(int width);

  virtual int height() const;
  virtual void setHeight(int height);

  virtual int implicitWidth() const { return m_implicitWidth; }
  void setImplicitWidth(int w);

  virtual int implicitHeight() const { return m_implicitHeight; }
  void setImplicitHeight(int h);

  bool visible() const { return m_visible; }
  virtual void setVisible(bool visible);

  bool enabled() const { return m_enabled; }
  void setEnabled(bool enabled);
  bool isEffectivelyEnabled() const;
  bool isEffectivelyVisible() const;

  bool focus() const { return m_focus; }
  void setFocus(bool focus);

  bool activeFocus() const { return m_activeFocus; }
  void setActiveFocus(bool active);

  bool focusable() const { return m_focusable; }
  void setFocusable(bool focusable);

  Terminal *terminal() const;
  QPoint absolutePosition() const;
  virtual QPoint contentOffset() const;

  Item *parentItem() const { return qobject_cast<Item *>(QObject::parent()); }
  virtual void setParentItem(Item *parentItem);
  Anchors *anchors() const { return const_cast<Anchors*>(&m_anchors); }
  AnchorLine *left() const { return m_leftLine; }
  AnchorLine *right() const { return m_rightLine; }
  AnchorLine *top() const { return m_topLine; }
  AnchorLine *bottom() const { return m_bottomLine; }
  AnchorLine *horizontalCenter() const { return m_horizontalCenterLine; }
  AnchorLine *verticalCenter() const { return m_verticalCenterLine; }

  bool clip() const { return m_clip; }
  void setClip(bool c);

  int padding() const { return m_padding; }
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
  virtual void mousePressEvent(class MouseEvent* event);
  virtual void mouseReleaseEvent(class MouseEvent* event);
  virtual void mouseMoveEvent(class MouseEvent* event);
  virtual void wheelEvent(class MouseEvent* event);
  bool filtersChildMouseEvents() const { return m_filtersChildMouseEvents; }
  void setFiltersChildMouseEvents(bool filter) { m_filtersChildMouseEvents = filter; }
  virtual bool childMouseEventFilter(Item* item, class MouseEvent* event);

  // Cheap type tests for the framework's own hot paths (per item per frame),
  // set by the Popup and Terminal constructors instead of using RTTI.
  bool isPopup() const { return m_itemFlags & static_cast<quint8>(ItemFlag::IsPopup); }
  bool isTerminalRoot() const { return m_itemFlags & static_cast<quint8>(ItemFlag::IsTerminalRoot); }

  DirtyFlags dirtyFlags() const { return m_dirtyFlags; }
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
  void focusableChanged();
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

  void layoutSubtree();
  void paintSubtree(TextGridBuffer &buffer, int absX, int absY,
                    const ClipRect &clipRect = ClipRect());
  void invalidateStackingOrder();
  void scheduleChildGeometryInvalidation();
  void registerChildItem(Item* childItem);
  void unregisterChildItem(Item* childItem);
  // Emits ancestryChanged() on this item and every visual descendant.
  void notifySubtreeAncestryChanged();
  void reorderChildItemsAfter(Item* sibling, const QList<Item*>& items);
  static void appendData(QQmlListProperty<QObject> *list, QObject *item);
  static qsizetype dataCount(QQmlListProperty<QObject> *list);
  static QObject *dataAt(QQmlListProperty<QObject> *list, qsizetype index);
  static void clearData(QQmlListProperty<QObject> *list);

protected:
  // `focus: true` set while a subtree is still detached from its Terminal is
  // recorded on the item; these apply it once the subtree is attached.
  Item* findFocusRequest() const;
  void applyPendingFocus();
  enum class ItemFlag : quint8 {
    IsPopup = 1 << 0,
    IsTerminalRoot = 1 << 1,
  };
  void setItemFlag(ItemFlag flag, bool on = true)
  {
    if (on)
      m_itemFlags |= static_cast<quint8>(flag);
    else
      m_itemFlags &= static_cast<quint8>(~static_cast<quint8>(flag));
  }

  void childEvent(QChildEvent* event) override;

  // Emits implicitWidthChanged()/implicitHeightChanged() only when the
  // effective implicit size differs from the last reported value. Containers
  // whose implicit size derives from their children call this instead of
  // emitting unconditionally.
  void notifyImplicitSizeIfChanged();

  /**
   * @brief Recomputes geometry or child item arrangements.
   *
   * Called during layout passes before rendering when LayoutDirty is set.
   * Container subclasses (Row, Column, Grid) override this to position children.
   */
  virtual void updateLayout();

  /**
   * @brief Paints the item into the terminal text grid buffer.
   *
   * Overridden by visual subclasses (such as Rectangle, Text, Canvas) to write
   * cells, glyphs, and styling into @p buffer during the frame commit phase.
   *
   * ### Subclassing Contract & Invariants
   * - **Coordinate System**: @p absX and @p absY represent the item's origin in
   *   absolute buffer grid columns and rows (0-indexed integer cell geometry).
   *   Local coordinate (0, 0) corresponds to (@p absX, @p absY).
   * - **Const Correctness**: This method is `const`. It must not mutate item state,
   *   recompute layouts, or emit signals. State changes must happen in property
   *   setters, which call update() to request a future paint pass.
   * - **Clipping**: Drawing must stay within @p clipRect. Writing outside this rect
   *   will corrupt adjacent terminal regions or cause rendering artifacts.
   *   Helper methods on @ref TextGridBuffer automatically clip to this boundary.
   * - **Performance**: Never perform heap allocations, string formatting, or
   *   Unicode width calculations inside paint(). Pre-compute string caches and
   *   glyph widths in property setters.
   *
   * @param buffer   Double-buffered text grid representing the terminal screen.
   * @param absX     Absolute horizontal grid column of this item's left edge.
   * @param absY     Absolute vertical grid row of this item's top edge.
   * @param clipRect Visible bounding box in absolute buffer coordinates.
   *
   * @thread_safety Main/engine thread only. Not thread-safe.
   * @see paintOverlay(), update(), TextGridBuffer
   */
  virtual void paint(TextGridBuffer &buffer, int absX, int absY,
                     const ClipRect &clipRect) const;

  /**
   * @brief Paints an overlay pass above all sibling items and child subtrees.
   *
   * Called after the standard paint() traversal of all items has completed.
   * Use this for elements that must visually float over neighboring items
   * without affecting layout (e.g., focus rings, tooltips, dropdown popups).
   *
   * Follows the same const, clipping, and performance rules as paint().
   *
   * @param buffer   Double-buffered text grid representing the terminal screen.
   * @param absX     Absolute horizontal grid column of this item's left edge.
   * @param absY     Absolute vertical grid row of this item's top edge.
   * @param clipRect Visible bounding box in absolute buffer coordinates.
   *
   * @see paint(), TextGridBuffer
   */
  virtual void paintOverlay(TextGridBuffer &buffer, int absX, int absY,
                            const ClipRect &clipRect) const;

  /**
   * @brief Computes the clipping rectangle applied to this item's children.
   *
   * If clip is true, returns the intersection of @p clipRect with this item's
   * bounding box. Otherwise, returns @p clipRect unchanged.
   */
  virtual ClipRect clipRectForChildren(int absX, int absY,
                                       const ClipRect &clipRect) const;

  int m_x = 0;
  int m_y = 0;
  qreal m_z = 0;
  int m_width = -1;
  int m_height = -1;
  int m_implicitWidth = 0;
  int m_implicitHeight = 0;
  int m_reportedImplicitWidth = 0;
  int m_reportedImplicitHeight = 0;
  bool m_visible = true;
  bool m_enabled = true;
  bool m_focus = false;
  bool m_activeFocus = false;
  bool m_focusable = false;
  Anchors m_anchors;
  AnchorLine* m_leftLine = nullptr;
  AnchorLine* m_rightLine = nullptr;
  AnchorLine* m_topLine = nullptr;
  AnchorLine* m_bottomLine = nullptr;
  AnchorLine* m_horizontalCenterLine = nullptr;
  AnchorLine* m_verticalCenterLine = nullptr;
  bool m_clip = false;
  int m_padding = 0;
  int m_leftPadding = -1;
  int m_rightPadding = -1;
  int m_topPadding = -1;
  int m_bottomPadding = -1;
  QList<QPointer<QObject>> m_dataChildren;
  QList<Item *> m_visualChildren;
  mutable QList<Item *> m_stackingChildren;
  mutable bool m_stackingOrderDirty = true;

  std::unique_ptr<StateController> m_stateController;
  QHash<Item*, QList<QMetaObject::Connection>> m_childGeometryConnections;
  bool m_filtersChildMouseEvents = false;
  quint8 m_itemFlags = 0;
  DirtyFlags m_dirtyFlags = DirtyFlag::SelfDirty;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Item::DirtyFlags)

} // namespace qamui
