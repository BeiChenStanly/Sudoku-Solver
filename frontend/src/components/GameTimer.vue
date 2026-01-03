<script setup lang="ts">
import { useSudokuGame } from '@/composables/useSudokuGame'

const { formattedTime, timerRunning, toggleTimer, isVictory } = useSudokuGame()
</script>

<template>
  <div class="timer">
    <div class="timer-display">
      <svg class="timer-icon" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
        <circle cx="12" cy="12" r="10" />
        <polyline points="12 6 12 12 16 14" />
      </svg>
      <span class="timer-value">{{ formattedTime }}</span>
    </div>
    
    <button 
      class="timer-btn"
      :class="{ 'timer-btn--paused': !timerRunning }"
      :disabled="isVictory"
      @click="toggleTimer"
    >
      <template v-if="timerRunning">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
          <rect x="6" y="4" width="4" height="16" rx="1" />
          <rect x="14" y="4" width="4" height="16" rx="1" />
        </svg>
        <span>暂停</span>
      </template>
      <template v-else>
        <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
          <polygon points="5 3 19 12 5 21 5 3" />
        </svg>
        <span>继续</span>
      </template>
    </button>
  </div>
</template>

<style scoped lang="scss">
.timer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  padding: 12px 16px;
  background: var(--bg-secondary);
  border-radius: var(--radius-lg);
  border: 1px solid var(--border-light);
}

.timer-display {
  display: flex;
  align-items: center;
  gap: 10px;
}

.timer-icon {
  color: var(--primary);
}

.timer-value {
  font-size: 24px;
  font-weight: 700;
  font-family: 'JetBrains Mono', 'Consolas', monospace;
  color: var(--text-primary);
  min-width: 80px;
  letter-spacing: 1px;
}

.timer-btn {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 14px;
  border: none;
  border-radius: var(--radius-md);
  background: var(--bg-tertiary);
  color: var(--text-secondary);
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  transition: all var(--transition-fast);
  
  &:hover:not(:disabled) {
    background: var(--primary);
    color: var(--text-inverse);
  }
  
  &:disabled {
    opacity: 0.4;
    cursor: not-allowed;
  }
  
  &--paused:not(:disabled) {
    background: var(--success);
    color: var(--text-inverse);
    animation: pulse 2s ease-in-out infinite;
    
    &:hover {
      background: #059669;
    }
  }
}

@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.8; }
}
</style>
