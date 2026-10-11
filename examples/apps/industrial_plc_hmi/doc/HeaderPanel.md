# HeaderPanel

## 1. Component Overview

The `HeaderPanel` component renders the top status bar in the Industrial PLC HMI interface. It displays station information (e.g., station ID) alongside a machine state badge (`[■ STANDBY]`, `[● RUNNING]`, `[⚙ MAINT]`, `[▲ FAULT!]`) with dynamic border and background color changes according to system state.

Developers use `HeaderPanel` at the top of an HMI view hierarchy to give machine operators immediate status awareness.

## 2. Project Structure and Dependencies

- **Import Dependencies**:
  - `QtQml` (version 2.15)
  - `Tui` (version 1.0)
  - `IndustrialPlcHmi` (version 1.0)
- **File Location**: `examples/industrial_plc_hmi/HeaderPanel.qml`

## 3. Component Hierarchy and Role

- **Base Type**: `Rectangle`
  - Fixed-height top bar (`height: 3`) with double border styling.
  - Composes title text on the left and a state indicator badge on the right.

## 4. Properties

| Property | Type | Default | Required | Description |
|----------|------|---------|----------|-------------|
| `machineState` | `string` | `"idle"` | No | Current operational state of the PLC machine. Valid values: `"idle"`, `"processing"`, `"maintenance"`, `"error"`. |
| `stationId` | `string` | `"STN#42"` | No | Identifier for the PLC station. |

## 5. Signals

*This component declares no custom signals.*

## 6. Methods

*This component declares no custom JavaScript functions.*

## 7. Inter-Component Interactions

- **Theme Integration**: Reads `HmiTheme` border colors (`borderError`, `borderFocus`, `statusIdle`), background colors (`headerBgError`, `headerBg`), and text colors to style the panel and state badge based on `machineState`.

## 8. Usage Example

```qml
import QtQml
import Qamui
import IndustrialPlcHmi 1.0

HeaderPanel {
    width: parent.width
    machineState: "processing"
    stationId: "STN#01"
}
```
