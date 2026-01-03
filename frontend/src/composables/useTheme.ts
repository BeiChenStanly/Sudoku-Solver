import { ref, computed } from 'vue'

export type ThemeMode = 'light' | 'dark' | 'auto'

const THEME_KEY = 'sudoku-theme'

// 获取存储的主题或默认值
function getStoredTheme(): ThemeMode {
  if (typeof window === 'undefined') return 'auto'
  return (localStorage.getItem(THEME_KEY) as ThemeMode) || 'auto'
}

// 全局主题状态
const themeMode = ref<ThemeMode>(getStoredTheme())

// 实际显示的主题（考虑 auto 模式）
const isDark = computed(() => {
  if (themeMode.value === 'auto') {
    return window.matchMedia('(prefers-color-scheme: dark)').matches
  }
  return themeMode.value === 'dark'
})

// 应用主题到 DOM
function applyTheme() {
  document.documentElement.setAttribute('data-theme', themeMode.value)
}

// 切换主题
function setTheme(mode: ThemeMode) {
  themeMode.value = mode
  localStorage.setItem(THEME_KEY, mode)
  applyTheme()
}

// 循环切换主题
function toggleTheme() {
  const modes: ThemeMode[] = ['light', 'dark', 'auto']
  const currentIndex = modes.indexOf(themeMode.value)
  const nextIndex = (currentIndex + 1) % modes.length
  setTheme(modes[nextIndex]!)
}

// 初始化主题
function initTheme() {
  applyTheme()
  
  // 监听系统主题变化
  const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)')
  mediaQuery.addEventListener('change', () => {
    if (themeMode.value === 'auto') {
      // 触发响应式更新
      applyTheme()
    }
  })
}

export function useTheme() {
  return {
    themeMode,
    isDark,
    setTheme,
    toggleTheme,
    initTheme
  }
}
