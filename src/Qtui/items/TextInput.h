#pragma once

#include "Item.h"
#include "TextEditorState.h"
#include <QRegularExpression>
#include <QString>

namespace qtui {

class TextInput : public Item {
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
    Q_PROPERTY(int maximumLength READ maximumLength WRITE setMaximumLength NOTIFY maximumLengthChanged)
    Q_PROPERTY(QRegularExpression validator READ validator WRITE setValidator NOTIFY validatorChanged)
    Q_PROPERTY(bool acceptableInput READ acceptableInput NOTIFY acceptableInputChanged)
    Q_PROPERTY(EchoMode echoMode READ echoMode WRITE setEchoMode NOTIFY echoModeChanged)
    Q_PROPERTY(QString echoCharacter READ echoCharacter WRITE setEchoCharacter NOTIFY echoCharacterChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY canUndoChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY canRedoChanged)

public:
    enum EchoMode {
        Normal,
        Password,
        NoEcho
    };
    Q_ENUM(EchoMode)

    explicit TextInput(QObject *parent = nullptr);
    ~TextInput() override;

    QString text() const { return m_editor.text(); }
    void setText(const QString& text);

    QString color() const { return m_color; }
    void setColor(const QString& color);
    QString placeholderText() const { return m_placeholderText; }
    void setPlaceholderText(const QString &text);

    int cursorPosition() const { return m_editor.cursorPosition(); }
    void setCursorPosition(int pos);
    int selectionStart() const { return m_editor.selectionStart(); }
    int selectionEnd() const { return m_editor.selectionEnd(); }
    QString selectedText() const { return m_editor.selectedText(); }
    bool readOnly() const { return m_readOnly; }
    void setReadOnly(bool readOnly);
    int contentX() const { return m_contentX; }
    void setContentX(int x);
    int maximumLength() const { return m_maximumLength; }
    void setMaximumLength(int length);
    QRegularExpression validator() const { return m_validator; }
    void setValidator(const QRegularExpression &validator);
    bool acceptableInput() const;
    EchoMode echoMode() const { return m_echoMode; }
    void setEchoMode(EchoMode mode);
    QString echoCharacter() const { return m_echoCharacter; }
    void setEchoCharacter(const QString &character);
    bool canUndo() const { return m_editor.canUndo(); }
    bool canRedo() const { return m_editor.canRedo(); }

    Q_INVOKABLE void select(int start, int end);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void copy();
    Q_INVOKABLE void cut();
    Q_INVOKABLE void paste(const QString &text);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    void keyPressEvent(class KeyEvent* event) override;
    void mousePressEvent(class MouseEvent *event) override;
    void mouseMoveEvent(class MouseEvent *event) override;
    void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) override;

signals:
    void textChanged();
    void colorChanged();
    void placeholderTextChanged();
    void cursorPositionChanged();
    void selectionChanged();
    void readOnlyChanged();
    void contentXChanged();
    void maximumLengthChanged();
    void validatorChanged();
    void acceptableInputChanged();
    void echoModeChanged();
    void echoCharacterChanged();
    void canUndoChanged();
    void canRedoChanged();
    void accepted();
    void editingFinished();

private:
    struct EditorStatus {
        bool acceptable = true;
        bool canUndo = false;
        bool canRedo = false;
    };

    EditorStatus editorStatus() const;
    void applyEditorChange(const TextEditorState::Change &change, const EditorStatus &before);
    QString constrainedReplacement(const QString &replacement) const;
    QString displayText() const;
    int displayColumnForPosition(int position) const;
    int positionForDisplayColumn(int column) const;
    int positionAtMouse(int x) const;
    void ensureCursorVisible();
    void updateImplicitWidth();

    TextEditorState m_editor;
    QString m_color = "white";
    QString m_placeholderText;
    QString m_echoCharacter = "*";
    QRegularExpression m_validator;
    uint32_t m_resolvedColor = 0xFFFFFFFF;
    int m_contentX = 0;
    int m_maximumLength = 0;
    bool m_readOnly = false;
    EchoMode m_echoMode = Normal;
};

} // namespace qtui
