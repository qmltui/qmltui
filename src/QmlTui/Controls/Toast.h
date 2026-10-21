#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Items/Item.h>
namespace qmltui {
class ClockTimer;
}
#include <QList>
#include <QString>

namespace qmltui {

// Single queued notification entry managed by Toast.
struct QMLTUI_CONTROLS_EXPORT ToastItem {
    QString title;
    QString message;
    QString type; // "info", "success", "warning", "error"
    int remainingMs;
};

// Transient notifications stacked in a corner. Status colors are semantic and
// directly configurable, along with the body text and surface.
class QMLTUI_CONTROLS_EXPORT Toast : public Item {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qmltui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qmltui::Color textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)

    Q_PROPERTY(qmltui::Color infoColor READ infoColor WRITE setInfoColor NOTIFY infoColorChanged)
    Q_PROPERTY(qmltui::Color successColor READ successColor WRITE setSuccessColor NOTIFY
                   successColorChanged)
    Q_PROPERTY(qmltui::Color warningColor READ warningColor WRITE setWarningColor NOTIFY
                   warningColorChanged)
    Q_PROPERTY(
        qmltui::Color errorColor READ errorColor WRITE setErrorColor NOTIFY errorColorChanged)
    Q_PROPERTY(int toastWidth READ toastWidth WRITE setToastWidth NOTIFY toastWidthChanged)
    // Corner placement: "top-right" (default), "top-left", "bottom-right", or "bottom-left".
    Q_PROPERTY(QString position READ position WRITE setPosition NOTIFY positionChanged)

public:
    explicit Toast(QObject* parent = nullptr);
    ~Toast() override = default;

    Q_INVOKABLE void showToast(const QString& title, const QString& message,
                               const QString& type = "info", int timeoutMs = 3000);

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
    void setPosition(const QString& pos);

    void paint(TextGridBuffer& buffer, int absX, int absY,
               const ClipRect& clipRect = ClipRect()) const override;

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
    // On the QmlTui Clock so toast lifetimes are steppable in tests.
    ClockTimer* m_timer = nullptr;

    Color m_infoColor = "default";
    uint32_t m_resolvedInfoColor = Color::resolve(m_infoColor, 0);

    Color m_successColor = "default";
    uint32_t m_resolvedSuccessColor = Color::resolve(m_successColor, 0);

    Color m_warningColor = "default";
    uint32_t m_resolvedWarningColor = Color::resolve(m_warningColor, 0);

    Color m_errorColor = "default";
    uint32_t m_resolvedErrorColor = Color::resolve(m_errorColor, 0);

    int m_toastWidth = 28;
    QString m_position = "top-right";
};

} // namespace qmltui
