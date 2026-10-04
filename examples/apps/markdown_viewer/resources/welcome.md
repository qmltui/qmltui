# Welcome to Qtui Markdown Viewer

A high-performance retained-mode **terminal Markdown renderer** built with **Qt 6** and KDE's [md4qt](https://github.com/KDE/md4qt) parser library.

## Key Features

- **CommonMark & GFM Compliance**: Full Abstract Syntax Tree (AST) parsing.
- **Syntax Highlighting**: Fenced code blocks for C++, QML, Python, Bash, Rust, JSON, CMake.
- **Unicode Box Tables**: Tables with automatic column calculation and alignment.
- **Section Navigation**: Jump between headings with Tab and Shift+Tab.
- **Line-number Gutter**: Track every rendered row, including wrapped content.
- **Fast Search**: In-document substring searching with visual match highlights.
- **Themes**: Multiple terminal palettes (Tokyo Night, Dracula, Nord, Monokai, Amber CRT).

---

## Interactive Task Lists

- [x] Integrate KDE md4qt AST parser
- [x] Implement syntax highlighting engine
- [x] Support Unicode box tables and alignments
- [x] Add keyboard section navigation and rendered-row line numbers
- [ ] Add PDF export backend
- [ ] Add image preview rendering via Sixel / Kitty protocol

---

## Code Highlighting Showcase

### Modern C++ with Qt 6

```cpp
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include "MarkdownDocument.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("Qtui.Examples.MarkdownViewer", "Main");
    return app.exec();
}
```

### Declarative QML Component

```qml
import QtQml 2.15
import Qtui
import Qtui.Examples.MarkdownViewer 1.0

Terminal {
    id: root
    width: 80
    height: 24

    MarkdownView {
        anchors.fill: parent
        document: mdDoc
        theme: "tokyo_night"
    }
}
```

---

## GFM Tables Demo

| Component | Description | Status |
| :--- | :---: | ---: |
| **MarkdownDocument** | AST Parser & Layout Engine | Completed |
| **MarkdownView** | Terminal Viewport Renderer | Active |
| **Line Gutter** | Rendered Row Navigation | Active |
| **SyntaxHighlighter** | Multi-language Tokenizer | Active |

---

## Blockquotes & Citations

> "The true delight is in the finding out, rather than in the knowing."
> — *Isaac Asimov*

> Terminal user interfaces combine the speed of CLI with the visual hierarchy of modern GUIs.

---

## Keyboard Shortcuts Reference

- `Tab` / `Shift+Tab` : Jump to next / previous section
- `j` / `k` or `Up` / `Down` : Scroll document lines
- `PageUp` / `PageDown` : Scroll page by page
- `/` : Open search bar
- `n` / `N` : Jump to next / previous search match
- `t` : Cycle color themes
- `o` : Open sample document switcher
- `q` / `Esc` : Quit viewer
