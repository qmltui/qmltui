#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/TextEditorState.h>
#include <QRegularExpression>
#include <QString>
#include <cstdint>
namespace qmltui {
class ClockTimer;
}
#include <QmlTui/Core/Primitives/ColorMapper.h>

namespace qmltui {

class QMLTUI_CORE_EXPORT TextInput : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(qmltui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY
                   placeholderTextChanged)
    Q_PROPERTY(qmltui::Color placeholderColor READ placeholderColor WRITE setPlaceholderColor NOTIFY
                   placeholderColorChanged)
    Q_PROPERTY(QString displayText READ displayText NOTIFY textChanged)
    Q_PROPERTY(
        bool overwriteMode READ overwriteMode WRITE setOverwriteMode NOTIFY overwriteModeChanged)
    Q_PROPERTY(bool autoScroll READ autoScroll WRITE setAutoScroll NOTIFY autoScrollChanged)
    Q_PROPERTY(
        int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorPositionChanged)
    Q_PROPERTY(int selectionStart READ selectionStart NOTIFY selectionChanged)
    Q_PROPERTY(int selectionEnd READ selectionEnd NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedText READ selectedText NOTIFY selectionChanged)
    Q_PROPERTY(bool readOnly READ readOnly WRITE setReadOnly NOTIFY readOnlyChanged)
    Q_PROPERTY(int contentX READ contentX WRITE setContentX NOTIFY contentXChanged)
    Q_PROPERTY(
        int maximumLength READ maximumLength WRITE setMaximumLength NOTIFY maximumLengthChanged)
    Q_PROPERTY(
        QRegularExpression validator READ validator WRITE setValidator NOTIFY validatorChanged)
    Q_PROPERTY(bool acceptableInput READ acceptableInput NOTIFY acceptableInputChanged)
    Q_PROPERTY(EchoMode echoMode READ echoMode WRITE setEchoMode NOTIFY echoModeChanged)
    Q_PROPERTY(
        QString echoCharacter READ echoCharacter WRITE setEchoCharacter NOTIFY echoCharacterChanged)
    Q_PROPERTY(
        CursorShape cursorShape READ cursorShape WRITE setCursorShape NOTIFY cursorShapeChanged)
    Q_PROPERTY(
        qmltui::Color cursorColor READ cursorColor WRITE setCursorColor NOTIFY cursorColorChanged)
    Q_PROPERTY(bool cursorBlinking READ cursorBlinking WRITE setCursorBlinking NOTIFY
                   cursorBlinkingChanged)
    Q_PROPERTY(int cursorBlinkRate READ cursorBlinkRate WRITE setCursorBlinkRate NOTIFY
                   cursorBlinkRateChanged)
    Q_PROPERTY(
        bool cursorVisible READ cursorVisible WRITE setCursorVisible NOTIFY cursorVisibleChanged)
    Q_PROPERTY(QString cursorDelegate READ cursorDelegate WRITE setCursorDelegate NOTIFY
                   cursorDelegateChanged)
    Q_PROPERTY(qmltui::Color selectionColor READ selectionColor WRITE setSelectionColor NOTIFY
                   selectionColorChanged)
    Q_PROPERTY(qmltui::Color selectedTextColor READ selectedTextColor WRITE setSelectedTextColor
                   NOTIFY selectedTextColorChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY canUndoChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY canRedoChanged)

public:
    enum EchoMode : uint8_t {
        Normal,
        Password,
        NoEcho,
    };
    Q_ENUM(EchoMode)

    enum CursorShape : uint8_t {
        Block,
        Underline,
        Beam,
        HollowBlock,
    };
    Q_ENUM(CursorShape)

    explicit TextInput(QObject* parent = nullptr);
    ~TextInput() override;

    QString text() const { return m_editor.text(); }
    void setText(const QString& text);

    Color color() const { return m_color; }
    void setColor(Color color);
    QString placeholderText() const { return m_placeholderText; }
    void setPlaceholderText(const QString& text);

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
    void setValidator(const QRegularExpression& validator);
    bool acceptableInput() const;
    EchoMode echoMode() const { return m_echoMode; }
    void setEchoMode(EchoMode mode);
    QString echoCharacter() const { return m_echoCharacter; }
    void setEchoCharacter(const QString& character);
    CursorShape cursorShape() const { return m_cursorShape; }
    void setCursorShape(CursorShape shape);
    Color cursorColor() const { return m_cursorColor; }
    void setCursorColor(Color color);
    bool cursorBlinking() const { return m_cursorBlinking; }
    void setCursorBlinking(bool blinking);
    int cursorBlinkRate() const { return m_cursorBlinkRate; }
    void setCursorBlinkRate(int rate);
    bool cursorVisible() const { return m_cursorVisible; }
    void setCursorVisible(bool visible);
    QString cursorDelegate() const { return m_cursorDelegate; }
    void setCursorDelegate(const QString& delegate);
    Color selectionColor() const { return m_selectionColor; }
    void setSelectionColor(Color color);
    Color selectedTextColor() const { return m_selectedTextColor; }
    void setSelectedTextColor(Color color);
    Color placeholderColor() const { return m_placeholderColor; }
    void setPlaceholderColor(Color color);
    bool overwriteMode() const { return m_overwriteMode; }
    void setOverwriteMode(bool overwrite);
    bool autoScroll() const { return m_autoScroll; }
    void setAutoScroll(bool autoScroll);
    bool canUndo() const { return m_editor.canUndo(); }
    bool canRedo() const { return m_editor.canRedo(); }

    Q_INVOKABLE void select(int start, int end);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void clear();
    Q_INVOKABLE void copy();
    Q_INVOKABLE void cut();
    Q_INVOKABLE void paste(const QString& text);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    void keyPressEvent(class KeyEvent* event) override;
    void mousePressEvent(class MouseEvent* event) override;
    void mouseMoveEvent(class MouseEvent* event) override;
    void paint(TextGridBuffer& buffer, int absX, int absY,
               const ClipRect& clipRect = ClipRect()) const override;

signals:
    void textChanged();
    void colorChanged();
    void placeholderTextChanged();
    void placeholderColorChanged();
    void overwriteModeChanged();
    void autoScrollChanged();
    void cursorPositionChanged();
    void selectionChanged();
    void readOnlyChanged();
    void contentXChanged();
    void maximumLengthChanged();
    void validatorChanged();
    void acceptableInputChanged();
    void echoModeChanged();
    void echoCharacterChanged();
    void cursorShapeChanged();
    void cursorColorChanged();
    void cursorBlinkingChanged();
    void cursorBlinkRateChanged();
    void cursorVisibleChanged();
    void cursorDelegateChanged();
    void selectionColorChanged();
    void selectedTextColorChanged();
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
    void applyEditorChange(const TextEditorState::Change& change, const EditorStatus& before);
    QString constrainedReplacement(const QString& replacement) const;
    QString displayText() const;
    int displayColumnForPosition(int position) const;
    int positionForDisplayColumn(int column) const;
    int positionAtMouse(int x) const;
    void ensureCursorVisible();
    void updateImplicitWidth();
    void resetBlinkTimer();
    void updateTerminalConnection();

    TextEditorState m_editor;
    // "default" is the terminal's foreground (alpha 0 -> SGR 39),
    // not invisible ink: a fixed "white" default is unreadable on a light theme.
    Color m_color = "default";
    QString m_placeholderText;
    Color m_placeholderColor;
    uint32_t m_resolvedPlaceholderColor = 0;
    bool m_hasResolvedPlaceholderColor = false;
    QString m_echoCharacter = "*";
    QRegularExpression m_validator;
    // Resolved caches mirror the Color members: an ANSI-named default must not be
    // written as literal RGB, or the terminal theme stops applying to it (setters
    // early-return when the color is unchanged, so the initializer is the only chance).
    uint32_t m_resolvedColor = ColorMapper::fromString(m_color, 0xFFFFFFFF);
    int m_contentX = 0;
    int m_maximumLength = 0;
    bool m_readOnly = false;
    bool m_overwriteMode = false;
    bool m_autoScroll = true;
    EchoMode m_echoMode = Normal;

    CursorShape m_cursorShape = Block;
    Color m_cursorColor;
    uint32_t m_resolvedCursorColor = 0;
    bool m_hasResolvedCursorColor = false;
    bool m_cursorBlinking = false;
    int m_cursorBlinkRate = 500;
    bool m_cursorVisible = true;
    QString m_cursorDelegate;
    // On the QmlTui Clock so cursor blink is steppable in tests.
    ClockTimer* m_blinkTimer = nullptr;
    bool m_blinkState = true;

    Color m_selectionColor;
    uint32_t m_resolvedSelectionColor = 0;
    bool m_hasResolvedSelectionColor = false;
    Color m_selectedTextColor;
    uint32_t m_resolvedSelectedTextColor = 0;
    bool m_hasResolvedSelectedTextColor = false;
    QPointer<Terminal> m_observedTerminal;
    QMetaObject::Connection m_appActiveConnection;
    qint64 m_lastClickMs = -1;
    int m_lastClickPos = -1;
};

} // namespace qmltui
