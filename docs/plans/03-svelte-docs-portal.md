# Implementation Plan 03: Svelte-Powered Documentation Portal & Quickstart

## 1. Goal Description

Build a high-performance, dark-mode documentation website in [`docs/`](file:///home/yosri/qtui/docs) powered by **SvelteKit** and **mdsvex** (with static adapter prerendering for GitHub Pages).

### Key Pillars
1. **Brand Aesthetics**: Qt Emerald Green (`#2CDE85`) on Dark Terminal Navy (`#0d1117`), authentic monospace typography, and responsive layouts.
2. **The 5-Minute "Hello Qtui" Quickstart**: Zero-install setup using CMake `FetchContent`.
3. **Svelte-Native Widgets**: Interactive terminal window chrome, state switchers, and client-side instant search (Pagefind).

---

## 2. Directory Layout in `docs/`

```text
docs/
├── package.json               # SvelteKit, mdsvex, Tailwind/Bits UI, Pagefind
├── svelte.config.js           # adapter-static with prerender: true
├── vite.config.js             # Vite + Svelte plugins
├── src/
│   ├── lib/
│   │   ├── components/
│   │   │   ├── TerminalWindow.svelte   # Window frame with red/yellow/green dots
│   │   │   ├── AsciinemaPlayer.svelte  # Terminal cast player wrapper
│   │   │   ├── ComponentCard.svelte    # Widget catalog preview card
│   │   │   └── SearchModal.svelte      # Pagefind instant search modal
│   │   ├── styles/
│   │   │   └── app.css                 # Terminal palette & code block themes
│   ├── routes/
│   │   ├── +layout.svelte              # Top navigation, sidebar, dark theme provider
│   │   ├── +page.svelte                # Landing page (Hero GIF + Quickstart)
│   │   ├── guide/
│   │   │   ├── why-qtui/+page.md       # Declarative QML vs Immediate-mode Ratatui
│   │   │   ├── quickstart/+page.md     # 5-minute "Hello World" with FetchContent
│   │   │   ├── concepts/+page.md       # Retained-mode, cell grid, event loop
│   │   │   ├── layouts/+page.md        # Anchors, Row, Column, ScrollView
│   │   │   ├── cxx-models/+page.md     # QAbstractItemModel, QSortFilterProxyModel
│   │   │   ├── custom-items/+page.md   # Subclassing qtui::Item in C++
│   │   │   └── testing/+page.md        # Headless TestRunner & scenario tests
│   │   ├── components/
│   │   │   ├── +page.svelte            # Visual grid of all 31 controls
│   │   │   ├── button/+page.md         # Button QML reference (Qt standard)
│   │   │   ├── table-view/+page.md     # TableView QML reference (Qt standard)
│   │   │   └── ...
│   │   └── showcase/
│   │       ├── +page.svelte            # Filterable 17-app gallery
│   │       └── ...
└── static/
    ├── images/                        # High-resolution screenshots
    └── casts/                         # Asciinema recording files
```

---

## 3. The 5-Minute "Hello Qtui" Quickstart Page

This guide will be prominently featured on both the landing page and `guide/quickstart/`:

### A. Minimal `CMakeLists.txt` (Zero Install via FetchContent)
```cmake
cmake_minimum_required(VERSION 3.20)
project(my_qtui_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)

include(FetchContent)
FetchContent_Declare(
    Qtui
    GIT_REPOSITORY https://github.com/yosriayed/Qtui.git
    GIT_TAG        v0.4.0
)
FetchContent_MakeAvailable(Qtui)

qt_add_executable(my_qtui_app main.cpp)
qt_add_qml_module(my_qtui_app
    URI MyApp
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET QtuiCore QtuiControls
)
target_link_libraries(my_qtui_app PRIVATE Qtui::Core Qtui::Controls)
```

### B. Minimal `main.cpp`
```cpp
#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("MyApp", "Main");
    return app.exec();
}
```

### C. Minimal `Main.qml`
```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    id: root
    fillTerminal: true
    color: "#0d1117"

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 1

        Text {
            text: "⚡ Welcome to Qtui!"
            color: "cyan"
            bold: true
        }

        Button {
            text: "Press [Q] or [Esc] to Exit"
            focus: true
            onClicked: Qt.quit()
        }
    }
}
```

---

## 4. GitHub Actions Deployment (`.github/workflows/docs.yml`)

Automate static site deployment on tag and `main` pushes:
```yaml
name: Deploy Documentation
on:
  push:
    branches: [main]
    paths: ['docs/**']

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: pnpm/action-setup@v3
        with:
          version: 9
      - uses: actions/setup-node@v4
        with:
          node-version: 22
          cache: 'pnpm'
          cache-dependency-path: docs/pnpm-lock.yaml
      - run: cd docs && pnpm install
      - run: cd docs && pnpm build
      - uses: actions/deploy-pages@v4
        with:
          artifact_name: github-pages
          path: docs/build
```
