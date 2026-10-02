#include "Terminal.h"
#include "../engine/TerminalEngine.h"
#include "../engine/FrameRenderer.h"
#include "../engine/StreamRenderer.h"
#include "Popup.h"

namespace qtui {

Terminal::Terminal(QObject *parent) : Rectangle(parent) {
    m_engine = new TerminalEngine(this, this);
    connect(this, &Item::dirty, m_engine, &TerminalEngine::requestRender);
}

Terminal::~Terminal() {
    delete m_engine;
}

void Terminal::print(const QString& text) {
    if (m_engine) {
        m_engine->print(text);
    }
}

void Terminal::append(qtui::Item* item) {
    if (m_engine) {
        m_engine->append(item);
    }
}

void Terminal::setUseAlternateScreen(bool use) {
    if (m_useAlternateScreen == use) return;
    m_useAlternateScreen = use;
    emit useAlternateScreenChanged();
}

void Terminal::registerPopup(Popup* popup) {
    if (!popup || m_popups.contains(popup)) return;
    m_popups.append(popup);
    markDirty();
}

void Terminal::unregisterPopup(Popup* popup) {
    if (!popup || !m_popups.contains(popup)) return;
    m_popups.removeOne(popup);
    markDirty();
}

void Terminal::setFocusedItem(Item* item) {
    if (m_focusedItem == item) return;
    
    Item* oldFocus = m_focusedItem;
    m_focusedItem = item;
    
    if (oldFocus) {
        oldFocus->setFocus(false);
        oldFocus->setActiveFocus(false);
    }
    if (m_focusedItem) {
        m_focusedItem->setActiveFocus(true);
    }
}

void Terminal::findFocusableItems(Item* root, QList<Item*>& list) {
    if (!root || !root->visible()) return;
    
    if (root->focusable()) {
        list.append(root);
    }
    
    for (Item* child : root->childItems()) {
        findFocusableItems(child, list);
    }
}

void Terminal::setFillTerminal(bool fill) {
    m_fillTerminalExplicit = true;
    if (m_fillTerminal == fill) return;
    m_fillTerminal = fill;
    emit fillTerminalChanged();
    emit widthChanged();
    emit heightChanged();
    markDirty();
}

void Terminal::setWidth(int w) {
    if (m_fillTerminal && !m_fillTerminalExplicit) {
        m_fillTerminal = false;
        emit fillTerminalChanged();
    }
    Item::setWidth(w);
}

void Terminal::setHeight(int h) {
    if (m_fillTerminal && !m_fillTerminalExplicit) {
        m_fillTerminal = false;
        emit fillTerminalChanged();
    }
    Item::setHeight(h);
}

void Terminal::setRendererMode(RendererMode mode) {
    if (m_rendererMode == mode) return;
    m_rendererMode = mode;
    
    if (m_engine) {
        if (m_rendererMode == RendererMode::Frame) {
            m_engine->setRenderer(new FrameRenderer());
        } else {
            m_engine->setRenderer(new StreamRenderer());
        }
    }
    
    emit rendererModeChanged();
    markDirty();
}

int Terminal::width() const {
    if (m_fillTerminal && m_engine) {
        return m_engine->terminalWidth();
    }
    return Item::width();
}

int Terminal::height() const {
    if (m_fillTerminal && m_engine) {
        return m_engine->terminalHeight();
    }
    return Item::height();
}

static void dimBuffer(TextGridBuffer& buffer, const ClipRect& clipRect) {
    for (int y = 0; y < buffer.height(); ++y) {
        for (int x = 0; x < buffer.width(); ++x) {
            if (clipRect.contains(x, y)) {
                Cell& cell = buffer.cellAt(x, y);
                // Dim fg (shift right by 1 to halve, or divide by 2)
                uint8_t a_fg = (cell.fgColor >> 24) & 0xFF;
                uint8_t r_fg = (cell.fgColor >> 16) & 0xFF;
                uint8_t g_fg = (cell.fgColor >> 8) & 0xFF;
                uint8_t b_fg = cell.fgColor & 0xFF;
                cell.fgColor = (a_fg << 24) | ((r_fg / 2) << 16) | ((g_fg / 2) << 8) | (b_fg / 2);
                
                // Dim bg
                uint8_t a_bg = (cell.bgColor >> 24) & 0xFF;
                uint8_t r_bg = (cell.bgColor >> 16) & 0xFF;
                uint8_t g_bg = (cell.bgColor >> 8) & 0xFF;
                uint8_t b_bg = cell.bgColor & 0xFF;
                cell.bgColor = (a_bg << 24) | ((r_bg / 2) << 16) | ((g_bg / 2) << 8) | (b_bg / 2);
            }
        }
    }
}

void Terminal::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!m_visible) return;
    
    // Render normal hierarchy
    Rectangle::render(buffer, absX, absY, clipRect);
    
    // Render popups as absolute overlay layers on top of everything
    for (Popup* popup : m_popups) {
        if (popup && popup->visible()) {
            if (popup->modal() && popup->dimBackground()) {
                dimBuffer(buffer, clipRect);
            }
            
            QPoint absPos = popup->absolutePosition();
            // Pass the absolute terminal bounds as clipRect (clipRect is usually full screen)
            popup->render(buffer, absPos.x(), absPos.y(), clipRect);
        }
    }
}

} // namespace qtui
