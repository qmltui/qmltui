import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import { defineConfig } from 'vitepress'

const docsDir = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..')
const repoRoot = path.resolve(docsDir, '..')

function stagePlaygroundAssets(): void {
  const outDir = path.join(docsDir, 'public', 'playground')
  const examplesOut = path.join(outDir, 'examples')
  fs.mkdirSync(examplesOut, { recursive: true })

  // 1. Copy playground HTML/JS/types from examples/wasm/www/
  const wasmWwwDir = path.join(repoRoot, 'examples', 'wasm', 'www')
  for (const name of ['index.html', 'qmltui-playground.js', 'qmltui-completion.js', 'types.json']) {
    const src = path.join(wasmWwwDir, name)
    if (fs.existsSync(src)) fs.copyFileSync(src, path.join(outDir, name))
  }

  // 2. Copy gallery Main.qml and examples/*.qml + write examples/index.json
  const galleryDir = path.join(repoRoot, 'examples', 'gallery')
  const mainQml = path.join(galleryDir, 'Main.qml')
  if (fs.existsSync(mainQml)) fs.copyFileSync(mainQml, path.join(outDir, 'gallery.qml'))

  const galleryExamplesDir = path.join(galleryDir, 'examples')
  if (fs.existsSync(galleryExamplesDir)) {
    const qmlFiles = fs.readdirSync(galleryExamplesDir).filter((f) => f.endsWith('.qml')).sort()
    for (const file of qmlFiles) {
      fs.copyFileSync(path.join(galleryExamplesDir, file), path.join(examplesOut, file))
    }
    fs.writeFileSync(path.join(examplesOut, 'index.json'), JSON.stringify(qmlFiles, null, 2) + '\n')
  }

  // 3. Stage recorded GIFs and branding SVGs into public/assets/
  const castsSrc = path.join(docsDir, 'assets', 'casts')
  const castsPub = path.join(docsDir, 'public', 'assets', 'casts')
  if (fs.existsSync(castsSrc)) {
    fs.cpSync(castsSrc, castsPub, {
      recursive: true,
      filter: (src) => fs.statSync(src).isDirectory() || src.endsWith('.gif')
    })
  }

  const brandingSrc = path.join(docsDir, 'assets', 'branding')
  const brandingPub = path.join(docsDir, 'public', 'assets', 'branding')
  if (fs.existsSync(brandingSrc)) {
    fs.cpSync(brandingSrc, brandingPub, {
      recursive: true,
      filter: (src) => fs.statSync(src).isDirectory() || src.endsWith('.svg') || src.endsWith('.png')
    })
  }

  // 4. Copy prebuilt Qt for WebAssembly runtime bundle if available
  const wasmCandidates = [
    process.env.QMLTUI_WASM_DIR,
    path.join(repoRoot, 'build-wasm', 'www'),
    path.join(repoRoot, 'build-wasm', 'playground'),
    path.join(repoRoot, 'build-wasm')
  ].filter((dir): dir is string => Boolean(dir))

  for (const cand of wasmCandidates) {
    if (fs.existsSync(path.join(cand, 'qmltui_playground.js'))) {
      for (const art of ['qmltui_playground.js', 'qmltui_playground.wasm', 'qtloader.js']) {
        const artPath = path.join(cand, art)
        if (fs.existsSync(artPath)) fs.copyFileSync(artPath, path.join(outDir, art))
      }
      break
    }
  }
}

stagePlaygroundAssets()

export default defineConfig({
  title: 'QmlTui',
  description: 'Retained-mode terminal UI framework for C++17 and Qt 6',
  srcDir: '.',
  srcExclude: ['contributing/**'],
  ignoreDeadLinks: [
    (url: string) => {
      const clean = url.replace(/[?#].*$/, '').replace(/^(?:\.\/|\.\.\/)+/, '')
      if (!/^(?:examples\/|scripts\/|contributing\/|LICENSE$)/.test(clean)) return false
      const base = clean.startsWith('contributing/') ? docsDir : repoRoot
      const target = path.join(base, clean)
      const dirTarget = clean.endsWith('/index') ? path.join(base, clean.slice(0, -6)) : null
      return (
        fs.existsSync(target) ||
        fs.existsSync(`${target}.md`) ||
        (dirTarget !== null && fs.existsSync(dirTarget))
      )
    }
  ],
  cleanUrls: false,
  appearance: 'dark',

  rewrites: {
    'README.md': 'index.md'
  },

  head: [
    ['link', { rel: 'icon', type: 'image/svg+xml', href: '/assets/branding/qmltui_mark.svg' }]
  ],

  themeConfig: {
    logo: {
      light: '/assets/branding/qmltui_mark_light.svg',
      dark: '/assets/branding/qmltui_mark.svg'
    },

    search: {
      provider: 'local'
    },

    nav: [
      { text: 'Getting Started', link: '/getting-started' },
      { text: 'Guides', link: '/guides/' },
      { text: 'QML Types', link: '/qml-types/' },
      { text: 'C++ Reference', link: '/cpp-classes/' },
      { text: 'Examples', link: '/examples' },
      { text: 'Playground', link: '/playground' }
    ],

    socialLinks: [
      { icon: 'github', link: 'https://github.com/qmltui-dev/qmltui' }
    ],

    sidebar: [
      {
        text: 'Getting Started',
        collapsed: false,
        items: [
          { text: 'Overview', link: '/' },
          { text: 'Getting Started Tutorial', link: '/getting-started' },
          { text: 'Qt Quick & Widgets to QmlTui', link: '/guides/qt-quick-to-qmltui' },
          { text: 'Build & CMake Configuration', link: '/build-configuration' },
          { text: 'Qt Creator Setup & Tooling', link: '/guides/qt-creator' },
          { text: 'Examples & Gallery', link: '/examples' },
          { text: 'WebAssembly Playground', link: '/playground' }
        ]
      },
      {
        text: 'Guides',
        collapsed: false,
        items: [
          { text: 'All Guides', link: '/guides/' },
          { text: 'Qt Quick & Widgets to QmlTui', link: '/guides/qt-quick-to-qmltui' },
          { text: 'Architecture', link: '/guides/architecture' },
          { text: 'Layouts & Geometry', link: '/guides/layouts' },
          { text: 'Input & Focus', link: '/guides/input-and-focus' },
          { text: 'Styling & Colors', link: '/guides/styling-and-colors' },
          { text: 'Models & Views', link: '/guides/models' },
          { text: 'Animation & States', link: '/guides/animation-and-states' },
          { text: 'Inline Mode & Scrollback', link: '/guides/inline-mode' },
          { text: 'Embedded PTY (PtyView)', link: '/guides/pty-view' },
          { text: 'Custom C++ Items', link: '/guides/custom-items' },
          { text: 'Testing with Runner', link: '/guides/testing' },
          { text: 'WebAssembly Playground', link: '/guides/webassembly' }
        ]
      },
      {
        text: 'QML Types',
        collapsed: false,
        items: [
          { text: 'QML Type Index', link: '/qml-types/' },
          {
            text: 'QmlTui: Core & Terminal',
            collapsed: true,
            items: [
              { text: 'Terminal', link: '/qml-types/terminal' },
              { text: 'Scrollback', link: '/qml-types/scrollback' },
              { text: 'Item', link: '/qml-types/item' },
              { text: 'FocusScope', link: '/qml-types/focusscope' },
              { text: 'Popup', link: '/qml-types/popup' },
              { text: 'Rectangle', link: '/qml-types/rectangle' },
              { text: 'Border', link: '/qml-types/border' },
              { text: 'Anchors', link: '/qml-types/anchors' },
              { text: 'AnchorLine', link: '/qml-types/anchorline' },
              { text: 'Gradient', link: '/qml-types/gradient' },
              { text: 'GradientStop', link: '/qml-types/gradientstop' },
              { text: 'Clipboard', link: '/qml-types/clipboard' },
              { text: 'QmlTui', link: '/qml-types/qmltui' },
              { text: 'color', link: '/qml-types/color' },
              { text: 'span', link: '/qml-types/span' },
              { text: 'highlightRange', link: '/qml-types/highlightrange' },
              { text: 'terminalCapabilities', link: '/qml-types/terminalcapabilities' }
            ]
          },
          {
            text: 'QmlTui: Layout & Containers',
            collapsed: true,
            items: [
              { text: 'Row', link: '/qml-types/row' },
              { text: 'Column', link: '/qml-types/column' },
              { text: 'RowLayout', link: '/qml-types/rowlayout' },
              { text: 'ColumnLayout', link: '/qml-types/columnlayout' },
              { text: 'GridLayout', link: '/qml-types/gridlayout' },
              { text: 'StackLayout', link: '/qml-types/stacklayout' },
              { text: 'Layout', link: '/qml-types/layout' },
              { text: 'Repeater', link: '/qml-types/repeater' },
              { text: 'Loader', link: '/qml-types/loader' }
            ]
          },
          {
            text: 'QmlTui: Display & Text Inputs',
            collapsed: true,
            items: [
              { text: 'Text', link: '/qml-types/text' },
              { text: 'TextInput', link: '/qml-types/textinput' },
              { text: 'TextArea', link: '/qml-types/textarea' }
            ]
          },
          {
            text: 'QmlTui: Data Views',
            collapsed: true,
            items: [
              { text: 'TableView', link: '/qml-types/tableview' },
              { text: 'TreeView', link: '/qml-types/treeview' },
              { text: 'ListView', link: '/qml-types/listview' },
              { text: 'GridView', link: '/qml-types/gridview' },
              { text: 'ScrollView', link: '/qml-types/scrollview' }
            ]
          },
          {
            text: 'QmlTui: Input Events',
            collapsed: true,
            items: [
              { text: 'Keys', link: '/qml-types/keys' },
              { text: 'KeyEvent', link: '/qml-types/keyevent' },
              { text: 'MouseArea', link: '/qml-types/mousearea' },
              { text: 'MouseEvent', link: '/qml-types/mouseevent' },
              { text: 'Shortcut', link: '/qml-types/shortcut' }
            ]
          },
          {
            text: 'QmlTui: Canvas',
            collapsed: true,
            items: [
              { text: 'Canvas', link: '/qml-types/canvas' },
              { text: 'CanvasShape', link: '/qml-types/canvasshape' },
              { text: 'CanvasRectangle', link: '/qml-types/canvasrectangle' },
              { text: 'Circle', link: '/qml-types/circle' },
              { text: 'Line', link: '/qml-types/line' },
              { text: 'Point', link: '/qml-types/point' },
              { text: 'Polygon', link: '/qml-types/polygon' }
            ]
          },
          {
            text: 'QmlTui: Animation & State',
            collapsed: true,
            items: [
              { text: 'Animation', link: '/qml-types/animation' },
              { text: 'PropertyAnimation', link: '/qml-types/propertyanimation' },
              { text: 'NumberAnimation', link: '/qml-types/numberanimation' },
              { text: 'ColorAnimation', link: '/qml-types/coloranimation' },
              { text: 'Behavior', link: '/qml-types/behavior' },
              { text: 'Easing', link: '/qml-types/easing' },
              { text: 'State', link: '/qml-types/state' },
              { text: 'StateGroup', link: '/qml-types/stategroup' },
              { text: 'PropertyChanges', link: '/qml-types/propertychanges' },
              { text: 'Transition', link: '/qml-types/transition' }
            ]
          },
          {
            text: 'QmlTui.Controls',
            collapsed: true,
            items: [
              { text: 'AbstractButton', link: '/qml-types/abstractbutton' },
              { text: 'Button', link: '/qml-types/button' },
              { text: 'ButtonGroup', link: '/qml-types/buttongroup' },
              { text: 'CheckBox', link: '/qml-types/checkbox' },
              { text: 'RadioButton', link: '/qml-types/radiobutton' },
              { text: 'Switch', link: '/qml-types/switch' },
              { text: 'Slider', link: '/qml-types/slider' },
              { text: 'ProgressBar', link: '/qml-types/progressbar' },
              { text: 'BusyIndicator', link: '/qml-types/busyindicator' },
              { text: 'Sparkline', link: '/qml-types/sparkline' },
              { text: 'Panel', link: '/qml-types/panel' },
              { text: 'Dialog', link: '/qml-types/dialog' },
              { text: 'Toast', link: '/qml-types/toast' },
              { text: 'ToolTip', link: '/qml-types/tooltip' },
              { text: 'TabBar', link: '/qml-types/tabbar' },
              { text: 'StackView', link: '/qml-types/stackview' }
            ]
          },
          {
            text: 'QmlTui.Image',
            collapsed: true,
            items: [
              { text: 'Image', link: '/qml-types/image' },
              { text: 'AnimatedSprite', link: '/qml-types/animatedsprite' }
            ]
          },
          {
            text: 'QmlTui.PtyView',
            collapsed: true,
            items: [
              { text: 'PtyView', link: '/qml-types/ptyview' }
            ]
          }
        ]
      },
      {
        text: 'C++ Classes',
        collapsed: true,
        items: [
          { text: 'C++ Class Index', link: '/cpp-classes/' },
          { text: 'Terminal', link: '/cpp-classes/terminal' },
          { text: 'TerminalEngine', link: '/cpp-classes/terminalengine' },
          { text: 'TerminalDevice', link: '/cpp-classes/terminaldevice' },
          { text: 'TerminalBackend', link: '/cpp-classes/terminalbackend' },
          { text: 'MemoryTerminalBackend', link: '/cpp-classes/memoryterminalbackend' },
          { text: 'TerminalCapabilities', link: '/cpp-classes/terminalcapabilities' },
          { text: 'Item', link: '/cpp-classes/item' },
          { text: 'Rectangle', link: '/cpp-classes/rectangle' },
          { text: 'Text', link: '/cpp-classes/text' },
          { text: 'MouseArea', link: '/cpp-classes/mousearea' },
          { text: 'TextGridBuffer', link: '/cpp-classes/textgridbuffer' },
          { text: 'Cell & TextAttribute', link: '/cpp-classes/cell' },
          { text: 'Color', link: '/cpp-classes/color' },
          { text: 'InputDecoder', link: '/cpp-classes/inputdecoder' },
          { text: 'ImageProvider', link: '/cpp-classes/imageprovider' },
          { text: 'QtMessageRouting', link: '/cpp-classes/qtmessagerouting' },
          { text: 'PtyView', link: '/cpp-classes/ptyview' },
          { text: 'Runner', link: '/cpp-classes/runner' }
        ]
      },
      {
        text: 'Release Notes',
        collapsed: true,
        items: [
          { text: 'Changelog', link: '/changes/changelog' },
          { text: 'QmlTui 0.4', link: '/changes/0.4' },
          { text: 'QmlTui 0.3', link: '/changes/0.3' },
          { text: 'QmlTui 0.1', link: '/changes/0.1' }
        ]
      }
    ],

    footer: {
      message: 'Documentation licensed under CC-BY-4.0. Library code under LGPL-3.0-only OR GPL-3.0-only.',
      copyright: 'Copyright © 2026 Yosri Ayed'
    }
  }
})
