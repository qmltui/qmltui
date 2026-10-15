<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { withBase } from 'vitepress'

const iframeSrc = ref('')

onMounted(() => {
  const base = withBase('/playground/index.html')
  const search = window.location.search || ''
  const hash = window.location.hash || ''
  iframeSrc.value = `${base}${search}${hash}`
})
</script>

<template>
  <div class="qmltui-playground-wrapper">
    <iframe
      v-if="iframeSrc"
      :src="iframeSrc"
      class="qmltui-playground-iframe"
      title="QmlTui WebAssembly Playground"
      allow="clipboard-read; clipboard-write"
    />
  </div>
</template>

<style scoped>
.qmltui-playground-wrapper {
  width: 100%;
  height: calc(100vh - var(--vp-nav-height, 64px));
  background: #0d1117;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.qmltui-playground-iframe {
  width: 100%;
  height: 100%;
  border: 0;
  background: #0d1117;
}
</style>
