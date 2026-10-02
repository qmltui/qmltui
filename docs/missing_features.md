# Missing Features & Future Roadmap

This document outlines architectural limitations and feature gaps in Qtui that should be considered for future development.

## 1. Advanced Controls
Qtui includes buttons, checkboxes, radio buttons, switches, sliders, dialogs,
text inputs, and text areas. Remaining gaps include combo boxes, dropdowns,
menus, and a general progress control.

## 2. Overlay Controls
Dynamic `z` ordering, owned popup stacks, nested modality, and attached text
tooltips are supported. Remaining gaps are dedicated context-menu and menu
controls, reusable menu models, and arbitrary custom-content tooltips.

## 3. Rich Interactions
- **Drag & Drop**: No API currently exists for initiating drag operations, visualizing dragged items, or defining drop targets.
- **Public hover styling**: Hover tracking powers tooltips internally, but a
  public `hovered` state and enter/leave signals for general control styling
  are not yet exposed.

## 4. Advanced Styling
- **Theming / Stylesheets**: Colors and styling are hardcoded per item (`color: "red"`). A robust styling system (like QSS or CSS) that allows global themes (Dark/Light mode) without repeating properties everywhere would significantly improve developer experience.
- **Borders & Shadows**: Rendering drop shadows using ANSI background colors and ASCII block characters would add a lot of depth to the UI.

## 5. Flexible Layouts
- **Flexbox / Grid Layouts**: While `Row`, `Column`, and `Anchors` provide a solid foundation, an equivalent to CSS Flexbox (wrapping rows, proportional stretching, space-between) or CSS Grid would make complex responsive terminal interfaces significantly easier to build.
