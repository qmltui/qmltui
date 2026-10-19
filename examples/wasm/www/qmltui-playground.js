// The gallery page: CodeMirror on the left, the gallery running in xterm.js on
// the right. Loaded as an ES module by index.html. CodeMirror 6 comes from
// esm.sh through the import map in index.html (exact versions, single instance
// per package).
//
// Every demo is an Item-rooted QML component under examples/. The gallery
// launcher (gallery.qml) loads the active demo directly using a Loader and
// names the open one in its terminal title ("QmlTui Gallery · Button.qml").
// When that arrives the editor shows the file; each edit is compiled first
// (qmltuiCheckQml), written to /playground/examples/<file> (qmltuiWriteFile),
// and the gallery re-runs with the same tile open (qmltuiGalleryPage). Escape
// in the gallery empties the editor again.
import { EditorView, basicSetup } from 'codemirror';
import { Compartment } from '@codemirror/state';
import { javascript } from '@codemirror/lang-javascript';
import { oneDark } from '@codemirror/theme-one-dark';
import { keymap } from '@codemirror/view';
import { indentWithTab } from '@codemirror/commands';
import { autocompletion } from '@codemirror/autocomplete';
import { createQmlCompletion } from './qmltui-completion.js';

const $ = (id) => document.getElementById(id);
const status = $('status');
const problems = $('problems');
const exampleSelect = $('exampleSelect');
const previewFallback = $('previewFallback');
const previewGif = $('previewGif');
const previewLabel = $('previewLabel');

const PLACEHOLDER = `// Pick a component in the gallery (or from the "Example" dropdown above).
//
// Arrow keys move, Enter (or a click) opens a tile, and its file appears
// here: an Item-rooted QML component loaded directly by the gallery's Loader.
// Edit it and the gallery re-runs with that component open. Escape in the
// gallery comes back to the grid.
//
// "launcher" edits gallery.qml, the grid itself.
`;

function camelToSnake(stem) {
  return stem
    .replace(/(.)([A-Z][a-z]+)/g, '$1_$2')
    .replace(/([a-z0-9])([A-Z])/g, '$1_$2')
    .toLowerCase();
}

// --- URL sharing: #code=<base64url(deflate-raw(utf8))> --------------------
const b64url = {
  enc: (bytes) => btoa(String.fromCharCode(...bytes)).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, ''),
  dec: (s) => Uint8Array.from(atob(s.replace(/-/g, '+').replace(/_/g, '/')), (c) => c.charCodeAt(0)),
};
async function encodeShare(text) {
  const stream = new Blob([text]).stream().pipeThrough(new CompressionStream('deflate-raw'));
  return b64url.enc(new Uint8Array(await new Response(stream).arrayBuffer()));
}
async function decodeShare(hash) {
  const stream = new Blob([b64url.dec(hash)]).stream().pipeThrough(new DecompressionStream('deflate-raw'));
  return await new Response(stream).text();
}

// --- Terminal + wasm ---------------------------------------------------------
let term = null;
let fit = null;
let instance = null;
let hasWasmRuntime = false;
const encoder = new TextEncoder();

function initXterm() {
  if (term || typeof window.Terminal !== 'function') return;
  term = new Terminal({
    allowProposedApi: true,
    cursorBlink: false, scrollback: 0, fontSize: 15,
    fontFamily: 'ui-monospace, "Cascadia Mono", "JetBrains Mono", Menlo, monospace',
    theme: { background: '#0d1117', foreground: '#c9d1d9' },
  });
  if (window.UnicodeGraphemesAddon) {
    term.loadAddon(new UnicodeGraphemesAddon.UnicodeGraphemesAddon());
    term.unicode.activeVersion = '15-graphemes';
  }
  fit = new FitAddon.FitAddon();
  term.loadAddon(fit);
  term.open($('terminal'));
  window.qmltuiTerm = term;
  fit.fit();
}

function updatePreviewFallback() {
  if (hasWasmRuntime || !previewFallback) return;
  previewFallback.classList.add('active');
  if (mode === 'example' && focusFile) {
    const stem = focusFile.replace(/\.qml$/i, '');
    const snake = camelToSnake(stem);
    previewGif.src = `../assets/casts/components/${snake}.gif`;
    previewLabel.textContent = `${snake}.gif`;
  } else {
    previewGif.src = '../assets/casts/gallery-tour.gif';
    previewLabel.textContent = 'gallery-tour.gif';
  }
}

async function startRuntime() {
  if (typeof window.qmltui_playground_entry !== 'function') {
    hasWasmRuntime = false;
    status.textContent = 'preview mode (Wasm runtime not built locally)';
    updatePreviewFallback();
    return;
  }
  initXterm();
  status.textContent = 'loading runtime…';
  instance = await window.qmltui_playground_entry({
    noInitialRun: true,
    qmltuiOnOutput: (bytes) => term.write(bytes),
    qmltuiOnQuit: () => { status.textContent = 'Qt.quit() called. Edit or press Run to restart.'; term.reset(); },
    qmltuiOnWarning: (text) => addProblem('warning', text),
    print: (t) => console.log(t),
    printErr: (t) => console.warn(t),
  });
  hasWasmRuntime = true;
  if (previewFallback) previewFallback.classList.remove('active');
  instance.qmltuiResize(term.cols, term.rows);
  term.onData((s) => instance.qmltuiFeedInput(encoder.encode(s)));
  term.onBinary((s) => instance.qmltuiFeedInput(Uint8Array.from(s, (c) => c.charCodeAt(0))));
  term.onResize(({ cols, rows }) => instance.qmltuiResize(cols, rows));
  term.onTitleChange((title) => {
    const m = title.match(/ · (\S+)$/);
    if (m && sources[m[1]] !== undefined) focusExample(m[1]);
    else if (!m && mode === 'example') showGrid();
  });
  new ResizeObserver(() => fit.fit()).observe($('terminal'));
  try { instance.callMain([]); } catch (e) { if (e !== 'unwind') throw e; }
  status.textContent = 'ready';
}

// --- Editor state ------------------------------------------------------------
// mode: 'grid'     the gallery shows its grid; the editor holds the read-only hint
//       'example'  a tile is open; the editor holds that example file
//       'launcher' the editor holds gallery.qml, the grid itself
let mode = 'grid';
let launcher = '';          // gallery.qml with every edit applied so far
let sources = {};           // example file name -> text, with edits applied
let focusFile = null;       // the file in the editor for mode 'example'
let settingDoc = false;     // suppress run-on-edit while the page replaces the document
const editable = new Compartment();

function commitEditor() {
  if (mode === 'example' && focusFile) sources[focusFile] = editor.state.doc.toString();
  else if (mode === 'launcher') launcher = editor.state.doc.toString();
}

function setDoc(text, isEditable) {
  settingDoc = true;
  editor.dispatch({
    changes: { from: 0, to: editor.state.doc.length, insert: text },
    effects: editable.reconfigure(EditorView.editable.of(isEditable)),
  });
  settingDoc = false;
}

function focusExample(file) {
  if (!file || sources[file] === undefined) return;
  if (mode === 'example' && focusFile === file) return;
  commitEditor();
  mode = 'example';
  focusFile = file;
  setDoc(sources[file], true);
  $('focusId').textContent = `examples/${file}`;
  $('wholeFile').textContent = 'launcher';
  if (exampleSelect && exampleSelect.value !== file) exampleSelect.value = file;
  updatePreviewFallback();
}

function showGrid() {
  commitEditor();
  mode = 'grid';
  focusFile = null;
  setDoc(PLACEHOLDER, false);
  $('focusId').textContent = 'pick a component';
  $('wholeFile').textContent = 'launcher';
  if (exampleSelect) exampleSelect.value = '';
  updatePreviewFallback();
}

function showLauncher() {
  commitEditor();
  mode = 'launcher';
  focusFile = null;
  setDoc(launcher, true);
  $('focusId').textContent = 'gallery.qml';
  $('wholeFile').textContent = 'back';
  if (exampleSelect) exampleSelect.value = '';
  updatePreviewFallback();
}

// --- Editor ------------------------------------------------------------------
let runTimer = null;
let qmlCompletion = null;
fetch('types.json').then((r) => r.json()).then((index) => { qmlCompletion = createQmlCompletion(index); })
  .catch((e) => addProblem('warning', `completion index unavailable: ${e.message}`));
const editor = new EditorView({
  doc: PLACEHOLDER,
  extensions: [
    basicSetup, javascript(), oneDark,
    editable.of(EditorView.editable.of(false)),
    autocompletion({ override: [(ctx) => (qmlCompletion ? qmlCompletion(ctx) : null)], activateOnTyping: true, icons: true }),
    keymap.of([indentWithTab, { key: 'Mod-Enter', run: () => { run(); return true; } }]),
    EditorView.updateListener.of((u) => {
      if (!u.docChanged || settingDoc || mode === 'grid' || !$('autorun').checked) return;
      clearTimeout(runTimer);
      runTimer = setTimeout(run, 400);
    }),
  ],
  parent: $('editor'),
});
window.qmltuiEditor = editor;

function addProblem(kind, text) {
  const li = document.createElement('li');
  li.className = kind;
  li.textContent = text;
  problems.append(li);
}

function resolveExampleFile(queryValue) {
  if (!queryValue) return null;
  const trimmed = queryValue.trim();
  const candidate = trimmed.endsWith('.qml') ? trimmed : `${trimmed}.qml`;
  if (sources[candidate] !== undefined) return candidate;
  const lower = candidate.toLowerCase();
  return Object.keys(sources).find((k) => k.toLowerCase() === lower) || null;
}

function run() {
  problems.replaceChildren();
  commitEditor();
  if (!instance) {
    updatePreviewFallback();
    return;
  }
  if (mode === 'example') {
    const result = instance.qmltuiCheckQml(sources[focusFile], `examples/${focusFile}`);
    if (result) {
      const errors = JSON.parse(result);
      status.textContent = `${errors.length} error${errors.length === 1 ? '' : 's'} in ${focusFile}`;
      for (const e of errors) addProblem('error', `${focusFile}:${e.line}:${e.column}: ${e.message}`);
      return;
    }
    instance.qmltuiWriteFile(`examples/${focusFile}`, sources[focusFile]);
  }
  term.reset();
  instance.qmltuiSetContextProperty('qmltuiAvailableExamples', JSON.stringify(Object.keys(sources).filter((f) => f !== 'TerminalView.qml')));
  instance.qmltuiSetContextProperty('qmltuiGalleryPage', mode === 'example' && focusFile !== 'TerminalView.qml' ? focusFile : '');
  const result = instance.qmltuiLoadQml(launcher, 'gallery.qml');
  if (!result) {
    status.textContent = 'running';
    term.focus();
    return;
  }
  const errors = JSON.parse(result);
  status.textContent = `${errors.length} error${errors.length === 1 ? '' : 's'}`;
  for (const e of errors) addProblem('error', `${e.file || 'gallery.qml'}:${e.line}:${e.column}: ${e.message}`);
}

$('run').onclick = run;
$('toggleEditor').onclick = () => {
  const hidden = document.body.classList.toggle('editor-hidden');
  $('toggleEditor').textContent = hidden ? 'show editor' : 'hide editor';
  if (term) term.focus();
};
$('wholeFile').onclick = () => (mode === 'launcher' ? showGrid() : showLauncher());
if (exampleSelect) {
  exampleSelect.onchange = () => {
    const val = exampleSelect.value;
    if (!val) showGrid();
    else focusExample(val);
    run();
  };
}
$('share').onclick = async () => {
  commitEditor();
  location.hash = 'code=' + await encodeShare(JSON.stringify({ launcher, sources, focusFile }));
  await navigator.clipboard?.writeText(location.href).catch(() => {});
  status.textContent = 'link copied to clipboard';
};

window.addEventListener('message', (ev) => {
  if (ev.data && ev.data.type === 'qmltui:selectExample') {
    const file = resolveExampleFile(ev.data.example);
    if (file) {
      focusExample(file);
      run();
    } else if (!ev.data.example) {
      showGrid();
      run();
    }
  }
});

// --- Boot --------------------------------------------------------------------
(async () => {
  launcher = await (await fetch('gallery.qml')).text();
  const names = await (await fetch('examples/index.json')).json();
  await Promise.all(names.map(async (name) => { sources[name] = await (await fetch(`examples/${name}`)).text(); }));

  if (exampleSelect) {
    for (const name of names) {
      const opt = document.createElement('option');
      opt.value = name;
      opt.textContent = name.replace(/\.qml$/, '');
      exampleSelect.append(opt);
    }
  }

  let initialExample = null;
  const params = new URLSearchParams(location.search);
  if (params.get('example')) {
    initialExample = resolveExampleFile(params.get('example'));
  } else {
    try {
      const parentParams = new URLSearchParams(window.parent?.location?.search || '');
      if (parentParams.get('example')) initialExample = resolveExampleFile(parentParams.get('example'));
    } catch { /* cross-origin ignore */ }
  }

  const m = location.hash.match(/#code=([A-Za-z0-9_-]+)/);
  if (m) {
    try {
      const shared = JSON.parse(await decodeShare(m[1]));
      launcher = shared.launcher ?? launcher;
      Object.assign(sources, shared.sources ?? {});
      if (shared.focusFile && sources[shared.focusFile]) initialExample = shared.focusFile;
    } catch { addProblem('warning', 'could not decode the shared code in the URL'); }
  }

  if (initialExample) focusExample(initialExample);
  else showGrid();

  await startRuntime();
  if (instance) {
    for (const [name, code] of Object.entries(sources)) {
      instance.qmltuiWriteFile(`examples/${name}`, code);
    }
  }
  run();
})().catch((e) => { status.textContent = `failed: ${e.message || e}`; console.error(e); });
