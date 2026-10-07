#pragma once

#include <QtuiExport.h>

#include "Item.h"
#include "TextEditorState.h"
#include <QString>
#include "../primitives/ColorMapper.h"

namespace qtui {

class QTUI_CORE_EXPORT TextArea : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(qtui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY placeholderTextChanged)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorPositionChanged)
    Q_PROPERTY(int selectionStart READ selectionStart NOTIFY selectionChanged)
    Q_PROPERTY(int selectionEnd READ selectionEnd NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedText READ selectedText NOTIFY selectionChanged)
    Q_PROPERTY(bool readOnly READ readOnly WRITE setReadOnly NOTIFY readOnlyChanged)
    Q_PROPERTY(int contentX READ contentX WRITE setContentX NOTIFY contentXChanged)
    Q_PROPERTY(int contentY READ contentY WRITE setContentY NOTIFY contentYChanged)

public:
    explicit TextArea(QObject *parent = nullptr);
    ~TextArea() override;

    QString text() const { return m_editor.text(); }
    void setText(const QString& text);
    Color color() const { return m_color; }
    void setColor(Color color);
    QString placeholderText() const { return m_placeholderText; }
    void setPlaceholderText(const QString& text);
    int cursorPosition() const { return m_editor.cursorPosition(); }
    void setCursorPosition(int pos);
    int selectionStart() const;
    int selectionEnd() const;
    QString selectedText() const;
    bool readOnly() const { return m_readOnly; }
    void setReadOnly(bool readOnly);
    int contentX() const { return m_contentX; }
    void setContentX(int x);
    int contentY() const { return m_contentY; }
    void setContentY(int y);

    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void copy();
    Q_INVOKABLE void cut();
    Q_INVOKABLE void paste(const QString &text);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    void keyPressEvent(class KeyEvent* event) override;
    void mousePressEvent(class MouseEvent* event) override;
    void mouseMoveEvent(class MouseEvent* event) override;
    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) const override;

signals:
    void textChanged();
    void colorChanged();
    void placeholderTextChanged();
    void cursorPositionChanged();
    void selectionChanged();
    void readOnlyChanged();
    void contentXChanged();
    void contentYChanged();
    void accepted();

private:
    void applyEditorChange(const TextEditorState::Change &change);
    void lineColumnForPosition(int position, int &line, int &column) const;
    int positionForLineColumn(int line, int column) const;
    void moveCursor(int position, bool extendSelection);
    void ensureCursorVisible();
    int lineStart(int position) const;
    int lineEnd(int position) const;
    int positionAtMouse(int x, int y) const;

    TextEditorState m_editor;
    // "default" is the terminal's foreground (alpha 0 -> SGR 39),
    // not invisible ink: a fixed "white" default is unreadable on a light theme.
    Color m_color = "default";
    QString m_placeholderText;
    // Resolved caches mirror the Color members: an ANSI-named default must not be
    // written as literal RGB, or the terminal theme stops applying to it (setters
    // early-return when the color is unchanged, so the initializer is the only chance).
    uint32_t m_resolvedColor = ColorMapper::fromString(m_color, 0xFFFFFFFF);
    int m_contentX = 0;
    int m_contentY = 0;
    bool m_readOnly = false;
};

} // namespace qtui
