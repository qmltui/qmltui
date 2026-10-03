# HmiTheme

## 1. Component Overview

The `HmiTheme` component is a singleton configuration object that defines the visual design system for the Industrial PLC HMI application. It centralizes semantic color palettes (backgrounds, borders, status indicators, and text colors) and provides utility functions like progress bar formatting for terminal user interfaces.

Developers use `HmiTheme` across the PLC HMI application to enforce a consistent dark theme visual style and handle dynamic text-based gauge formatting.

## 2. Project Structure and Dependencies

- **Import Dependencies**:
  - `QtQml` (version 2.15) — provides core `QtObject` features.
- **File Location**: `examples/industrial_plc_hmi/HmiTheme.qml`
- **Module**: `IndustrialPlcHmi` (version 1.0) declared as a `pragma Singleton`.

## 3. Component Hierarchy and Role

- **Base Type**: `QtObject`
  - Non-visual component serving purely as a data and logic provider.
  - Implements the Singleton pattern (`pragma Singleton`), accessible globally via `HmiTheme`.

## 4. Properties

| Property | Type | Default | Required | Description |
|----------|------|---------|----------|-------------|
| `bgIdle` | `string` | `"#111111"` | No | Background color string when system is in IDLE mode. |
| `bgProcessing` | `string` | `"#0a1212"` | No | Background color string when system is in PROCESSING mode. |
| `bgMaintenance` | `string` | `"#14140e"` | No | Background color string when system is in MAINTENANCE mode. |
| `bgError` | `string` | `"#2a0505"` | No | Background color string when system is in ERROR state. |
| `bgErrorAlt` | `string` | `"#120202"` | No | Alternative background color string for error dialogs. |
| `headerBg` | `string` | `"#1c1e24"` | No | Background color for header panels. |
| `headerBgError` | `string` | `"#3b0808"` | No | Header background color during alarm/error state. |
| `footerBg` | `string` | `"#16181d"` | No | Footer background color. |
| `footerBgError` | `string` | `"#3a0505"` | No | Footer background color during error state. |
| `cardBg` | `string` | `"#181a1f"` | No | Default card/panel container background color. |
| `cardBgAlt` | `string` | `"#21252b"` | No | Alternate card container background color. |
| `borderNormal` | `string` | `"#444444"` | No | Default border color. |
| `borderFocus` | `string` | `"#e5c07b"` | No | Border color when item has keyboard focus. |
| `borderError` | `string` | `"#e06c75"` | No | Border color for error or warning states. |
| `borderActive` | `string` | `"#61afef"` | No | Border color for active elements. |
| `statusIdle` | `string` | `"#98c379"` | No | Color indicator for IDLE status (green). |
| `statusProcessing` | `string` | `"#56b6c2"` | No | Color indicator for PROCESSING status (cyan). |
| `statusMaintenance` | `string` | `"#e5c07b"` | No | Color indicator for MAINTENANCE status (yellow). |
| `statusError` | `string` | `"#e06c75"` | No | Color indicator for ERROR status (red). |
| `textPrimary` | `string` | `"#ffffff"` | No | Primary text color (white). |
| `textSecondary` | `string` | `"#abb2bf"` | No | Secondary body text color. |
| `textDim` | `string` | `"#5c6370"` | No | Muted/dim text color. |
| `textHighlight` | `string` | `"#61afef"` | No | Highlighted text color. |
| `textWarning` | `string` | `"#e5c07b"` | No | Warning text color. |
| `textDanger` | `string` | `"#e06c75"` | No | Danger/error text color. |
| `textSuccess` | `string` | `"#98c379"` | No | Success text color. |
| `textAccent` | `string` | `"#c678dd"` | No | Accent text color. |

## 5. Signals

*This component declares no custom signals.*

## 6. Methods

#### `makeProgressBar(double value, double minVal, double maxVal, int width) : string`

- **Parameters**:
  - `value`: Current numerical value.
  - `minVal`: Minimum value of the range.
  - `maxVal`: Maximum value of the range.
  - `width`: Number of characters wide the progress bar should be.
- **Description**: Calculates the percentage completion between `minVal` and `maxVal`, and constructs a text progress bar string composed of filled block characters (`█`) and empty shade characters (`░`).
- **Return Type**: `string`

## 7. Inter-Component Interactions

- **Global Consumer Bindings**:
  - `Gauge.qml`, `ActuatorLed.qml`, `HeaderPanel.qml`, `NavTabBar.qml`, `AlarmsView.qml`, `DiagnosticsView.qml`, `SettingsView.qml`, `ConfirmDialog.qml`, and `Main.qml` read properties and invoke `makeProgressBar()` directly from `HmiTheme`.

## 8. Usage Example

```qml
import QtQml 2.15
import Qtui
import IndustrialPlcHmi 1.0

Text {
    text: HmiTheme.makeProgressBar(75, 0, 100, 20)
    color: HmiTheme.statusProcessing
}
```
