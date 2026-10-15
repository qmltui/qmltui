#include <QmlTui/Controls/Toast.h>
#include <QmlTui/Core/Engine/Clock_p.h>
#include <QmlTui/Core/Primitives/ColorMapper.h>
#include <QmlTui/Core/Primitives/UnicodeText.h>
#include <algorithm>

namespace qmltui {
void Toast::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    emit colorChanged();
    markDirty();
}

void Toast::setTextColor(Color color) {
    if (m_textColor == color) return;
    m_textColor = color;
    emit textColorChanged();
    markDirty();
}

Toast::Toast(QObject* parent) : Item(parent) {
    setImplicitWidth(30);
    setImplicitHeight(10);

    m_timer = new ClockTimer(this);
    connect(m_timer, &ClockTimer::timeout, this, &Toast::onTick);
    m_timer->setInterval(100);
}

void Toast::showToast(const QString& title, const QString& message, const QString& type,
                      int timeoutMs) {
    m_toasts.append(ToastItem{title, message, type, (std::max)(100, timeoutMs)});
    if (!m_timer->isActive()) m_timer->start();
    markDirty();
}

void Toast::setInfoColor(Color color) {
    if (m_infoColor == color) return;
    m_infoColor = color;
    m_resolvedInfoColor = ColorMapper::fromString(m_infoColor, 0);
    emit infoColorChanged();
    markDirty();
}

void Toast::setSuccessColor(Color color) {
    if (m_successColor == color) return;
    m_successColor = color;
    m_resolvedSuccessColor = ColorMapper::fromString(m_successColor, 0);
    emit successColorChanged();
    markDirty();
}

void Toast::setWarningColor(Color color) {
    if (m_warningColor == color) return;
    m_warningColor = color;
    m_resolvedWarningColor = ColorMapper::fromString(m_warningColor, 0);
    emit warningColorChanged();
    markDirty();
}

void Toast::setErrorColor(Color color) {
    if (m_errorColor == color) return;
    m_errorColor = color;
    m_resolvedErrorColor = ColorMapper::fromString(m_errorColor, 0);
    emit errorColorChanged();
    markDirty();
}

void Toast::setToastWidth(int w) {
    if (m_toastWidth == w) return;
    m_toastWidth = w;
    emit toastWidthChanged();
    markDirty();
}

void Toast::setPosition(const QString& pos) {
    if (m_position == pos) return;
    m_position = pos;
    emit positionChanged();
    markDirty();
}

void Toast::onTick() {
    if (m_toasts.isEmpty()) {
        m_timer->stop();
        return;
    }

    bool changed = false;
    for (int i = m_toasts.size() - 1; i >= 0; --i) {
        m_toasts[i].remainingMs -= 100;
        if (m_toasts[i].remainingMs <= 0) {
            m_toasts.removeAt(i);
            changed = true;
        }
    }
    if (changed) markDirty();
    if (m_toasts.isEmpty()) m_timer->stop();
}

void Toast::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    if (!m_visible || m_toasts.isEmpty()) return;

    int curY = absY;
    if (m_position.contains("bottom")) {
        curY = absY + height() - (static_cast<int>(m_toasts.size()) * 4);
    }

    int tWidth = m_toastWidth;
    const uint32_t background = ColorMapper::fromString(m_color, 0);
    const uint32_t messageColor = ColorMapper::fromString(m_textColor, 0);

    for (const ToastItem& toast : m_toasts) {
        int toastX = absX + width() - tWidth;
        if (m_position.contains("left")) {
            toastX = absX;
        }

        uint32_t borderColor = m_resolvedInfoColor;
        if (toast.type == "success")
            borderColor = m_resolvedSuccessColor;
        else if (toast.type == "warning")
            borderColor = m_resolvedWarningColor;
        else if (toast.type == "error")
            borderColor = m_resolvedErrorColor;

        // Draw Box
        buffer.drawBox(toastX, curY, tWidth, 3, borderColor, background);

        // Header Title
        QString prefix =
            (toast.type == "error"
                 ? "✖ "
                 : (toast.type == "success" ? "✔ " : (toast.type == "warning" ? "⚠ " : "ℹ ")));
        buffer.writeText(toastX + 1, curY,
                         prefix + UnicodeText::elideText(toast.title, (std::max)(0, tWidth - 4)),
                         borderColor, background, TextAttribute::Bold);

        // Body message.
        buffer.writeText(toastX + 1, curY + 1,
                         UnicodeText::elideText(toast.message, (std::max)(0, tWidth - 3)),
                         messageColor, background);

        curY += 4;
    }
}

} // namespace qmltui
