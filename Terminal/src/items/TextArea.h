#pragma once

#include "Item.h"
#include <QString>
#include <QVector>

namespace qtui {

class TextArea : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)
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

    QString text() const { return m_text; }
    void setText(const QString& text);
    QString color() const { return m_color; }
    void setColor(const QString& color);
    QString placeholderText() const { return m_placeholderText; }
    void setPlaceholderText(const QString& text);
    int cursorPosition() const { return m_cursorPosition; }
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
    void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) override;

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
    void lineColumnForPosition(int position, int &line, int &column) const;
    int positionForLineColumn(int line, int column) const;
    void moveCursor(int position, bool extendSelection);
    bool deleteSelection();
    void replaceSelection(const QString &replacement);
    void recordUndo();
    void ensureCursorVisible();
    int lineStart(int position) const;
    int lineEnd(int position) const;
    int positionAtMouse(int x, int y) const;

    QString m_text;
    QString m_color = "white";
    QString m_placeholderText;
    uint32_t m_resolvedColor = 0xFFFFFFFF;
    int m_cursorPosition = 0;
    int m_selectionAnchor = 0;
    int m_contentX = 0;
    int m_contentY = 0;
    bool m_readOnly = false;
    QVector<QString> m_undoStack;
    QVector<QString> m_redoStack;
};

} // namespace qtui
