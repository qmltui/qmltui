# DiagnosticsView

## 1. Component Overview

The `DiagnosticsView` component provides a hardware status screen for the PLC system. It displays digital inputs (DI), digital outputs (DO), analog 24V bus voltages, CPU temperature telemetry, and network communication stats (PROFINET / Modbus packet counts).

Developers use `DiagnosticsView` for low-level system troubleshooting and I/O checkout.

## 2. Project Structure and Dependencies

- **Import Dependencies**:
  - `QtQml` (version 2.15)
  - `Tui` (version 1.0)
  - `IndustrialPlcHmi` (version 1.0)
- **File Location**: `examples/industrial_plc_hmi/DiagnosticsView.qml`

## 3. Component Hierarchy and Role

- **Base Type**: `Rectangle`
  - Split-view container housing a two-column layout: Digital I/O on the left, Power & Communications health on the right.

## 4. Properties

| Property | Type | Default | Required | Description |
|----------|------|---------|----------|-------------|
| `cycles` | `int` | `0` | No | Total PLC cycle count (used to calculate communication packet metrics). |
| `valveA` | `bool` | `false` | No | State of Digital Output DO-01 (Valve A coil). |
| `valveB` | `bool` | `false` | No | State of Digital Output DO-02 (Valve B coil). |
| `machineState` | `string` | `"idle"` | No | Current operational machine state (`"idle"`, `"processing"`, `"maintenance"`, `"error"`). |

## 5. Signals

*This component declares no custom signals.*

## 6. Methods

*This component declares no custom JavaScript functions.*

## 7. Inter-Component Interactions

- **Theme Integration**: Uses `HmiTheme.textSuccess`, `HmiTheme.textDanger`, and `HmiTheme.textSecondary` for live status indicators.

## 8. Usage Example

```qml
import QtQml
import Qtui
import IndustrialPlcHmi 1.0

DiagnosticsView {
    cycles: 1420
    valveA: true
    valveB: false
    machineState: "processing"
}
```
