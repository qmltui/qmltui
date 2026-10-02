# Missing Features & Future Roadmap

This document outlines architectural limitations and feature gaps in Qtui that should be considered for future development.

## 1. Advanced Controls
While basic `TextInput` and `TextArea` exist, the library lacks a comprehensive suite of UI controls found in mature GUI frameworks.
- **Checkboxes & RadioButtons**: Toggling boolean states with standard visual indicators (`[ ]`, `[x]`, `( )`, `(o)`).
- **ComboBoxes / Dropdowns**: Selecting from a list of options. Requires popup support (see Layering & Popups below).
- **Sliders & Progress Bars**: Visual numeric representation and manipulation.

## 2. Layering & Popups (Z-Index)
Currently, the rendering pipeline relies entirely on tree traversal order (Painter's algorithm) based on QML declarative ordering.
- **Z-Index Property**: There is no dynamic `z` property to force an item to render above a sibling defined later in the QML file.
- **Modals & Overlays**: Creating a popup dialog that dims the background and traps focus is difficult because focus and rendering are strictly hierarchical.
- **Context Menus**: Right-click context menus are practically impossible without a dedicated "overlay layer" at the root `Terminal` level.

## 3. Rich Interactions
- **Drag & Drop**: No API currently exists for initiating drag operations, visualizing dragged items, or defining drop targets.
- **Mouse Hover Tracking**: Terminals support `ANY_EVENT_MOUSE` to track mouse movements even without buttons pressed. Currently, Qtui only captures explicit clicks and scrolls. Enabling hover would allow for tooltip popups and button hover-state styling.

## 4. Advanced Styling
- **Theming / Stylesheets**: Colors and styling are hardcoded per item (`color: "red"`). A robust styling system (like QSS or CSS) that allows global themes (Dark/Light mode) without repeating properties everywhere would significantly improve developer experience.
- **Borders & Shadows**: Rendering drop shadows using ANSI background colors and ASCII block characters would add a lot of depth to the UI.

## 5. Flexible Layouts
- **Flexbox / Grid Layouts**: While `Row`, `Column`, and `Anchors` provide a solid foundation, an equivalent to CSS Flexbox (wrapping rows, proportional stretching, space-between) or CSS Grid would make complex responsive terminal interfaces significantly easier to build.
