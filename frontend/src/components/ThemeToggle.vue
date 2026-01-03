<script setup lang="ts">
import { computed } from 'vue'
import { useTheme, type ThemeMode } from '@/composables/useTheme'

const { themeMode, setTheme } = useTheme()

const themeIcon = computed(() => {
  switch (themeMode.value) {
    case 'light': return '☀️'
    case 'dark': return '🌙'
    default: return '🌓'
  }
})

const themeLabel = computed(() => {
  switch (themeMode.value) {
    case 'light': return '浅色'
    case 'dark': return '深色'
    default: return '自动'
  }
})

function cycleTheme() {
  const modes: ThemeMode[] = ['light', 'dark', 'auto']
  const idx = modes.indexOf(themeMode.value)
  setTheme(modes[(idx + 1) % modes.length]!)
}
</script>

<template>
  <button class="theme-toggle" @click="cycleTheme" :title="`当前: ${themeLabel}`">
    <span class="theme-icon">{{ themeIcon }}</span>
    <span class="theme-label">{{ themeLabel }}</span>
  </button>
</template>

<style scoped lang="scss">
.theme-toggle {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 12px;
  border: 1px solid var(--border-light);
  border-radius: var(--radius-md);
  background: var(--bg-secondary);
  color: var(--text-secondary);
  font-size: 13px;
  font-weight: 500;
  cursor: pointer;
  transition: all var(--transition-fast);
  
  &:hover {
    background: var(--bg-tertiary);
    color: var(--text-primary);
    border-color: var(--border-medium);
  }
}

.theme-icon {
  font-size: 16px;
}

.theme-label {
  @media (max-width: 480px) {
    display: none;
  }
}
</style>
