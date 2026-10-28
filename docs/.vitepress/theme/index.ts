import DefaultTheme from 'vitepress/theme'
import { useRoute, withBase } from 'vitepress'
import type { Theme } from 'vitepress'
import { nextTick, onMounted, watch } from 'vue'
import PlaygroundPage from './PlaygroundPage.vue'
import './custom.css'

function toBase64Url(str: string): string {
  const bytes = new TextEncoder().encode(str)
  let binary = ''
  for (let i = 0; i < bytes.length; i++) {
    binary += String.fromCharCode(bytes[i])
  }
  return btoa(binary).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '')
}

function preparePlaygroundQml(raw: string): string {
  const lines = raw.replace(/\r\n/g, '\n').trim().split('\n')
  const headerLines: string[] = []
  const bodyLines: string[] = []
  let inHeader = true

  for (const line of lines) {
    const trimmed = line.trim()
    if (inHeader && (trimmed === '' || trimmed.startsWith('pragma ') || trimmed.startsWith('import '))) {
      if (trimmed.startsWith('pragma ') || trimmed.startsWith('import ')) {
        headerLines.push(trimmed)
      }
      continue
    }
    inHeader = false
    bodyLines.push(line)
  }

  const body = bodyLines.join('\n').trim()
  const hasPragma = headerLines.some((l) => l.startsWith('pragma ComponentBehavior'))
  const hasQtQml = headerLines.some((l) => /^import\s+QtQml\b/.test(l))
  const hasQmlTui = headerLines.some((l) => /^import\s+QmlTui\s*$/.test(l))
  const hasControls = headerLines.some((l) => /^import\s+QmlTui\.Controls\b/.test(l))

  const finalHeaders: string[] = []
  if (!hasPragma) finalHeaders.push('pragma ComponentBehavior: Bound')
  for (const h of headerLines) {
    if (h.startsWith('pragma ')) finalHeaders.push(h)
  }
  if (!hasQtQml) finalHeaders.push('import QtQml')
  if (!hasQmlTui) finalHeaders.push('import QmlTui')
  if (!hasControls) finalHeaders.push('import QmlTui.Controls')
  for (const h of headerLines) {
    if (h.startsWith('import ') && !finalHeaders.includes(h)) {
      finalHeaders.push(h)
    }
  }

  if (/^Terminal\s*\{/.test(body)) {
    return `${finalHeaders.join('\n')}\n\n${body}\n`
  }

  const indentedBody = body
    .split('\n')
    .map((l) => (l.trim() ? `    ${l}` : ''))
    .join('\n')

  return (
    `${finalHeaders.join('\n')}\n\n` +
    `Terminal {\n` +
    `    id: root\n` +
    `    title: "QmlTui Playground"\n\n` +
    `${indentedBody}\n` +
    `}\n`
  )
}

function attachTryInPlaygroundButtons(): void {
  if (typeof document === 'undefined') return
  const blocks = document.querySelectorAll<HTMLElement>('.vp-doc div.language-qml')
  blocks.forEach((block) => {
    if (block.querySelector('.qmltui-try-btn')) return
    const codeEl = block.querySelector('pre code')
    if (!codeEl) return
    const rawCode = codeEl.textContent || ''
    const hasTerminal = /\bTerminal\s*\{/.test(rawCode)
    const isGalleryExample =
      /^pragma\s+ComponentBehavior:\s*Bound\b/.test(rawCode.trim()) &&
      /\{/.test(rawCode) &&
      !/\bmodel:\s*processModel\b/.test(rawCode)
    if (
      !rawCode.trim() ||
      (!hasTerminal && !isGalleryExample) ||
      /\bimport\s+QmlTui\.PtyView\b/.test(rawCode)
    ) {
      return
    }

    const prepared = preparePlaygroundQml(rawCode)
    const encoded = toBase64Url(prepared)
    const link = document.createElement('a')
    link.className = 'qmltui-try-btn'
    link.href = `${withBase('/playground.html')}?code=${encoded}`
    link.target = '_blank'
    link.rel = 'noopener'
    link.textContent = '▶ Try in Playground'
    link.title = 'Open and edit this QML snippet in the WebAssembly Playground'
    block.appendChild(link)
  })

  const playgroundLinks = document.querySelectorAll<HTMLAnchorElement>('a[href]')
  playgroundLinks.forEach((anchor) => {
    const href = anchor.getAttribute('href') || ''
    if (
      /(?:^|\/)playground(?:\.html|\/index\.html)?(?:[?#]|$)/.test(href) &&
      !anchor.hasAttribute('target')
    ) {
      anchor.target = '_blank'
      anchor.rel = 'noopener'
    }
  })
}

export default {
  extends: DefaultTheme,
  enhanceApp({ app }) {
    app.component('PlaygroundPage', PlaygroundPage)
  },
  setup() {
    const route = useRoute()
    onMounted(() => {
      nextTick(() => attachTryInPlaygroundButtons())
    })
    watch(
      () => route.path,
      () => {
        nextTick(() => attachTryInPlaygroundButtons())
      }
    )
  }
} satisfies Theme
