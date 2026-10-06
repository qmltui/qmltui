# ConfirmDialog

## 1. Component Overview

The `ConfirmDialog` component is a modal confirmation popup dialog used in the PLC HMI interface to prompt operators before executing high-impact or risky actions (such as starting maintenance mode, clearing alarms, or changing safety setpoints). It displays a title, message text, and key prompt hints (`[Y] CONFIRM [N] CANCEL`).

Developers use `ConfirmDialog` to prevent accidental operator triggers.

## 2. Project Structure and Dependencies

- **Import Dependencies**:
  - `QtQml` (version 2.15)
  - `Tui` (version 1.0)
  - `IndustrialPlcHmi` (version 1.0)
- **File Location**: `examples/industrial_plc_hmi/ConfirmDialog.qml`

## 3. Component Hierarchy and Role

- **Base Type**: `Rectangle`
  - Floating modal dialog (`width: 44`, `height: 9`) centered in its parent window, styled with double yellow borders.

## 4. Properties

| Property | Type | Default | Required | Description |
|----------|------|---------|----------|-------------|
| `title` | `string` | `"CONFIRM ACTION"` | No | Title text displayed in the header banner. |
| `message` | `string` | `"Are you sure you want to proceed?"` | No | Confirmation body text shown to the user. |

## 5. Signals

#### `confirmed()`

- **Parameters**: None
- **Trigger**: Emitted when the user confirms the action (typically pressing the 'Y' key while the dialog is active).
- **Expected Handler**: Connected code executes the requested action and hides the dialog.

#### `cancelled()`

- **Parameters**: None
- **Trigger**: Emitted when the user cancels the action (typically pressing the 'N' key while the dialog is active).
- **Expected Handler**: Connected code aborts the requested action and hides the dialog.

## 6. Methods

*This component declares no custom JavaScript functions.*

## 7. Inter-Component Interactions

- **Parent Overlay**: Anchored in the center of its parent component (`anchors.centerIn: parent`).
- **Theme Integration**: Uses `HmiTheme.textWarning` and `HmiTheme.textPrimary` for warning banner styling.

## 8. Usage Example

```qml
import QtQml
import Qtui
import IndustrialPlcHmi 1.0

ConfirmDialog {
    id: dialog
    title: "RESTART CONTROLLER"
    message: "Confirm soft reboot of PLC STN#01?"
    visible: true
    onConfirmed: {
        console.log("Rebooting...")
        visible = false
    }
    onCancelled: {
        visible = false
    }
}
```
