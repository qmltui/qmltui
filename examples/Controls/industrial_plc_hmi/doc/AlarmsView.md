# AlarmsView

## 1. Component Overview

The `AlarmsView` component displays an alarm log and fault history table in the PLC HMI interface. It lists up to 10 recent system events, formatted with severity badges (`[CRITICAL]` in red vs `[INFO]` in green), event timestamps/messages, and a summary count of total recorded faults.

Developers use `AlarmsView` as the main audit log screen for machine operators.

## 2. Project Structure and Dependencies

- **Import Dependencies**:
  - `QtQml` (version 2.15)
  - `Tui` (version 1.0)
  - `IndustrialPlcHmi` (version 1.0)
- **File Location**: `examples/industrial_plc_hmi/AlarmsView.qml`

## 3. Component Hierarchy and Role

- **Base Type**: `Rectangle`
  - Container (`anchors.fill: parent`) hosting a header title, table column header, and a `Repeater` of log row delegates.

## 4. Properties

| Property | Type | Default | Required | Description |
|----------|------|---------|----------|-------------|
| `logs` | `var` | `[]` | No | Array of event log strings. |
| `errorCount` | `int` | `0` | No | Total count of critical faults currently logged. |

## 5. Signals

*This component declares no custom signals.*

## 6. Methods

*This component declares no custom JavaScript functions.*

## 7. Inter-Component Interactions

- **Theme Integration**: Uses `HmiTheme.textDanger` for critical alarms, `HmiTheme.textSuccess` for info logs, and `HmiTheme.textHighlight` for table titles.

## 8. Usage Example

```qml
import QtQml 2.15
import Terminal 1.0
import IndustrialPlcHmi 1.0

AlarmsView {
    logs: [
        "ALARM: Thermal overload tripped on DI-02",
        "System state transitioned to IDLE"
    ]
    errorCount: 1
}
```
