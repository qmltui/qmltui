#include "Terminal.h"
#include "../engine/TerminalEngine.h"
#include "../engine/FrameRenderer.h"
#include "../engine/StreamRenderer.h"

namespace qtui {

Terminal::Terminal(QObject *parent) : Item(parent) {
    m_engine = new TerminalEngine(this, this);
    connect(this, &Item::dirty, m_engine, &TerminalEngine::requestRender);
}

Terminal::~Terminal() = default;

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
    if (m_fillTerminal == fill) return;
    m_fillTerminal = fill;
    emit fillTerminalChanged();
    emit widthChanged();
    emit heightChanged();
    markDirty();
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

void Terminal::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!m_visible) return;
    Item::render(buffer, absX, absY, clipRect);
}

} // namespace qtui
