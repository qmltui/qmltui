#pragma once

#include <Qamui/Core/items/Item.h>
#include <Qamui/Core/primitives/Color.h>

#include <QByteArray>
#include <QString>
#include <QtGlobal>

#include <deque>
#include <functional>
#include <vector>

#include <vterm.h>

namespace qamui {

class VTermAdapter final {
    Q_DISABLE_COPY_MOVE(VTermAdapter)
public:
  using BytesCallback = std::function<void(const QByteArray &)>;
  using StringCallback = std::function<void(const QString &)>;
  using VoidCallback = std::function<void()>;

  VTermAdapter(int rows, int columns);
  ~VTermAdapter();

  void setOutputCallback(BytesCallback callback) {
    m_outputCallback = std::move(callback);
  }
  void setDamageCallback(VoidCallback callback) {
    m_damageCallback = std::move(callback);
  }
  void setTitleCallback(StringCallback callback) {
    m_titleCallback = std::move(callback);
  }
  void setBellCallback(VoidCallback callback) {
    m_bellCallback = std::move(callback);
  }
  void setClipboardCallback(StringCallback callback) {
    m_clipboardCallback = std::move(callback);
  }
  void setNotificationCallback(StringCallback callback) {
    m_notificationCallback = std::move(callback);
  }
  void setMouseModeCallback(std::function<void(int)> callback) {
    m_mouseModeCallback = std::move(callback);
  }

  void feed(const QByteArray &bytes);
  void resize(int rows, int columns);
    void setDefaultColors(Color foreground, Color background);
    void setPaletteColor(int index, Color color);
    void setPalette(const QList<Color> &palette);
  void setScrollbackLimit(int limit);
  void clearScrollback();
  void reset();
  int scrollbackSize() const { return static_cast<int>(m_scrollback.size()); }
  bool mouseReporting() const { return m_mouseMode != VTERM_PROP_MOUSE_NONE; }

  void sendKey(int key, const QString &text, int modifiers);
  void sendPaste(const QString &text);
  void sendMouseMove(int row, int column, int modifiers);
  void sendMouseButton(int button, bool pressed, int modifiers);
  void sendWheel(int deltaY, int modifiers);
  void setFocused(bool focused);

  void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clip,
              int width, int height, int scrollOffset) const;

public: // C callback entry points; this class is private to TerminalHost.
  static int damage(VTermRect rect, void *user);
  static int moveRect(VTermRect dest, VTermRect src, void *user);
  static int moveCursor(VTermPos pos, VTermPos oldPos, int visible, void *user);
  static int setTermProperty(VTermProp prop, VTermValue *value, void *user);
  static int bell(void *user);
  static int resized(int rows, int columns, void *user);
  static int pushScrollback(int columns, const VTermScreenCell *cells,
                            void *user);
  static int popScrollback(int columns, VTermScreenCell *cells, void *user);
  static int clearScrollbackCallback(void *user);
  static int setSelection(VTermSelectionMask mask, VTermStringFragment fragment,
                          void *user);
  static int querySelection(VTermSelectionMask mask, void *user);
  static int unknownOsc(int command, VTermStringFragment fragment, void *user);
  static void outputBytes(const char *bytes, size_t length, void *user);

private:
  static VTermModifier toVTermModifiers(int modifiers);
  static VTermKey toVTermKey(int key);
  uint32_t colorToArgb(VTermColor color) const;
  VTermScreenCell screenCell(int row, int column) const;
  void appendFragment(QByteArray &target, VTermStringFragment fragment,
                      qsizetype limit, const StringCallback &callback);

  VTerm *m_vterm = nullptr;
  VTermState *m_state = nullptr;
  VTermScreen *m_screen = nullptr;
  int m_rows = 1;
  int m_columns = 1;
  int m_scrollbackLimit = 10000;
  int m_mouseMode = VTERM_PROP_MOUSE_NONE;
  bool m_altScreen = false;
  bool m_cursorVisible = true;
  VTermPos m_cursor{0, 0};
  uint32_t m_defaultForeground = 0xffffffffu;
  uint32_t m_defaultBackground = 0xff000000u;
    QList<Color> m_palette;
  std::deque<std::vector<VTermScreenCell>> m_scrollback;
  QByteArray m_selectionBuffer;
  QByteArray m_clipboardBuffer;
  QByteArray m_titleBuffer;
  QByteArray m_notificationBuffer;
  BytesCallback m_outputCallback;
  VoidCallback m_damageCallback;
  StringCallback m_titleCallback;
  VoidCallback m_bellCallback;
  StringCallback m_clipboardCallback;
  StringCallback m_notificationCallback;
  std::function<void(int)> m_mouseModeCallback;
};

} // namespace qamui
