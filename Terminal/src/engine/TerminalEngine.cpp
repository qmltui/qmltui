#include "TerminalEngine.h"
#include "InputDecoder.h"
#include "Renderer.h"
#include "FrameRenderer.h"
#include "../items/Item.h"
#include "../items/Terminal.h"
#include "../items/Popup.h"
#include "../items/MouseArea.h"
#include "../items/ScrollView.h"
#include "KeyEvent.h"
#include "MouseEvent.h"
#include "Keys.h"
#include <QCoreApplication>
#include <QMetaObject>
#include <QSet>
#include <QThread>
#include <QtGlobal>
#include <iostream>

#ifdef Q_OS_UNIX
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#endif

namespace qtui {

static bool isDescendantOf(Item* child, Item* ancestor) {
    QObject* p = child;
    while (p) {
        if (p == ancestor) return true;
        p = p->parent();
    }
    return false;
}

#ifdef Q_OS_UNIX
class TerminalResizeBridge : public QObject {
    Q_OBJECT
public:
    static TerminalResizeBridge* instance() {
        if (!qApp) return nullptr;
        // Application-owned static object
        static TerminalResizeBridge* bridge = new TerminalResizeBridge(qApp);
        return bridge;
    }

    void registerEngine(TerminalEngine* engine) {
        // Ensure this is called on the application thread
        if (!qApp || QThread::currentThread() != qApp->thread()) {
            qWarning("TerminalEngine must be created on the QCoreApplication thread.");
            return;
        }

        if (!m_engines.contains(engine)) {
            m_engines.insert(engine);
            if (m_engines.size() == 1) {
                setup();
            }
        }
    }

    void unregisterEngine(TerminalEngine* engine) {
        if (!qApp || QThread::currentThread() != qApp->thread()) return;
        m_engines.remove(engine);
    }

signals:
    void sigwinchReceived();

private slots:
    void handleSocketNotification() {
        char buf[128];
        ssize_t bytesRead;
        while ((bytesRead = ::read(sigwinchFd[0], buf, sizeof(buf))) > 0) {
            // Drain the pipe completely
        }
        emit sigwinchReceived();
    }

private:
    explicit TerminalResizeBridge(QObject* parent = nullptr) : QObject(parent) {}
    
    void setup() {
        if (m_installed) return; // Retained for process lifetime

        if (::pipe(sigwinchFd) != 0) {
            sigwinchFd[0] = -1;
            sigwinchFd[1] = -1;
            return;
        }

        for (int i = 0; i < 2; ++i) {
            int flags = ::fcntl(sigwinchFd[i], F_GETFD);
            if (flags != -1) {
                ::fcntl(sigwinchFd[i], F_SETFD, flags | FD_CLOEXEC);
            }
            flags = ::fcntl(sigwinchFd[i], F_GETFL);
            if (flags != -1) {
                ::fcntl(sigwinchFd[i], F_SETFL, flags | O_NONBLOCK);
            }
        }

        struct sigaction sa;
        sa.sa_handler = TerminalResizeBridge::sigwinchHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        
        if (::sigaction(SIGWINCH, &sa, &m_oldAction) == 0) {
            m_installed = true;
            m_notifier = new QSocketNotifier(sigwinchFd[0], QSocketNotifier::Read, this);
            connect(m_notifier, &QSocketNotifier::activated, this, &TerminalResizeBridge::handleSocketNotification);
        } else {
            ::close(sigwinchFd[0]);
            ::close(sigwinchFd[1]);
            sigwinchFd[0] = -1;
            sigwinchFd[1] = -1;
        }
    }

    static void sigwinchHandler(int) {
        int savedErrno = errno;
        if (sigwinchFd[1] != -1) {
            char c = 1;
            ssize_t res = ::write(sigwinchFd[1], &c, 1);
            (void)res;
        }
        errno = savedErrno;
    }

    static int sigwinchFd[2];
    QSet<TerminalEngine*> m_engines;
    QSocketNotifier* m_notifier = nullptr;
    struct sigaction m_oldAction;
    bool m_installed = false;
};

int TerminalResizeBridge::sigwinchFd[2] = {-1, -1};
#endif


TerminalEngine::TerminalEngine(Terminal *rootWindow, QObject *parent)
    : QObject(parent), m_rootWindow(rootWindow) {

  // Setup InputDecoder callbacks
  m_inputDecoder.setKeyCallback([this](int key, const QString& text, int modifiers) {
      handleKeyInput(key, text, modifiers);
  });
  m_inputDecoder.setMouseCallback([this](int button, int x, int y, bool release) {
      handleMouseInput(button, x, y, release);
  });

  m_device.enableRawMode();
  m_device.hideCursor();

  m_renderer = new FrameRenderer();

  // Enable mouse events: press, release, drag, SGR 1006 format
  m_device.write("\033[?1000h\033[?1002h\033[?1006h");

  handleResize();

  m_notifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
  connect(m_notifier, &QSocketNotifier::activated, this,
          &TerminalEngine::readTerminalData);

#ifdef Q_OS_UNIX
  connect(TerminalResizeBridge::instance(), &TerminalResizeBridge::sigwinchReceived,
          this, &TerminalEngine::readResizeSignal);
  TerminalResizeBridge::instance()->registerEngine(this);
#endif

  requestRender();
}

TerminalEngine::~TerminalEngine() {
#ifdef Q_OS_UNIX
  TerminalResizeBridge::instance()->unregisterEngine(this);
#endif
  m_device.write("\033[?1000l\033[?1002l\033[?1006l"); // Disable mouse tracking
  if (m_renderer && m_rendererStarted) {
      m_renderer->stop(m_rootWindow, m_device);
  }
  delete m_renderer;
  m_renderer = nullptr;
  m_device.showCursor();
  m_device.disableRawMode();
}

void TerminalEngine::requestRender() {
  if (m_renderPending)
    return;
  m_renderPending = true;
  QMetaObject::invokeMethod(this, &TerminalEngine::renderFrame,
                            Qt::QueuedConnection);
}

void TerminalEngine::handleResize() {
  int w = m_device.getWidth();
  int h = m_device.getHeight();
  if (m_renderer) {
      m_renderer->resize(w, h);
  }
  if (m_rootWindow) {
    if (m_rootWindow->fillTerminal()) {
      emit m_rootWindow->widthChanged();
      emit m_rootWindow->heightChanged();
      m_rootWindow->markDirty();
    }
  }
}

void TerminalEngine::readResizeSignal() {
  handleResize();
  requestRender();
}

void TerminalEngine::readTerminalData() {
  char buffer[1024];
  ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer));
  if (n <= 0) return;

  m_inputDecoder.decode(buffer, n);
}

void TerminalEngine::handleKeyInput(int key, const QString& text, int modifiers) {
  if (!m_rootWindow) return;

  KeyEvent event(key, text, modifiers);

  const QList<Popup*>& popups = m_rootWindow->activePopups();
  Popup* activeModalPopup = nullptr;
  for (int i = popups.size() - 1; i >= 0; --i) {
      Popup* p = popups[i];
      if (p && p->visible() && p->modal()) {
          activeModalPopup = p;
          break;
      }
  }

  if (activeModalPopup) {
      if (key == Qt::Key_Escape && (activeModalPopup->closePolicy() & Popup::CloseOnEscape)) {
          activeModalPopup->close();
          return;
      }
      
      Item* focused = m_rootWindow->focusedItem();
      Item* current = (focused && isDescendantOf(focused, activeModalPopup)) ? focused : activeModalPopup;
      
      while (current) {
          current->keyPressEvent(&event);
          if (event.isAccepted()) break;

          Keys* attached = qobject_cast<Keys*>(qmlAttachedPropertiesObject<Keys>(current, false));
          if (attached) {
              emit attached->pressed(&event);
              if (event.isAccepted()) break;
          }
          
          if (current == activeModalPopup) break;
          current = qobject_cast<Item*>(current->parent());
      }
      return;
  }

  Item* focused = m_rootWindow->focusedItem();
  Item* current = focused ? focused : m_rootWindow;

  while (current) {
    current->keyPressEvent(&event);
    if (event.isAccepted()) break;

    Keys* attached = qobject_cast<Keys*>(qmlAttachedPropertiesObject<Keys>(current, false));
    if (attached) {
      emit attached->pressed(&event);
      if (event.isAccepted()) break;
    }
    current = qobject_cast<Item*>(current->parent());
  }

  if (!event.isAccepted()) {
    if (key == Qt::Key_Tab) {
      cycleFocus(true);
    } else if (key == Qt::Key_Backtab) {
      cycleFocus(false);
    }
  }
}

void TerminalEngine::handleMouseInput(int button, int x, int y, bool release) {
  if (!m_rootWindow) return;

  const QList<Popup*>& popups = m_rootWindow->activePopups();
  
  for (int i = popups.count() - 1; i >= 0; --i) {
      Popup* popup = popups[i];
      if (!popup || !popup->visible()) continue;
      
      QPoint absPos = popup->absolutePosition();
      bool inside = (x >= absPos.x() && x < absPos.x() + popup->width() &&
                     y >= absPos.y() && y < absPos.y() + popup->height());
                     
      if (inside) {
          int parentAbsX = absPos.x() - popup->x();
          int parentAbsY = absPos.y() - popup->y();
          Item* target = hitTestItem(popup, x, y, parentAbsX, parentAbsY);
          if (target) {
              int targetAbsX = target->absolutePosition().x();
              int targetAbsY = target->absolutePosition().y();
              int localX = x - targetAbsX;
              int localY = y - targetAbsY;

              MouseArea* ma = qobject_cast<MouseArea*>(target);
              if (ma) {
                  ma->setMousePosition(localX, localY);
              }
              MouseEvent event(localX, localY, button);

              Item* current = target;
              while (current) {
                  int currentAbsX = current->absolutePosition().x();
                  int currentAbsY = current->absolutePosition().y();
                  MouseEvent currentEvent(x - currentAbsX, y - currentAbsY, button);

                  if (release) {
                      current->mouseReleaseEvent(&currentEvent);
                  } else {
                      current->mousePressEvent(&currentEvent);
                  }
                  if (currentEvent.isAccepted()) break;

                  if (current == ma) {
                      if (release) {
                          emit ma->released(&currentEvent);
                          emit ma->clicked(&currentEvent);
                      } else {
                          emit ma->pressed(&currentEvent);
                      }
                      if (currentEvent.isAccepted()) break;
                  }
                  
                  if (current == popup) break;
                  current = qobject_cast<Item*>(current->parent());
              }
              return;
          }
      } else {
          if (popup->closePolicy() & Popup::CloseOnOutsideClick) {
              if (release) {
                  popup->close();
              }
          }
          if (popup->modal()) {
              return;
          }
      }
  }

  Item* target = hitTestItem(m_rootWindow, x, y, 0, 0);
  if (target) {
    int targetAbsX = target->absolutePosition().x();
    int targetAbsY = target->absolutePosition().y();

    int localX = x - targetAbsX;
    int localY = y - targetAbsY;

    MouseArea* ma = qobject_cast<MouseArea*>(target);
    if (ma) {
        ma->setMousePosition(localX, localY);
    }

    MouseEvent event(localX, localY, button);

    Item* current = target;
    while (current) {
      int currentAbsX = current->absolutePosition().x();
      int currentAbsY = current->absolutePosition().y();
      MouseEvent currentEvent(x - currentAbsX, y - currentAbsY, button);

      if (release) {
        current->mouseReleaseEvent(&currentEvent);
      } else {
        current->mousePressEvent(&currentEvent);
      }
      if (currentEvent.isAccepted()) break;

      if (current == ma) {
        if (release) {
          emit ma->released(&currentEvent);
          emit ma->clicked(&currentEvent);
        } else {
          emit ma->pressed(&currentEvent);
        }
        if (currentEvent.isAccepted()) break;
      }

      current = qobject_cast<Item*>(current->parent());
    }
  }
}

void TerminalEngine::cycleFocus(bool forward) {
  if (!m_rootWindow) return;

  QList<Item*> focusableList;
  m_rootWindow->findFocusableItems(m_rootWindow, focusableList);
  if (focusableList.isEmpty()) return;

  Item* currentFocus = m_rootWindow->focusedItem();
  int idx = focusableList.indexOf(currentFocus);

  Item* nextFocus = nullptr;
  if (idx == -1) {
    nextFocus = forward ? focusableList.first() : focusableList.last();
  } else {
    if (forward) {
      nextFocus = focusableList[(idx + 1) % focusableList.size()];
    } else {
      nextFocus = focusableList[(idx - 1 + focusableList.size()) % focusableList.size()];
    }
  }

  if (nextFocus) {
    nextFocus->setFocus(true);
  }
}

Item* TerminalEngine::hitTestItem(Item* item, int absX, int absY, int parentAbsX, int parentAbsY) const {
  if (!item || !item->visible()) return nullptr;

  int itemAbsX = parentAbsX + item->x();
  int itemAbsY = parentAbsY + item->y();

  bool inside = (absX >= itemAbsX && absX < itemAbsX + item->width() &&
                 absY >= itemAbsY && absY < itemAbsY + item->height());

  if (item->clip() && !inside) {
    return nullptr;
  }

  int childParentAbsX = itemAbsX;
  int childParentAbsY = itemAbsY;
  
  if (const ScrollView* sv = qobject_cast<const ScrollView*>(item)) {
    childParentAbsY -= sv->contentY();
  } else {
    QVariant scrollX = item->property("contentX");
    if (scrollX.isValid()) childParentAbsX -= scrollX.toInt();
    QVariant scrollY = item->property("contentY");
    if (scrollY.isValid()) childParentAbsY -= scrollY.toInt();
  }

  const auto& children = item->childItems();
  for (int i = children.size() - 1; i >= 0; --i) {
    Item* child = children[i];
    Item* hit = hitTestItem(child, absX, absY, childParentAbsX, childParentAbsY);
    if (hit) return hit;
  }

  if (inside) {
    return item;
  }

  return nullptr;
}

void TerminalEngine::renderFrame() {
  if (!m_rootWindow)
    return;

  m_renderPending = false;
  if (m_renderer) {
      startRenderer();
      m_renderer->renderFrame(m_rootWindow, m_device);
  }
}

void TerminalEngine::setRenderer(Renderer* renderer) {
    if (m_renderer == renderer) return;
    if (m_renderer) {
        if (m_rendererStarted) {
            m_renderer->stop(m_rootWindow, m_device);
        }
        delete m_renderer;
    }
    m_renderer = renderer;
    m_rendererStarted = false;
    if (m_renderer) {
        m_renderer->resize(m_device.getWidth(), m_device.getHeight());
        requestRender();
    }
}

void TerminalEngine::print(const QString& text) {
    if (m_renderer) {
        startRenderer();
        m_renderer->print(m_device, text.toStdString());
    }
}

void TerminalEngine::startRenderer() {
    if (!m_renderer || m_rendererStarted) return;
    m_renderer->start(m_rootWindow, m_device);
    m_rendererStarted = true;
}

void TerminalEngine::append(qtui::Item* item) {
    if (!item) return;
    int w = item->width() > 0 ? item->width() : m_device.getWidth();
    int h = item->height() > 0 ? item->height() : item->implicitHeight();
    if (h <= 0) h = 1;
    
    TextGridBuffer buffer;
    buffer.resize(w, h);
    item->render(buffer, 0, 0);
    
    print(QString::fromStdString(buffer.toAnsiString()) + "\n");
}

} // namespace qtui

#include "TerminalEngine.moc"
