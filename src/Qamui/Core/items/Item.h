#pragma once

#include <Qamui/Core/primitives/QamuiExport.h>

#include <Qamui/Core/primitives/TextGridBuffer.h>
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

#include <Qamui/Core/items/Anchors.h>
#include <Qamui/Core/items/Border.h>

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

  Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
  Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)
  Q_PROPERTY(qreal z READ z WRITE setZ NOTIFY zChanged)
  Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
  Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)
  Q_PROPERTY(int implicitWidth READ implicitWidth WRITE setImplicitWidth NOTIFY
                 implicitWidthChanged)
  Q_PROPERTY(int implicitHeight READ implicitHeight WRITE setImplicitHeight
                 NOTIFY implicitHeightChanged)
  Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
  Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
  Q_PROPERTY(bool focus READ focus WRITE setFocus NOTIFY focusChanged)
  Q_PROPERTY(bool activeFocus READ activeFocus NOTIFY activeFocusChanged)
  Q_PROPERTY(
      bool focusable READ focusable WRITE setFocusable NOTIFY focusableChanged)
  Q_PROPERTY(qamui::Item *parent READ parentItem WRITE setParentItem NOTIFY parentChanged)
  Q_PROPERTY(qamui::Anchors *anchors READ anchors CONSTANT)
  Q_PROPERTY(qamui::AnchorLine* left READ left CONSTANT)
  Q_PROPERTY(qamui::AnchorLine* right READ right CONSTANT)
  Q_PROPERTY(qamui::AnchorLine* top READ top CONSTANT)
  Q_PROPERTY(qamui::AnchorLine* bottom READ bottom CONSTANT)
  Q_PROPERTY(qamui::AnchorLine* horizontalCenter READ horizontalCenter CONSTANT)
  Q_PROPERTY(qamui::AnchorLine* verticalCenter READ verticalCenter CONSTANT)
  Q_PROPERTY(bool clip READ clip WRITE setClip NOTIFY clipChanged)
  Q_PROPERTY(int padding READ padding WRITE setPadding NOTIFY paddingChanged)
  Q_PROPERTY(int leftPadding READ leftPadding WRITE setLeftPadding NOTIFY
                 leftPaddingChanged)
  Q_PROPERTY(int rightPadding READ rightPadding WRITE setRightPadding NOTIFY
                 rightPaddingChanged)
  Q_PROPERTY(int topPadding READ topPadding WRITE setTopPadding NOTIFY
                 topPaddingChanged)
  Q_PROPERTY(int bottomPadding READ bottomPadding WRITE setBottomPadding NOTIFY
                 bottomPaddingChanged)
  Q_PROPERTY(QQmlListProperty<QObject> data READ data)
  Q_PROPERTY(QQmlListProperty<qamui::Item> children READ childrenProperty NOTIFY childrenChanged)
  Q_PROPERTY(QQmlListProperty<qamui::State> states READ states)
  Q_PROPERTY(QQmlListProperty<qamui::Transition> transitions READ transitions)
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

  virtual void updateLayout();
  virtual void paint(TextGridBuffer &buffer, int absX, int absY,
                     const ClipRect &clipRect) const;
  virtual void paintOverlay(TextGridBuffer &buffer, int absX, int absY,
                            const ClipRect &clipRect) const;
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
