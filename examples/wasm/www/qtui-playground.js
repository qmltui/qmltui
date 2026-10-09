// The gallery page: CodeMirror on the left, the gallery running in xterm.js on
// the right. Loaded as an ES module by index.html. CodeMirror 6 comes from
// esm.sh through the import map in index.html (exact versions, single instance
// per package).
//
// Every demo is a complete standalone file under examples/. The gallery
// launcher (gallery.qml) instantiates them from their text, which it receives
// as the qtuiExampleSources context property, and names the open one in its
// terminal title ("Qtui Gallery · Button.qml"). When that arrives the editor
// shows the file; each edit is compiled first (qtuiCheckQml), then handed back
// and the gallery re-runs with the same tile open (qtuiGalleryPage). Escape
// in the gallery empties the editor again.
import { EditorView, basicSetup } from 'codemirror';
import { Compartment } from '@codemirror/state';
import { javascript } from '@codemirror/lang-javascript';
import { oneDark } from '@codemirror/theme-one-dark';
import { keymap } from '@codemirror/view';
import { indentWithTab } from '@codemirror/commands';
import { autocompletion } from '@codemirror/autocomplete';
import { createQmlCompletion } from './qtui-completion.js';

const $ = (id) => document.getElementById(id);
const status = $('status');
const problems = $('problems');

const PLACEHOLDER = `// Pick a component in the gallery on the right.
//
// Arrow keys move, Enter (or a click) opens a tile, and its file appears
// here: a complete Qtui app with its imports and Terminal root, exactly
// what runs inside the gallery. Edit it and the gallery re-runs with that
// component open. Escape in the gallery comes back to the grid.
//
// "launcher" edits gallery.qml, the grid itself.
`;

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
const term = new Terminal({
  allowProposedApi: true,
  cursorBlink: false, scrollback: 0, fontSize: 15,
  fontFamily: 'ui-monospace, "Cascadia Mono", "JetBrains Mono", Menlo, monospace',
  theme: { background: '#0d1117', foreground: '#c9d1d9' },
});
// Qtui measures text the way modern terminals do: grapheme clusters, with
// emoji, flags, and keycaps two columns wide. xterm.js defaults to Unicode 6
// widths (emoji one column), which clips glyphs and shifts every row that
// holds one. The graphemes addon applies the same rules Qtui uses.
if (window.UnicodeGraphemesAddon) {
  term.loadAddon(new UnicodeGraphemesAddon.UnicodeGraphemesAddon());
  term.unicode.activeVersion = '15-graphemes';
}
const fit = new FitAddon.FitAddon();
term.loadAddon(fit);
term.open($('terminal'));
window.qtuiTerm = term;   // for scripted tests
fit.fit();

let instance = null;
const encoder = new TextEncoder();

async function startRuntime() {
  status.textContent = 'loading runtime…';
  instance = await window.qtui_playground_entry({
    noInitialRun: true,
    qtuiOnOutput: (bytes) => term.write(bytes),
    qtuiOnQuit: () => { status.textContent = 'Qt.quit() called. Edit or press Run to restart.'; term.reset(); },
    qtuiOnWarning: (text) => addProblem('warning', text),
    print: (t) => console.log(t),
    printErr: (t) => console.warn(t),
  });
  instance.qtuiResize(term.cols, term.rows);
  term.onData((s) => instance.qtuiFeedInput(encoder.encode(s)));
  term.onBinary((s) => instance.qtuiFeedInput(Uint8Array.from(s, (c) => c.charCodeAt(0))));
  term.onResize(({ cols, rows }) => instance.qtuiResize(cols, rows));
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

// Takes the editor's current text into the model, whatever the mode.
function commitEditor() {
  if (mode === 'example') sources[focusFile] = editor.state.doc.toString();
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
  if (mode === 'example' && focusFile === file) return;
  commitEditor();
  mode = 'example';
  focusFile = file;
  setDoc(sources[file], true);
  $('focusId').textContent = `examples/${file}`;
  $('wholeFile').textContent = 'launcher';
}

function showGrid() {
  commitEditor();
  mode = 'grid';
  focusFile = null;
  setDoc(PLACEHOLDER, false);
  $('focusId').textContent = 'pick a component';
  $('wholeFile').textContent = 'launcher';
}

function showLauncher() {
  commitEditor();
  mode = 'launcher';
  focusFile = null;
  setDoc(launcher, true);
  $('focusId').textContent = 'gallery.qml';
  $('wholeFile').textContent = 'back';
}

// --- Editor ------------------------------------------------------------------
let runTimer = null;
// Completion index; the source is registered up front and answers once loaded.
let qmlCompletion = null;
fetch('types.json').then((r) => r.json()).then((index) => { qmlCompletion = createQmlCompletion(index); })
  .catch((e) => addProblem('warning', `completion index unavailable: ${e.message}`));
const editor = new EditorView({
  doc: PLACEHOLDER,
  extensions: [
    basicSetup, javascript(), oneDark,
    editable.of(EditorView.editable.of(false)),
    autocompletion({ override: [(ctx) => (qmlCompletion ? qmlCompletion(ctx) : null)], activateOnTyping: true, icons: true }),
    // Tab indents inside the editor instead of moving the browser's focus
    // to the terminal (Escape then Tab still leaves, as CodeMirror documents).
    keymap.of([indentWithTab, { key: 'Mod-Enter', run: () => { run(); return true; } }]),
    EditorView.updateListener.of((u) => {
      if (!u.docChanged || settingDoc || mode === 'grid' || !$('autorun').checked) return;
      clearTimeout(runTimer);
      runTimer = setTimeout(run, 400);
    }),
  ],
  parent: $('editor'),
});
window.qtuiEditor = editor;   // for scripted tests

function addProblem(kind, text) {
  const li = document.createElement('li');
  li.className = kind;
  li.textContent = text;
  problems.append(li);
}

// Compiles an edited example before the gallery gets it, then runs the
// launcher with every source; a focused tile reopens.
function run() {
  if (!instance) return;
  problems.replaceChildren();
  commitEditor();
  if (mode === 'example') {
    const result = instance.qtuiCheckQml(sources[focusFile], focusFile);
    if (result) {
      const errors = JSON.parse(result);
      status.textContent = `${errors.length} error${errors.length === 1 ? '' : 's'} in ${focusFile}`;
      for (const e of errors) addProblem('error', `${focusFile}:${e.line}:${e.column}: ${e.message}`);
      return;   // the previous version keeps running
    }
  }
  term.reset();
  instance.qtuiSetContextProperty('qtuiExampleSources', JSON.stringify(sources));
  instance.qtuiSetContextProperty('qtuiGalleryPage', mode === 'example' ? focusFile : '');
  const result = instance.qtuiLoadQml(launcher, 'gallery.qml');
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
// The terminal takes the whole width while the editor is hidden; the
// ResizeObserver on #terminal refits xterm and the app gets the new size.
$('toggleEditor').onclick = () => {
  const hidden = document.body.classList.toggle('editor-hidden');
  $('toggleEditor').textContent = hidden ? 'show editor' : 'hide editor';
  term.focus();
};
$('wholeFile').onclick = () => (mode === 'launcher' ? showGrid() : showLauncher());
$('share').onclick = async () => {
  commitEditor();
  location.hash = 'code=' + await encodeShare(JSON.stringify({ launcher, sources }));
  await navigator.clipboard?.writeText(location.href).catch(() => {});
  status.textContent = 'link copied to the address bar';
};

// --- Boot --------------------------------------------------------------------
// gallery.qml and examples/*.qml are the shipped gallery; a #code= hash (a
// shared edit: JSON of { launcher, sources }) replaces them.
(async () => {
  await startRuntime();
  launcher = await (await fetch('gallery.qml')).text();
  const names = await (await fetch('examples/index.json')).json();
  await Promise.all(names.map(async (name) => { sources[name] = await (await fetch(`examples/${name}`)).text(); }));
  const m = location.hash.match(/#code=([A-Za-z0-9_-]+)/);
  if (m) {
    try {
      const shared = JSON.parse(await decodeShare(m[1]));
      launcher = shared.launcher ?? launcher;
      Object.assign(sources, shared.sources ?? {});
    } catch { addProblem('warning', 'could not decode the shared code in the URL'); }
  }
  run();
})().catch((e) => { status.textContent = `failed: ${e.message || e}`; console.error(e); });
