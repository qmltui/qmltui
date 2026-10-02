#pragma once

#include "../core/TextGridBuffer.h"
#include <QObject>
#include <QQmlEngine>
#include <vector>
#include <QMap>
#include <QPair>
#include <QPointer>
#include <QPoint>

#include "Anchors.h"
#include "Border.h"

namespace qtui {

class State;
class Transition;
class Animation;

struct ClipRect {
    int x = 0;
    int y = 0;
    int width = 99999;
    int height = 99999;

    bool contains(int cx, int cy) const {
        return cx >= x && cx < x + width && cy >= y && cy < y + height;
    }

    ClipRect intersected(const ClipRect& other) const {
        int nx = std::max(x, other.x);
        int ny = std::max(y, other.y);
        int nrx = std::min(x + width, other.x + other.width);
        int nby = std::min(y + height, other.y + other.height);
        return ClipRect{nx, ny, std::max(0, nrx - nx), std::max(0, nby - ny)};
    }
};

class Keys;
class Terminal;
class AnchorLine;

class Item : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
  Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)
  Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
  Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)
  Q_PROPERTY(int implicitWidth READ implicitWidth WRITE setImplicitWidth NOTIFY
                 implicitWidthChanged)
  Q_PROPERTY(int implicitHeight READ implicitHeight WRITE setImplicitHeight
                 NOTIFY implicitHeightChanged)
  Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
  Q_PROPERTY(bool focus READ focus WRITE setFocus NOTIFY focusChanged)
  Q_PROPERTY(bool activeFocus READ activeFocus NOTIFY activeFocusChanged)
  Q_PROPERTY(
      bool focusable READ focusable WRITE setFocusable NOTIFY focusableChanged)
  Q_PROPERTY(qtui::Item *parent READ parentItem CONSTANT)
  Q_PROPERTY(qtui::Anchors *anchors READ anchors CONSTANT)
  Q_PROPERTY(qtui::AnchorLine* left READ left CONSTANT)
  Q_PROPERTY(qtui::AnchorLine* right READ right CONSTANT)
  Q_PROPERTY(qtui::AnchorLine* top READ top CONSTANT)
  Q_PROPERTY(qtui::AnchorLine* bottom READ bottom CONSTANT)
  Q_PROPERTY(qtui::AnchorLine* horizontalCenter READ horizontalCenter CONSTANT)
  Q_PROPERTY(qtui::AnchorLine* verticalCenter READ verticalCenter CONSTANT)
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
  Q_PROPERTY(double flexGrow READ flexGrow WRITE setFlexGrow NOTIFY flexGrowChanged)
  Q_PROPERTY(double flexShrink READ flexShrink WRITE setFlexShrink NOTIFY flexShrinkChanged)
  Q_PROPERTY(int flexBasis READ flexBasis WRITE setFlexBasis NOTIFY flexBasisChanged)
  Q_PROPERTY(QQmlListProperty<QObject> data READ data)
  Q_PROPERTY(QQmlListProperty<qtui::State> states READ states)
  Q_PROPERTY(QQmlListProperty<qtui::Transition> transitions READ transitions)
  Q_PROPERTY(QString state READ state WRITE setState NOTIFY stateChanged)
  Q_CLASSINFO("DefaultProperty", "data")

public:
  explicit Item(QObject *parent = nullptr);
  virtual ~Item();

  QQmlListProperty<QObject> data();
  QQmlListProperty<qtui::State> states();
  QQmlListProperty<qtui::Transition> transitions();
  QString state() const { return m_stateName; }
  void setState(const QString& newState);
  void evaluateStateConditions();
  const QList<qtui::Item *> &childItems() const { return m_visualChildren; }

  int x() const { return m_x; }
  void setX(int x);

  int y() const { return m_y; }
  void setY(int y);

  virtual int width() const;
  void setWidth(int width);

  virtual int height() const;
  void setHeight(int height);

  virtual int implicitWidth() const { return m_implicitWidth; }
  void setImplicitWidth(int w);

  virtual int implicitHeight() const { return m_implicitHeight; }
  void setImplicitHeight(int h);

  bool visible() const { return m_visible; }
  void setVisible(bool visible);

  bool focus() const { return m_focus; }
  void setFocus(bool focus);

  bool activeFocus() const { return m_activeFocus; }
  void setActiveFocus(bool active);

  bool focusable() const { return m_focusable; }
  void setFocusable(bool focusable);

  Terminal *terminal() const;
  QPoint absolutePosition() const;
  void setGeometryDirect(int x, int y, int w, int h);

  Item *parentItem() const { return qobject_cast<Item *>(QObject::parent()); }
  void setParentItem(Item *parentItem);
  void removeChildItem(Item *child);
  Anchors *anchors() { return &m_anchors; }
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

  double flexGrow() const { return m_flexGrow; }
  void setFlexGrow(double grow);

  double flexShrink() const { return m_flexShrink; }
  void setFlexShrink(double shrink);

  int flexBasis() const { return m_flexBasis; }
  void setFlexBasis(int basis);

  virtual void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect = ClipRect());
  virtual void keyPressEvent(class KeyEvent* event);
  virtual void mousePressEvent(class MouseEvent* event);
  virtual void mouseReleaseEvent(class MouseEvent* event);
  void markDirty();

signals:
  void xChanged();
  void yChanged();
  void widthChanged();
  void heightChanged();
  void implicitWidthChanged();
  void implicitHeightChanged();
  void visibleChanged();
  void focusChanged();
  void activeFocusChanged();
  void focusableChanged();
  void paddingChanged();
  void leftPaddingChanged();
  void rightPaddingChanged();
  void topPaddingChanged();
  void bottomPaddingChanged();
  void flexGrowChanged();
  void flexShrinkChanged();
  void flexBasisChanged();
  void clipChanged();
  void stateChanged();
  void dirty(); // For bubbling up render requests

private:
  static void appendData(QQmlListProperty<QObject> *list, QObject *item);
  static qsizetype dataCount(QQmlListProperty<QObject> *list);
  static QObject *dataAt(QQmlListProperty<QObject> *list, qsizetype index);
  static void clearData(QQmlListProperty<QObject> *list);

  static void appendState(QQmlListProperty<qtui::State>* list, qtui::State* state);
  static qsizetype stateCount(QQmlListProperty<qtui::State>* list);
  static qtui::State* stateAt(QQmlListProperty<qtui::State>* list, qsizetype index);
  static void clearStates(QQmlListProperty<qtui::State>* list);

  static void appendTransition(QQmlListProperty<qtui::Transition>* list, qtui::Transition* transition);
  static qsizetype transitionCount(QQmlListProperty<qtui::Transition>* list);
  static qtui::Transition* transitionAt(QQmlListProperty<qtui::Transition>* list, qsizetype index);
  static void clearTransitions(QQmlListProperty<qtui::Transition>* list);

protected:
  void childEvent(QChildEvent* event) override;

  int m_x = 0;
  int m_y = 0;
  int m_width = -1;
  int m_height = -1;
  int m_implicitWidth = 0;
  int m_implicitHeight = 0;
  bool m_visible = true;
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
  double m_flexGrow = 0.0;
  double m_flexShrink = 1.0;
  int m_flexBasis = -1; // -1 represents 'auto'
  QList<QObject*> m_dataChildren;
  QList<Item *> m_visualChildren;

  QString m_stateName;
  QList<qtui::State*> m_statesList;
  QList<qtui::Transition*> m_transitionsList;
  QMap<QPair<QPointer<QObject>, QString>, QVariant> m_savedPropertyValues;
  QList<QPointer<Animation>> m_runningTransitionAnimations;
};

} // namespace qtui
