#pragma once

#include "Item.h"
#include <QString>

namespace qtui {

class TextInput : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorPositionChanged)

public:
    explicit TextInput(QObject *parent = nullptr);
    ~TextInput() override;

    QString text() const { return m_text; }
    void setText(const QString& text);

    QString color() const { return m_color; }
    void setColor(const QString& color);

    int cursorPosition() const { return m_cursorPosition; }
    void setCursorPosition(int pos);

    void keyPressEvent(class KeyEvent* event) override;
    void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) override;

signals:
    void textChanged();
    void colorChanged();
    void cursorPositionChanged();
    void accepted();

private:
    QString m_text;
    QString m_color = "white";
    uint32_t m_resolvedColor = 0xFFFFFFFF;
    int m_cursorPosition = 0;
};

} // namespace qtui
