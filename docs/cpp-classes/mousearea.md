# MouseArea

## 1. Class Overview

`MouseArea` is an invisible item that captures terminal mouse input (presses, releases, clicks, and movement coordinates `mouseX`, `mouseY`). Passive pointer movement is opt-in through `hoverEnabled`; button-held drag motion remains available without it.

Developers use `MouseArea` to handle mouse clicks and hover tracking in QML layouts.

## 2. Project Structure and Dependencies

- **Header**: `Terminal/src/items/MouseArea.h`
- **Implementation**: `Terminal/src/items/MouseArea.cpp`
- **Dependencies**: `Item.h`, `MouseEvent.h`.

## 3. Class Hierarchy and Role

- **Base Class**: `Item`
  - Inherits spatial bounds, positioning, and input handling hooks.

## 4. Q_PROPERTY Declarations

| Property | Type | READ | WRITE | NOTIFY | Description |
|----------|------|------|-------|--------|-------------|
| `mouseX` | `int` | `mouseX` | — | `mouseXChanged` | Current X mouse position relative to this MouseArea. |
| `mouseY` | `int` | `mouseY` | — | `mouseYChanged` | Current Y mouse position relative to this MouseArea. |
| `hoverEnabled` | `bool` | `hoverEnabled` | `setHoverEnabled` | `hoverEnabledChanged` | Requests passive mouse-motion reporting while true. Defaults to false. |

## 7. Signals

#### void mouseXChanged()
Emitted when the relative X coordinate of the mouse cursor changes.

#### void mouseYChanged()
Emitted when the relative Y coordinate of the mouse cursor changes.

#### void hoverEnabledChanged()
Emitted when passive hover tracking is enabled or disabled.

#### void pressed(qamui::MouseEvent *event)
Emitted when a mouse button is pressed inside the MouseArea bounds.

#### void released(qamui::MouseEvent *event)
Emitted when a mouse button is released over the MouseArea bounds.

#### void clicked(qamui::MouseEvent *event)
Emitted when a mouse press and release sequence completes within the MouseArea bounds.

## 9. Public Methods

#### void setMousePosition(int x, int y)
Manually updates the recorded cursor coordinates relative to this MouseArea.

## 16. Usage Example

```cpp
#include <Qamui/Core/items/MouseArea.h>
#include <QDebug>

using namespace qamui;

// In a parent item initialization:
MouseArea* area = new MouseArea(parentItem);
area->setWidth(20);
area->setHeight(3);

QObject::connect(area, &MouseArea::clicked, [](MouseEvent* event) {
    qDebug() << "Mouse clicked at:" << event->x() << event->y();
});
```

See the [QML type reference](../qml-types/mousearea.md) for QML usage.
