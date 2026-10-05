# Documentation Implementation Plan

Based on your request, I will generate four comprehensive documentation files covering the library's architecture, its QML API, a guide on building custom components, and a list of missing features for the roadmap.

## Proposed Documents

### 1. `docs/architecture.md`
**Purpose**: Explain the internal rendering and event loop.
**Key Sections**:
- **Core Loop**: `TerminalEngine` integration and run loop.
- **Rendering Pipeline**: The retained-mode rendering model (`TextGridBuffer`, `Cell`, Painter's Algorithm), diff generation, and the `FrameRenderer`.
- **Input Handling**: How `InputDecoder` translates raw terminal bytes into `KeyEvent` and `MouseEvent`.
- **Node Graph**: How the C++ `Item` tree corresponds to QML, managing state, transitions, and parent-child visual clipping.

### 2. `docs/qml_api.md`
**Purpose**: Document the available QML elements and link them to existing examples.
**Key Sections**:
- **Core Elements**: `Terminal`, `Item`, `Text`, `MouseArea`.
- **Layouts**: `Row`, `Column`, `Anchors`.
- **Views**: `Repeater`, `ListView`, `GridView`, `ScrollView`.
- **Inputs**: `TextInput`, `Button` (Controls).
- **Animations & States**: `State`, `Transition`, `PropertyAnimation`.
*(Will include relative links to the directories in `examples/` like `examples/anchors`, `examples/listview_demo`, etc.)*

### 3. `docs/custom_items.md`
**Purpose**: A developer guide on extending Qtui with new visual or behavioral items.
**Key Sections**:
- **C++ Class Definition**: Inheriting from `qtui::Item`, adding `Q_PROPERTY`, and `QML_ELEMENT`.
- **Rendering**: Overriding protected `paint(...)`; `Item::render()` owns traversal.
- **Event Handling**: Overriding `keyPressEvent()`, `mousePressEvent()`.
- **Implicit Sizing**: Managing `implicitWidth` and `implicitHeight`.

### 4. `docs/missing_features.md`
**Purpose**: Detail missing features that should be considered for the future roadmap.
**Key Sections**:
- **Advanced Controls**: Checkboxes, RadioButtons, Comboboxes, and multi-line TextAreas.
- **Layering & Popups**: Z-index ordering, Modal overlays, and Popup menus (currently difficult due to strict hierarchy).
- **Rich Interactions**: Drag & Drop support, hover states without clicking (terminal mouse tracking modes).
- **Styling**: More robust stylesheet or theme support beyond hardcoded `color` properties.
- **Layout**: Flexible Grid layouts (like CSS Flexbox/Grid equivalents).
