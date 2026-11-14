# WebAssembly support, included under if(EMSCRIPTEN) by both the root
# CMakeLists.txt and the installed QmlTuiConfig.cmake package.
#
# Qt's executable finalizer links every wasm app with MODULARIZE, EXPORT_NAME
# (<target>_entry), embind, the exported runtime methods, and memory settings.
# However, Qt's default wasm_shell.html mounts a GUI <canvas> and qtloader.js,
# which cannot host a QCoreApplication terminal app. qmltui_wasm_app() disables
# Qt's GUI shell and writes a standalone xterm.js terminal host page (<target>.html)
# next to <target>.js and <target>.wasm.

set(QMLTUI_WASM_INITIAL_MEMORY "" CACHE STRING
    "Override Qt's default -sINITIAL_MEMORY (50MB) for QmlTui apps, e.g. 96MB")

# Configures a WebAssembly executable target as a QmlTui terminal application.
# Call in the directory that created the target so Qt's deferred finalizer sees
# NO_WASM_DEFAULT_FILES.
#
# Usage:
#   qmltui_wasm_app(<target> [OUTPUT_DIRECTORY <dir>])
function(qmltui_wasm_app target)
    set(_qmltui_wasm_shell_template [=[<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>@QMLTUI_WASM_APP_NAME@</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Fira+Code:wght@400;700&family=JetBrains+Mono:ital,wght@0,400;0,700;1,400&display=swap">
<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/@xterm/xterm@5.5.0/css/xterm.min.css">
<style>
  :root {
    --bg: #0d1117;
    --surface: #161b22;
    --border: #30363d;
    --brand: #2cde85;
    --brand-bright: #00ff66;
    --text: #c9d1d9;
    --muted: #8b949e;
  }
  html, body { height: 100%; margin: 0; background: var(--bg); color: var(--text); font: 14px system-ui, -apple-system, sans-serif; }
  body { display: grid; grid-template-rows: 2.5rem minmax(0, 1fr); height: 100vh; overflow: hidden; }
  header { display: flex; gap: .6rem; align-items: center; padding: 0 .9rem; background: var(--surface); border-bottom: 1px solid var(--border); }
  header h1 { font-size: .9rem; margin: 0; font-weight: 700; color: var(--brand); font-family: "JetBrains Mono", ui-monospace, monospace; }
  #status { margin-left: auto; color: var(--muted); font-size: .78rem; font-family: "JetBrains Mono", ui-monospace, monospace; }
  button { background: var(--bg); color: var(--text); border: 1px solid var(--border); padding: .2rem .55rem; border-radius: 5px; font-size: .78rem; cursor: pointer; }
  button:hover { border-color: var(--brand); color: var(--brand-bright); }
  #terminal { min-height: 0; min-width: 0; height: 100%; overflow: hidden; padding: .4rem; box-sizing: border-box; background: var(--bg); }
  #terminal .xterm { height: 100%; width: 100%; overflow: hidden; }
  #terminal .xterm-viewport { overflow-x: hidden !important; overflow-y: auto !important; scrollbar-width: thin; scrollbar-color: var(--border) transparent; }
</style>
</head>
<body>
<header>
  <h1 id="title">@QMLTUI_WASM_APP_NAME@</h1>
  <span id="status">loading…</span>
  <button id="restart" type="button" title="Restart application">↻ Restart</button>
</header>
<div id="terminal"></div>
<script src="https://cdn.jsdelivr.net/npm/@xterm/xterm@5.5.0/lib/xterm.min.js"></script>
<script src="https://cdn.jsdelivr.net/npm/@xterm/addon-fit@0.10.0/lib/addon-fit.min.js"></script>
<script src="https://cdn.jsdelivr.net/npm/@xterm/addon-unicode-graphemes@0.4.0/lib/addon-unicode-graphemes.min.js"></script>
<script src="@QMLTUI_WASM_APP_NAME@.js"></script>
<script>
(() => {
  const EXPORT_NAME = '@QMLTUI_WASM_EXPORT_NAME@';
  const titleEl = document.getElementById('title');
  const statusEl = document.getElementById('status');
  const restartBtn = document.getElementById('restart');
  const terminalEl = document.getElementById('terminal');
  restartBtn.onclick = () => location.reload();

  const THEME = {
    background: '#0d1117', foreground: '#c9d1d9', cursor: '#00ff66', selectionBackground: '#264f78',
    black: '#0d1117', red: '#ff7b72', green: '#2cde85', yellow: '#f9c74f',
    blue: '#58a6ff', magenta: '#d2a8ff', cyan: '#39c5cf', white: '#c9d1d9',
    brightBlack: '#484f58', brightRed: '#ffa198', brightGreen: '#00ff66', brightYellow: '#fbbf24',
    brightBlue: '#79c0ff', brightMagenta: '#e2c5ff', brightCyan: '#56d4dd', brightWhite: '#f0f6fc',
  };
  const ANSI_KEYS = [
    'black', 'red', 'green', 'yellow', 'blue', 'magenta', 'cyan', 'white',
    'brightBlack', 'brightRed', 'brightGreen', 'brightYellow', 'brightBlue', 'brightMagenta', 'brightCyan', 'brightWhite',
  ];

  const encoder = new TextEncoder();
  let instance = null;
  let exited = false;

  const term = new Terminal({
    allowProposedApi: true,
    cursorStyle: 'block',
    cursorBlink: false,
    scrollback: 1000,
    fontSize: 15,
    lineHeight: 1.0,
    fontFamily: '"JetBrains Mono", "Fira Code", ui-monospace, monospace',
    theme: THEME,
  });
  if (window.UnicodeGraphemesAddon) {
    term.loadAddon(new UnicodeGraphemesAddon.UnicodeGraphemesAddon());
    term.unicode.activeVersion = '15-graphemes';
  }
  term.parser.registerOscHandler(52, (data) => {
    const idx = data.indexOf(';');
    if (idx < 0) return true;
    try {
      const bin = atob(data.slice(idx + 1));
      const bytes = Uint8Array.from(bin, (c) => c.charCodeAt(0));
      navigator.clipboard?.writeText(new TextDecoder().decode(bytes)).catch(() => {});
    } catch {}
    return true;
  });
  term.parser.registerOscHandler(10, (data) => {
    if (data.trim() === '?' && instance && !exited) {
      instance.qmltuiFeedInput(encoder.encode(`\x1b]10;${THEME.foreground}\x1b\\`));
    }
    return true;
  });
  term.parser.registerOscHandler(11, (data) => {
    if (data.trim() === '?' && instance && !exited) {
      instance.qmltuiFeedInput(encoder.encode(`\x1b]11;${THEME.background}\x1b\\`));
    }
    return true;
  });
  term.parser.registerOscHandler(4, (data) => {
    if (!instance || exited) return true;
    const parts = data.split(';');
    let replies = '';
    for (let i = 0; i + 1 < parts.length; i += 2) {
      const idx = Number(parts[i]);
      if (parts[i + 1].trim() === '?' && idx >= 0 && idx < ANSI_KEYS.length) {
        replies += `\x1b]4;${idx};${THEME[ANSI_KEYS[idx]]}\x1b\\`;
      }
    }
    if (replies) instance.qmltuiFeedInput(encoder.encode(replies));
    return true;
  });

  const fit = new FitAddon.FitAddon();
  term.loadAddon(fit);
  term.open(terminalEl);
  fit.fit();
  new ResizeObserver(() => fit.fit()).observe(terminalEl);
  terminalEl.addEventListener('mousedown', () => term.focus());

  if (document.fonts && document.fonts.load) {
    Promise.all([
      document.fonts.load('400 15px "JetBrains Mono"'),
      document.fonts.load('700 15px "JetBrains Mono"'),
    ]).then(() => {
      term.clearTextureAtlas?.();
      if (term.rows > 0) term.refresh(0, term.rows - 1);
      fit.fit();
    }).catch(() => {});
  }

  function finished(msg) {
    if (exited) return;
    exited = true;
    term.reset();
    statusEl.textContent = msg;
  }

  async function boot() {
    const factory = window[EXPORT_NAME];
    if (typeof factory !== 'function') {
      statusEl.textContent = `error: ${EXPORT_NAME} not found`;
      return;
    }
    instance = await factory({
      noInitialRun: true,
      qmltuiOnOutput: (bytes) => term.write(bytes),
      print: (t) => console.log(t),
      printErr: (t) => console.warn(t),
      onExit: (code) => finished(`exited (${code ?? 0}) — press Enter or click Restart`),
      onAbort: () => finished('exited — press Enter or click Restart'),
    });
    fit.fit();
    instance.qmltuiResize(term.cols, term.rows);
    term.onData((s) => {
      if (exited) {
        if (s === '\r' || s === '\n' || s === 'r' || s === 'R') location.reload();
        return;
      }
      try { instance.qmltuiFeedInput(encoder.encode(s)); } catch (e) { if (!exited) console.error(e); }
    });
    term.onBinary((s) => {
      if (exited) return;
      try { instance.qmltuiFeedInput(Uint8Array.from(s, (c) => c.charCodeAt(0))); } catch (e) { if (!exited) console.error(e); }
    });
    term.onResize(({ cols, rows }) => {
      if (!exited && instance) {
        try { instance.qmltuiResize(cols, rows); } catch {}
      }
    });
    term.onTitleChange((title) => {
      if (title) {
        document.title = title;
        titleEl.textContent = title;
      }
    });

    const params = new URLSearchParams(location.search);
    const args = params.getAll('arg');

    try {
      instance.callMain(args);
    } catch (e) {
      if (e !== 'unwind' && !exited) throw e;
    }
    if (!exited) statusEl.textContent = 'running';
    term.focus();
  }

  boot().catch((err) => {
    console.error(err);
    statusEl.textContent = 'error starting module (see console)';
  });
})();
</script>
</body>
</html>
]=])

    cmake_parse_arguments(ARG "" "OUTPUT_DIRECTORY" "" ${ARGN})

    set_target_properties(${target} PROPERTIES NO_WASM_DEFAULT_FILES ON)
    if(ARG_OUTPUT_DIRECTORY)
        set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${ARG_OUTPUT_DIRECTORY}")
    endif()
    if(QMLTUI_WASM_INITIAL_MEMORY)
        set_target_properties(${target} PROPERTIES
            QT_WASM_INITIAL_MEMORY "${QMLTUI_WASM_INITIAL_MEMORY}")
    endif()

    get_target_property(_output_name "${target}" OUTPUT_NAME)
    if(NOT _output_name)
        set(_output_name "${target}")
    endif()

    get_target_property(_export_name "${target}" QT_WASM_EXPORT_NAME)
    if(NOT _export_name)
        if("${target}" MATCHES "^[0-9]")
            set(_export_name "_${target}")
        else()
            set(_export_name "${target}")
        endif()
        string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" _export_name "${_export_name}")
        string(APPEND _export_name "_entry")
    endif()

    get_target_property(_output_dir "${target}" RUNTIME_OUTPUT_DIRECTORY)
    if(NOT _output_dir)
        set(_output_dir "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    file(MAKE_DIRECTORY "${_output_dir}")
    string(REPLACE "@QMLTUI_WASM_APP_NAME@" "${_output_name}" _qmltui_html "${_qmltui_wasm_shell_template}")
    string(REPLACE "@QMLTUI_WASM_EXPORT_NAME@" "${_export_name}" _qmltui_html "${_qmltui_html}")
    file(WRITE "${_output_dir}/${_output_name}.html" "${_qmltui_html}")

    if(CMAKE_CONFIGURATION_TYPES)
        add_custom_command(
            TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_output_dir}/${_output_name}.html"
                "${_output_dir}/$<CONFIG>/${_output_name}.html"
        )
    endif()
endfunction()
