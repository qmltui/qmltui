#include "ToastManager.h"
#include <ColorMapper.h>
#include <algorithm>

namespace qtui {

ToastManager::ToastManager(QObject *parent) : Item(parent) {
    setImplicitWidth(30);
    setImplicitHeight(10);

    connect(&m_timer, &QTimer::timeout, this, &ToastManager::onTick);
    m_timer.setInterval(100);
}

void ToastManager::showToast(const QString &title, const QString &message, const QString &type, int timeoutMs) {
    m_toasts.append(ToastItem{ title, message, type, (std::max)(100, timeoutMs) });
    if (!m_timer.isActive()) m_timer.start();
    markDirty();
}

void ToastManager::setInfoColor(const QString &color) {
    if (m_infoColor == color) return;
    m_infoColor = color;
    m_resolvedInfoColor = ColorMapper::fromString(m_infoColor, 0xFF61AFEF);
    emit infoColorChanged();
    markDirty();
}

void ToastManager::setSuccessColor(const QString &color) {
    if (m_successColor == color) return;
    m_successColor = color;
    m_resolvedSuccessColor = ColorMapper::fromString(m_successColor, 0xFF98C379);
    emit successColorChanged();
    markDirty();
}

void ToastManager::setWarningColor(const QString &color) {
    if (m_warningColor == color) return;
    m_warningColor = color;
    m_resolvedWarningColor = ColorMapper::fromString(m_warningColor, 0xFFE5C07B);
    emit warningColorChanged();
    markDirty();
}

void ToastManager::setErrorColor(const QString &color) {
    if (m_errorColor == color) return;
    m_errorColor = color;
    m_resolvedErrorColor = ColorMapper::fromString(m_errorColor, 0xFFE06C75);
    emit errorColorChanged();
    markDirty();
}

void ToastManager::setToastWidth(int w) {
    if (m_toastWidth == w) return;
    m_toastWidth = w;
    emit toastWidthChanged();
    markDirty();
}

void ToastManager::setPosition(const QString &pos) {
    if (m_position == pos) return;
    m_position = pos;
    emit positionChanged();
    markDirty();
}

void ToastManager::onTick() {
    if (m_toasts.isEmpty()) {
        m_timer.stop();
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
    if (m_toasts.isEmpty()) m_timer.stop();
}

void ToastManager::render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect) {
    if (!m_visible || m_toasts.isEmpty()) return;

    Item::render(buffer, absX, absY, clipRect);

    int curY = absY;
    if (m_position.contains("bottom")) {
        curY = absY + height() - (static_cast<int>(m_toasts.size()) * 4);
    }

    int tWidth = m_toastWidth;

    for (const ToastItem &toast : m_toasts) {
        int toastX = absX + width() - tWidth;
        if (m_position.contains("left")) {
            toastX = absX;
        }

        uint32_t borderColor = m_resolvedInfoColor;
        if (toast.type == "success") borderColor = m_resolvedSuccessColor;
        else if (toast.type == "warning") borderColor = m_resolvedWarningColor;
        else if (toast.type == "error") borderColor = m_resolvedErrorColor;

        // Draw Box
        buffer.drawBox(toastX, curY, tWidth, 3, borderColor, 0xFF181A1F);

        // Header Title
        QString prefix = (toast.type == "error" ? "✖ " : (toast.type == "success" ? "✔ " : "ℹ "));
        buffer.writeText(toastX + 1, curY, prefix + toast.title.left(tWidth - 4), borderColor, 0xFF181A1F, TextAttribute::Bold);

        // Body message
        buffer.writeText(toastX + 1, curY + 1, toast.message.left(tWidth - 3), 0xFFFFFFFF, 0xFF181A1F);

        curY += 4;
    }
}

} // namespace qtui
