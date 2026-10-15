import DefaultTheme from 'vitepress/theme'
import type { Theme } from 'vitepress'
import PlaygroundPage from './PlaygroundPage.vue'
import './custom.css'

export default {
  extends: DefaultTheme,
  enhanceApp({ app }) {
    app.component('PlaygroundPage', PlaygroundPage)
  }
} satisfies Theme
