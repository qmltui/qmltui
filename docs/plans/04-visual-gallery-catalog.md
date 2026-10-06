# Implementation Plan 04: Visual Showcase Gallery & 31-Component Catalog

## 1. Goal Description

Create the **visual-first adoption engine** for Qtui:
1. **Showcase Gallery**: An interactive, filterable catalog of all 17 full applications (now in `qtui-apps`).
2. **Component Catalog**: A comprehensive visual gallery of all 31 primitive controls (in `examples/components/`).
3. **Automated Screenshot Pipeline**: Automated generation of pixel-perfect PNGs and GIFs on release.

---

## 2. The 17 Showcase Applications (Categorized)

| Category | Application | Key Highlights | Run Command |
| :--- | :--- | :--- | :--- |
| **Industrial & IoT** | `industrial_plc_hmi` | Real-time gauges, alarm table, actuator LEDs, tabs | `./build/industrial_plc_hmi/qtui_industrial_plc_hmi` |
| | `din_gateway` | Modbus/Ethernet DIN-rail gateway telemetry | `./build/din_gateway/qtui_din_gateway` |
| | `device_cockpit` | Embedded Linux cockpit, DBus network telemetry | `./build/device_cockpit/qtui_device_cockpit` |
| | `starter_template` | Boot-to-Qtui setup, Wi-Fi scanner, signal meter | `./build/starter_template/qtui_starter_template` |
| **Cloud & DevOps** | `btop` | CPU/Memory graph, process trees, disk metrics | `./build/btop/qtui_btop` |
| | `container_fleet` | Docker/Kubernetes container fleet monitor | `./build/container_fleet/qtui_container_fleet` |
| | `service_monitor` | System services health, latency column format | `./build/service_monitor/qtui_service_monitor` |
| | `process_monitor` | Real-time task manager with sorting & filtering | `./build/process_monitor/qtui_process_monitor` |
| **Productivity** | `markdown_viewer` | KDE md4qt AST parser, syntax highlighting, image preview | `./build/markdown_viewer/qtui_markdown_viewer` |
| | `file_manager` | Dual-pane file manager, Unix PTY shell integration | `./build/file_manager/qtui_file_manager` |
| | `pos_cashier` | Point of Sale retail terminal, cart & checkout | `./build/pos_cashier/qtui_pos_cashier` |
| | `plotting_dashboard` | ASCII data visualization, line graphs & scatter plots | `./build/plotting_dashboard/qtui_plotting_dashboard` |
| **Games** | `spaceship_game` | High-speed terminal game, particle animations | `./build/spaceship_game/qtui_spaceship_game` |
| | `twozero48` | 2048 puzzle game with tile merge animations | `./build/twozero48/qtui_twozero48` |
| | `snake_game` | Classic snake game with AI autopilot mode | `./build/snake_game/qtui_snake_game` |
| **AI & Modern TUIs** | `agent_demo` | Real-time streaming LLM output, badge status | `./build/agent_demo/qtui_agent_demo` |
| | `system_dashboard` | Unified host dashboard with gauges & bars | `./build/system_dashboard/qtui_system_dashboard` |

---

## 3. The 31 Component Catalog (Categorized)

Each component in [`examples/components/`](file:///home/yosri/qtui/examples/components) receives a dedicated visual reference card:

### A. Data Views & Virtualization
- `TableView` (Virtual multi-column table with custom cell delegates)
- `TreeView` (Hierarchical node tree with expandable branches)
- `ListView` (Single-column list with selection and navigation)
- `GridView` (2D cell grid layout)
- `ScrollView` (Cell-based clipping and scrolling container)
- `Canvas` (Low-level cell drawing surface)

### B. Interactive Inputs & Controls
- `Button` (Key and mouse clickable action button)
- `CheckBox` (Boolean toggle with label)
- `RadioButton` (Mutual exclusion option group)
- `Switch` (Sliding binary toggle switch)
- `Slider` (Numeric drag and arrow step slider)
- `TextInput` (Single-line editable text buffer with cursor)
- `TextArea` (Multi-line editable text area)

### C. Visual Display & Metrics
- `Text` (ANSI colorized and styled text label)
- `BigText` (ASCII-art enlarged block font renderer)
- `Paragraph` (Word-wrapped rich text block)
- `Gauge` (Circular and linear metric percentage gauge)
- `Sparkline` (Compact mini-trendline chart)
- `StatusBadge` (Color-coded semantic state badge)
- `Spinner` (Animated ASCII loading spinner)
- `ProgressBar` (Horizontal task completion bar)
- `Rectangle` (Colored terminal box with custom borders)
- `Image` (ANSI half-block and Sixel terminal image renderer)

### D. Overlays & Navigation
- `Dialog` (Centered modal dialog box with action buttons)
- `Popup` (Contextual floating menu overlay)
- `Toast` (Non-blocking floating notification banner)
- `ToolTip` (Hover-activated hint bubble)
- `TabHeader` (Function-key and arrow-key navigation tab bar)
- `Panel` (Bordered container panel with title header)
- `Loader` (Dynamic component instantiation)
- `TerminalView` (Embedded Unix PTY terminal emulator host)

---

## 4. Svelte Terminal Window Component (`TerminalWindow.svelte`)

An authentic macOS/Linux styled window wrapper with window controls, active title bar, and copy-code button:

```svelte
<!-- src/lib/components/TerminalWindow.svelte -->
<script lang="ts">
  export let title: string = "qtui terminal";
  export let code: string = "";
</script>

<div class="rounded-lg border border-gray-800 bg-[#0d1117] shadow-2xl overflow-hidden my-6">
  <div class="flex items-center justify-between px-4 py-2 bg-[#161b22] border-b border-gray-800">
    <div class="flex items-center space-x-2">
      <span class="w-3 h-3 rounded-full bg-[#ff5f56]"></span>
      <span class="w-3 h-3 rounded-full bg-[#ffbd2e]"></span>
      <span class="w-3 h-3 rounded-full bg-[#27c93f]"></span>
    </div>
    <span class="text-xs font-mono text-gray-400 font-semibold">{title}</span>
    <div class="w-12"></div>
  </div>
  <div class="p-4 font-mono text-sm">
    <slot />
  </div>
</div>
```

---

## 5. Automated Asset Generation Script (`scripts/generate_screenshots.sh`)

Leverage Qtui's built-in `TestRunner::captureImage()` to generate high-resolution PNGs without manual screen captures:

```bash
#!/usr/bin/env bash
set -euo pipefail

# Output directory in Svelte static assets
OUTPUT_DIR="docs/static/images"
mkdir -p "${OUTPUT_DIR}/apps" "${OUTPUT_DIR}/components"

# Headless snapshot for each showcase app
for app in btop industrial_plc_hmi container_fleet file_manager pos_cashier; do
    echo "Capturing snapshot for ${app}..."
    QTUI_INSPECT=1 QTUI_TEST_ARTIFACT_DIR="${OUTPUT_DIR}/apps" \
        "./build/${app}/qtui_${app}" || true
done
```
