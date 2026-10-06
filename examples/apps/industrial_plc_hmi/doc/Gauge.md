# Gauge

## 1. Component Overview

The `Gauge` component displays a telemetry metric along with an animated ASCII progress bar. It includes warning and danger thresholding, dynamically changing color (normal, warning, or danger) as values cross configured limits.

Developers use `Gauge` in monitoring dashboards to display live sensor values like pressure, temperature, flow rate, or motor speed.

## 2. Project Structure and Dependencies

- **Import Dependencies**:
  - `QtQml` (version 2.15)
  - `Tui` (version 1.0)
  - `IndustrialPlcHmi` (version 1.0)
- **File Location**: `examples/industrial_plc_hmi/Gauge.qml`

## 3. Component Hierarchy and Role

- **Base Type**: `Column`
  - Vertically stacks two `Text` elements: the label with formatted numerical value, and the progress bar.

## 4. Properties

| Property | Type | Default | Required | Description |
|----------|------|---------|----------|-------------|
| `label` | `string` | `""` | No | Description label for the metric (e.g. `"Pressure"`). |
| `value` | `double` | `0` | No | Current telemetry reading. |
| `unit` | `string` | `""` | No | Measurement unit string (e.g. `"PSI"`, `"°C"`). |
| `minVal` | `double` | `0` | No | Minimum expected gauge value. |
| `maxVal` | `double` | `100` | No | Maximum expected gauge value. |
| `warningVal` | `double` | `75` | No | Value threshold at which warning styling activates. |
| `dangerVal` | `double` | `85` | No | Value threshold at which danger/error styling activates. |
| `barWidth` | `int` | `Math.max(8, root.width - 2)` | No | Width of the text-based progress bar in characters. |
| `decimals` | `int` | `1` | No | Number of decimal places to format for the value display. |

## 5. Signals

*This component declares no custom signals.*

## 6. Methods

*This component declares no custom JavaScript functions.*

## 7. Inter-Component Interactions

- **Theme Integration**: Calls `HmiTheme.makeProgressBar()` to generate the block progress bar string, and uses `HmiTheme` text/status colors for warning and danger levels.

## 8. Usage Example

```qml
import QtQml
import Qtui
import IndustrialPlcHmi 1.0

Gauge {
    label: "Hydraulic Pressure"
    value: 82.5
    unit: "BAR"
    minVal: 0
    maxVal: 100
    warningVal: 70
    dangerVal: 80
}
```
