#pragma once

#include <Item.h>
#include <QTimer>
#include <QList>
#include <QString>

namespace qtui {

struct ToastItem {
    QString title;
    QString message;
    QString type; // "info", "success", "warning", "error"
    int remainingMs;
};

class ToastManager : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString infoColor READ infoColor WRITE setInfoColor NOTIFY infoColorChanged)
    Q_PROPERTY(QString successColor READ successColor WRITE setSuccessColor NOTIFY successColorChanged)
    Q_PROPERTY(QString warningColor READ warningColor WRITE setWarningColor NOTIFY warningColorChanged)
    Q_PROPERTY(QString errorColor READ errorColor WRITE setErrorColor NOTIFY errorColorChanged)
    Q_PROPERTY(int toastWidth READ toastWidth WRITE setToastWidth NOTIFY toastWidthChanged)
    Q_PROPERTY(QString position READ position WRITE setPosition NOTIFY positionChanged)

public:
    explicit ToastManager(QObject *parent = nullptr);
    ~ToastManager() override = default;

    Q_INVOKABLE void showToast(const QString &title, const QString &message, const QString &type = "info", int timeoutMs = 3000);

    QString infoColor() const { return m_infoColor; }
    void setInfoColor(const QString &color);

    QString successColor() const { return m_successColor; }
    void setSuccessColor(const QString &color);

    QString warningColor() const { return m_warningColor; }
    void setWarningColor(const QString &color);

    QString errorColor() const { return m_errorColor; }
    void setErrorColor(const QString &color);

    int toastWidth() const { return m_toastWidth; }
    void setToastWidth(int width);

    QString position() const { return m_position; }
    void setPosition(const QString &pos);

    void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) override;

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

    QString m_infoColor = "cyan";
    uint32_t m_resolvedInfoColor = 0xFF61AFEF;

    QString m_successColor = "green";
    uint32_t m_resolvedSuccessColor = 0xFF98C379;

    QString m_warningColor = "yellow";
    uint32_t m_resolvedWarningColor = 0xFFE5C07B;

    QString m_errorColor = "red";
    uint32_t m_resolvedErrorColor = 0xFFE06C75;

    int m_toastWidth = 28;
    QString m_position = "top-right";
};

} // namespace qtui
