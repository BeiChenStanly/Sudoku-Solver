<script setup lang="ts">
import { useSudokuGame } from '@/composables/useSudokuGame'

const { showVictoryModal, formattedTime } = useSudokuGame()

const emit = defineEmits<{
  (e: 'newGame'): void
  (e: 'close'): void
}>()

function handleNewGame() {
  emit('newGame')
  showVictoryModal.value = false
}

function handleClose() {
  emit('close')
  showVictoryModal.value = false
}
</script>

<template>
  <Teleport to="body">
    <Transition name="modal">
      <div v-if="showVictoryModal" class="modal-overlay" @click="handleClose">
        <div class="modal-content" @click.stop>
          <div class="confetti">
            <span v-for="i in 20" :key="i" class="confetti-piece" :style="{
              '--delay': `${i * 0.1}s`,
              '--x': `${Math.random() * 100}%`,
              '--rotation': `${Math.random() * 360}deg`
            }"></span>
          </div>
          
          <div class="victory-icon">🎉</div>
          
          <h2 class="victory-title">恭喜完成！</h2>
          
          <div class="victory-stats">
            <div class="stat">
              <span class="stat-label">用时</span>
              <span class="stat-value">{{ formattedTime }}</span>
            </div>
          </div>
          
          <div class="victory-actions">
            <button class="btn btn--primary" @click="handleNewGame">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M12 3v3m0 12v3M3 12h3m12 0h3M5.6 5.6l2.1 2.1m8.6 8.6l2.1 2.1M5.6 18.4l2.1-2.1m8.6-8.6l2.1-2.1" />
              </svg>
              新游戏
            </button>
            <button class="btn btn--ghost" @click="handleClose">关闭</button>
          </div>
        </div>
      </div>
    </Transition>
  </Teleport>
</template>

<style scoped lang="scss">
.modal-overlay {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.6);
  backdrop-filter: blur(6px);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
  padding: 20px;
}

.modal-content {
  position: relative;
  background: var(--bg-secondary);
  border-radius: var(--radius-xl);
  padding: 40px;
  text-align: center;
  box-shadow: var(--shadow-xl);
  max-width: 400px;
  width: 100%;
  overflow: hidden;
}

.confetti {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 100%;
  pointer-events: none;
  overflow: hidden;
}

.confetti-piece {
  position: absolute;
  top: -20px;
  left: var(--x);
  width: 10px;
  height: 10px;
  background: var(--primary);
  animation: confetti-fall 3s ease-out var(--delay) infinite;
  transform: rotate(var(--rotation));
  
  &:nth-child(odd) {
    background: var(--accent);
    border-radius: 50%;
  }
  
  &:nth-child(3n) {
    background: var(--success);
  }
}

@keyframes confetti-fall {
  0% {
    transform: translateY(0) rotate(0deg);
    opacity: 1;
  }
  100% {
    transform: translateY(400px) rotate(720deg);
    opacity: 0;
  }
}

.victory-icon {
  font-size: 72px;
  margin-bottom: 16px;
  animation: bounce 1s ease infinite;
}

@keyframes bounce {
  0%, 100% { transform: translateY(0); }
  50% { transform: translateY(-12px); }
}

.victory-title {
  font-size: 28px;
  font-weight: 700;
  color: var(--text-primary);
  margin-bottom: 24px;
}

.victory-stats {
  display: flex;
  justify-content: center;
  gap: 24px;
  margin-bottom: 32px;
}

.stat {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.stat-label {
  font-size: 13px;
  color: var(--text-tertiary);
}

.stat-value {
  font-size: 24px;
  font-weight: 700;
  font-family: 'JetBrains Mono', 'Consolas', monospace;
  color: var(--primary);
}

.victory-actions {
  display: flex;
  gap: 12px;
  justify-content: center;
}

.btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  padding: 12px 24px;
  font-size: 14px;
  font-weight: 600;
  border-radius: var(--radius-md);
  border: none;
  cursor: pointer;
  transition: all var(--transition-fast);
  
  &--primary {
    background: var(--primary);
    color: var(--text-inverse);
    
    &:hover {
      background: var(--primary-hover);
      transform: translateY(-2px);
      box-shadow: var(--shadow-md);
    }
  }
  
  &--ghost {
    background: transparent;
    color: var(--text-secondary);
    
    &:hover {
      background: var(--bg-tertiary);
      color: var(--text-primary);
    }
  }
}

// 过渡动画
.modal-enter-active,
.modal-leave-active {
  transition: all 0.3s ease;
}

.modal-enter-from,
.modal-leave-to {
  opacity: 0;
  
  .modal-content {
    transform: scale(0.9);
  }
}

.modal-enter-to,
.modal-leave-from {
  opacity: 1;
  
  .modal-content {
    transform: scale(1);
  }
}
</style>
