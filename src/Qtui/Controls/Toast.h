#pragma once

#include <Qtui/Core/primitives/QtuiExport.h>

#include <Qtui/Core/items/Item.h>
#include <QTimer>
#include <QList>
#include <QString>
#include <Qtui/Core/primitives/ColorMapper.h>

namespace qtui {

struct QTUI_CONTROLS_EXPORT ToastItem {
    QString title;
    QString message;
    QString type; // "info", "success", "warning", "error"
    int remainingMs;
};

// Transient notifications stacked in a corner. Status colors are semantic and
// directly configurable, along with the body text and surface.
class QTUI_CONTROLS_EXPORT Toast : public Item {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qtui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qtui::Color textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)

    Q_PROPERTY(qtui::Color infoColor READ infoColor WRITE setInfoColor NOTIFY infoColorChanged)
    Q_PROPERTY(qtui::Color successColor READ successColor WRITE setSuccessColor NOTIFY successColorChanged)
    Q_PROPERTY(qtui::Color warningColor READ warningColor WRITE setWarningColor NOTIFY warningColorChanged)
    Q_PROPERTY(qtui::Color errorColor READ errorColor WRITE setErrorColor NOTIFY errorColorChanged)
    Q_PROPERTY(int toastWidth READ toastWidth WRITE setToastWidth NOTIFY toastWidthChanged)
    Q_PROPERTY(QString position READ position WRITE setPosition NOTIFY positionChanged)

public:
    explicit Toast(QObject *parent = nullptr);
    ~Toast() override = default;

    Q_INVOKABLE void showToast(const QString &title, const QString &message, const QString &type = "info", int timeoutMs = 3000);

    Color infoColor() const { return m_infoColor; }
    void setInfoColor(Color color);

    Color successColor() const { return m_successColor; }
    void setSuccessColor(Color color);

    Color warningColor() const { return m_warningColor; }
    void setWarningColor(Color color);

    Color errorColor() const { return m_errorColor; }
    void setErrorColor(Color color);

    int toastWidth() const { return m_toastWidth; }
    void setToastWidth(int width);

    QString position() const { return m_position; }
    void setPosition(const QString &pos);

    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) const override;

    Color textColor() const { return m_textColor; }
    void setTextColor(Color color);

    Color color() const { return m_color; }
    void setColor(Color color);

signals:
    void colorChanged();
    void textColorChanged();
    void infoColorChanged();
    void successColorChanged();
    void warningColorChanged();
    void errorColorChanged();
    void toastWidthChanged();
    void positionChanged();

private slots:
    void onTick();

private:
    Color m_color = "default";
    Color m_textColor = "default";
    QList<ToastItem> m_toasts;
    QTimer m_timer;

    Color m_infoColor = "default";
    // Resolved caches mirror the Color members: an ANSI-named default must not be
    // written as literal RGB, or the terminal theme stops applying to it (setters
    // early-return when the color is unchanged, so the initializer is the only chance).
    uint32_t m_resolvedInfoColor = ColorMapper::fromString(m_infoColor, 0);

    Color m_successColor = "default";
    uint32_t m_resolvedSuccessColor = ColorMapper::fromString(m_successColor, 0);

    Color m_warningColor = "default";
    uint32_t m_resolvedWarningColor = ColorMapper::fromString(m_warningColor, 0);

    Color m_errorColor = "default";
    uint32_t m_resolvedErrorColor = ColorMapper::fromString(m_errorColor, 0);

    int m_toastWidth = 28;
    QString m_position = "top-right";
};

} // namespace qtui
