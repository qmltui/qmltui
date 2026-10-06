# Implementation Plan 05: Tiered Live Previews (Asciinema Casts & WebAssembly Playground)

## 1. Goal Description

Deliver an unmatched **"Try Before You Install"** experience using a 2-tier architecture:
- **Tier 1 (Instant Previews everywhere)**: Ultra-lightweight vector terminal sessions via `<AsciinemaPlayer.svelte>` (30 KB payload, <50ms load, truecolor ANSI, selectable text).
- **Tier 2 (The WASM Playground)**: Full interactive in-browser execution of Qtui apps compiled to WebAssembly via Emscripten and hooked directly to `xterm.js`.

---

## 2. The 2-Tier Architecture Comparison

| Feature | Tier 1: Vector Casts (`.cast`) | Tier 2: WebAssembly Playground (`.wasm`) |
| :--- | :--- | :--- |
| **Location** | Landing page hero, all 31 component pages, 17 app cards | Dedicated `/playground` page or "Try Live" modal |
| **Download Size** | **15 KB – 40 KB** | **~4 MB – 6 MB** (compressed) |
| **Load Time** | **< 50 ms (Instant)** | **~1.5 – 3.0 seconds** |
| **Interactivity** | Play, pause, seek, speed toggle (1x, 2x) | **100% Interactive** (Keyboard navigation, typing, mouse clicks) |
| **Copyable Text** | Yes (Real terminal buffer) | Yes (via `xterm.js` buffer selection) |
| **Mobile UX** | Flawless (no keyboard required) | Requires soft keyboard for special keys (`Esc`, `Tab`) |

---

## 3. Tier 1: Svelte Asciinema Player Component

Wrap `asciinema-player` in an authentic terminal window:

```svelte
<!-- src/lib/components/AsciinemaPlayer.svelte -->
<script lang="ts">
  import { onMount } from "svelte";
  import TerminalWindow from "./TerminalWindow.svelte";

  export let src: string;
  export let title: string = "Terminal Preview";
  export let rows: number = 24;
  export let cols: number = 80;
  export let autoPlay: boolean = true;

  let playerContainer: HTMLDivElement;

  onMount(async () => {
    // Dynamic import to prevent SSR issues in SvelteKit
    const AsciinemaPlayer = await import("asciinema-player");
    AsciinemaPlayer.create(src, playerContainer, {
      autoPlay,
      loop: true,
      theme: "monokai",
      rows,
      cols,
      fit: "both"
    });
  });
</script>

<TerminalWindow {title}>
  <div bind:this={playerContainer} class="w-full"></div>
</TerminalWindow>
```

---

## 4. Tier 2: WebAssembly (WASM) Playground Architecture

Qtui is **uniquely suited for WebAssembly** because it depends exclusively on `QtCore` and `QtQml`, with **zero dependency on `QtGui`, OpenGL, WebGL, or the heavy Qt Quick Scene Graph**.

### A. How Qtui Connects to `xterm.js`

```
┌─────────────────────────────────────────────────────────────┐
│                       Browser Window                        │
│                                                             │
│   ┌─────────────────────────────────────────────────────┐   │
│   │               xterm.js Terminal Canvas              │   │
│   └──────────────▲───────────────────────┬──────────────┘   │
│                  │ stdout (ANSI strings) │ stdin (bytes)    │
│                  │                       ▼                  │
│   ┌──────────────┴──────────────────────────────────────┐   │
│   │           Emscripten Virtual POSIX Runtime          │   │
│   └──────────────────────▲──────────────────────────────┘   │
│                          │                                  │
│   ┌──────────────────────┴──────────────────────────────┐   │
│   │        Qtui C++20 Executable (QtCore + QtQml)       │   │
│   │                                                     │   │
│   │   • TerminalEngine::render() -> TerminalDevice      │   │
│   │   • InputDecoder::decode()   <- raw stdin bytes     │   │
│   │   • QQmlEngine & reactive QML bindings              │   │
│   └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### B. CMake WebAssembly Configuration (`cmake/QtuiWasm.cmake`)
```cmake
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_PTHREADS=0 -s ALLOW_MEMORY_GROWTH=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORTED_RUNTIME_METHODS=['FS','callMain']")
endif()
```

### C. Svelte WebAssembly Terminal Component (`WasmTerminal.svelte`)
```svelte
<!-- src/lib/components/WasmTerminal.svelte -->
<script lang="ts">
  import { onMount } from "svelte";
  import { Terminal } from "xterm";
  import { FitAddon } from "xterm-addon-fit";
  import "xterm/css/xterm.css";

  export let appName: string = "btop";

  let terminalContainer: HTMLDivElement;
  let term: Terminal;
  let fitAddon: FitAddon;

  onMount(async () => {
    term = new Terminal({
      cursorBlink: true,
      theme: { background: "#0d1117", foreground: "#c9d1d9" }
    });
    fitAddon = new FitAddon();
    term.loadAddon(fitAddon);
    term.open(terminalContainer);
    fitAddon.fit();

    // Dynamically load the Emscripten WASM module
    const Module = {
      print: (text: string) => term.writeln(text),
      printErr: (text: string) => console.error(text),
    };

    term.onData((data) => {
      // Forward keystrokes directly into Qtui's stdin
      if (Module.writeToStdin) {
        Module.writeToStdin(data);
      }
    });

    // Load compiled WASM script
    const script = document.createElement("script");
    script.src = `/wasm/qtui_${appName}.js`;
    document.body.appendChild(script);
  });
</script>

<div class="rounded-lg border border-gray-800 bg-[#0d1117] p-4">
  <div bind:this={terminalContainer} class="h-[600px] w-full"></div>
</div>
```

---

## 5. Summary of Deliverables

1. **`.cast` Recording Pipeline**:
   - Automated generation of `.cast` session files for `industrial_plc_hmi`, `btop`, `service_monitor`, `snake_game`, and `markdown_viewer`.
2. **Svelte Components**:
   - `<AsciinemaPlayer.svelte>` embedded on docs pages.
   - `<WasmTerminal.svelte>` on `/playground`.
3. **Emscripten Build Guide**:
   - Documented build steps for compiling Qtui to WebAssembly via `emcmake cmake`.
