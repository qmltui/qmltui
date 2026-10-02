# Missing Features & Future Roadmap

This document outlines architectural limitations and feature gaps in Qtui that should be considered for future development.

## 1. Advanced Controls
Qtui includes buttons, checkboxes, radio buttons, switches, sliders, dialogs,
text inputs, and text areas. Remaining gaps include combo boxes, dropdowns,
menus, and a general progress control.

## 2. Layering & Popups (Z-Index)
Currently, the rendering pipeline relies entirely on tree traversal order (Painter's algorithm) based on QML declarative ordering.
- **Z-Index Property**: There is no dynamic `z` property to force an item to render above a sibling defined later in the QML file.
- **General overlay stacking**: `Popup` and modal dialogs are supported, but
  arbitrary z-ordering and multiple overlay layers remain limited.
- **Context Menus**: There is no dedicated context-menu control or reusable
  menu model yet.

## 3. Rich Interactions
- **Drag & Drop**: No API currently exists for initiating drag operations, visualizing dragged items, or defining drop targets.
- **Mouse Hover Tracking**: Terminals support `ANY_EVENT_MOUSE` to track mouse movements even without buttons pressed. Currently, Qtui only captures explicit clicks and scrolls. Enabling hover would allow for tooltip popups and button hover-state styling.

## 4. Advanced Styling
- **Theming / Stylesheets**: Colors and styling are hardcoded per item (`color: "red"`). A robust styling system (like QSS or CSS) that allows global themes (Dark/Light mode) without repeating properties everywhere would significantly improve developer experience.
- **Borders & Shadows**: Rendering drop shadows using ANSI background colors and ASCII block characters would add a lot of depth to the UI.

## 5. Flexible Layouts
- **Flexbox / Grid Layouts**: While `Row`, `Column`, and `Anchors` provide a solid foundation, an equivalent to CSS Flexbox (wrapping rows, proportional stretching, space-between) or CSS Grid would make complex responsive terminal interfaces significantly easier to build.
