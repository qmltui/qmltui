#pragma once

#include <QtuiExport.h>

#include <Item.h>
#include <QTimer>
#include <QList>
#include <QString>

namespace qtui {

struct QTUI_EXTRAS_EXPORT ToastItem {
    QString title;
    QString message;
    QString type; // "info", "success", "warning", "error"
    int remainingMs;
};

class QTUI_EXTRAS_EXPORT ToastManager : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qtui::Color infoColor READ infoColor WRITE setInfoColor NOTIFY infoColorChanged)
    Q_PROPERTY(qtui::Color successColor READ successColor WRITE setSuccessColor NOTIFY successColorChanged)
    Q_PROPERTY(qtui::Color warningColor READ warningColor WRITE setWarningColor NOTIFY warningColorChanged)
    Q_PROPERTY(qtui::Color errorColor READ errorColor WRITE setErrorColor NOTIFY errorColorChanged)
    Q_PROPERTY(int toastWidth READ toastWidth WRITE setToastWidth NOTIFY toastWidthChanged)
    Q_PROPERTY(QString position READ position WRITE setPosition NOTIFY positionChanged)

public:
    explicit ToastManager(QObject *parent = nullptr);
    ~ToastManager() override = default;

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

signals:
    void infoColorChanged();
    void successColorChanged();
    void warningColorChanged();
    void errorColorChanged();
    void toastWidthChanged();
    void positionChanged();

private slots:
    void onTick();

private:
    QList<ToastItem> m_toasts;
    QTimer m_timer;

    Color m_infoColor = "cyan";
    uint32_t m_resolvedInfoColor = 0xFF61AFEF;

    Color m_successColor = "green";
    uint32_t m_resolvedSuccessColor = 0xFF98C379;

    Color m_warningColor = "yellow";
    uint32_t m_resolvedWarningColor = 0xFFE5C07B;

    Color m_errorColor = "red";
    uint32_t m_resolvedErrorColor = 0xFFE06C75;

    int m_toastWidth = 28;
    QString m_position = "top-right";
};

} // namespace qtui
