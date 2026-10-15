# MouseArea

## Class Overview

`MouseArea` is an invisible item that captures terminal mouse input (presses, releases, clicks, and movement coordinates `mouseX`, `mouseY`). Passive pointer movement is opt-in through `hoverEnabled`; button-held drag motion remains available without it.

Developers use `MouseArea` to handle mouse clicks and hover tracking in QML layouts.

## Project Structure and Dependencies

- **Header**: `src/QmlTui/Core/Items/MouseArea.h`
- **Implementation**: `src/QmlTui/Core/Items/MouseArea.cpp`
- **Dependencies**: `Item.h`, `MouseEvent.h`.

## Class Hierarchy and Role

- **Base Class**: `Item`
  - Inherits spatial bounds, positioning, and input handling hooks.

## Q_PROPERTY Declarations

| Property | Type | READ | WRITE | NOTIFY | Description |
|----------|------|------|-------|--------|-------------|
| `mouseX` | `int` | `mouseX` | — | `mouseXChanged` | Current X mouse position relative to this MouseArea. |
| `mouseY` | `int` | `mouseY` | — | `mouseYChanged` | Current Y mouse position relative to this MouseArea. |
| `hoverEnabled` | `bool` | `hoverEnabled` | `setHoverEnabled` | `hoverEnabledChanged` | Requests passive mouse-motion reporting while true. Defaults to false. |

## Signals

#### void mouseXChanged()
Emitted when the relative X coordinate of the mouse cursor changes.

#### void mouseYChanged()
Emitted when the relative Y coordinate of the mouse cursor changes.

#### void hoverEnabledChanged()
Emitted when passive hover tracking is enabled or disabled.

#### void pressed(qmltui::MouseEvent *event)
Emitted when a mouse button is pressed inside the MouseArea bounds.

#### void released(qmltui::MouseEvent *event)
Emitted when a mouse button is released over the MouseArea bounds.

#### void clicked(qmltui::MouseEvent *event)
Emitted when a mouse press and release sequence completes within the MouseArea bounds.

## Public Methods

#### void setMousePosition(int x, int y)
Manually updates the recorded cursor coordinates relative to this MouseArea.

## Usage Example

```cpp
#include <QmlTui/Core/Items/MouseArea.h>
#include <QDebug>

using namespace qmltui;

// In a parent item initialization:
MouseArea* area = new MouseArea(parentItem);
area->setWidth(20);
area->setHeight(3);

QObject::connect(area, &MouseArea::clicked, [](MouseEvent* event) {
    qDebug() << "Mouse clicked at:" << event->x() << event->y();
});
```

See the [QML type reference](../qml-types/mousearea.md) for QML usage.
