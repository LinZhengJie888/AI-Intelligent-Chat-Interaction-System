<script setup>
import { ref, nextTick } from 'vue'

const isPanelOpen = ref(false)
const isDragging = ref(false)
const petRef = ref(null)
const messagesRef = ref(null)
const inputRef = ref(null)

const messages = ref([
  { type: 'ai', text: '你好！需要我帮忙吗？可以直接问我任何问题 😊' }
])

let startX, startY, petX, petY, moved

function onMouseDown(e) {
  const touch = e.touches ? e.touches[0] : e
  startX = touch.clientX
  startY = touch.clientY
  const rect = petRef.value.getBoundingClientRect()
  petX = rect.left
  petY = rect.top
  moved = false
  isDragging.value = true
  petRef.value.classList.add('dragging')
  if (e.touches) e.preventDefault()

  document.addEventListener('mousemove', onMouseMove)
  document.addEventListener('touchmove', onMouseMove, { passive: false })
  document.addEventListener('mouseup', onMouseUp)
  document.addEventListener('touchend', onMouseUp)
}

function onMouseMove(e) {
  if (!isDragging.value) return
  const touch = e.touches ? e.touches[0] : e
  const dx = touch.clientX - startX
  const dy = touch.clientY - startY
  if (Math.abs(dx) > 5 || Math.abs(dy) > 5) moved = true
  const newX = petX + dx
  const newY = petY + dy
  petRef.value.style.left = Math.max(0, Math.min(window.innerWidth - 52, newX)) + 'px'
  petRef.value.style.top = Math.max(0, Math.min(window.innerHeight - 52, newY)) + 'px'
  petRef.value.style.right = 'auto'
  petRef.value.style.bottom = 'auto'
}

function onMouseUp() {
  if (!isDragging.value) return
  isDragging.value = false
  petRef.value.classList.remove('dragging')
  document.removeEventListener('mousemove', onMouseMove)
  document.removeEventListener('touchmove', onMouseMove)
  document.removeEventListener('mouseup', onMouseUp)
  document.removeEventListener('touchend', onMouseUp)

  if (!moved) {
    togglePanel()
  }
}

function togglePanel() {
  isPanelOpen.value = !isPanelOpen.value
  if (isPanelOpen.value) {
    nextTick(() => {
      const rect = petRef.value.getBoundingClientRect()
      const panel = document.querySelector('.ai-pet-panel')
      if (panel) {
        let left = rect.right - 340
        let top = rect.top - 480 - 12
        if (top < 8) top = rect.bottom + 12
        if (left < 8) left = 8
        if (left + 340 > window.innerWidth - 8) left = window.innerWidth - 348
        panel.style.left = left + 'px'
        panel.style.top = top + 'px'
      }
    })
  }
}

function sendMessage() {
  const text = inputRef.value.value.trim()
  if (!text) return

  messages.value.push({ type: 'user', text })
  inputRef.value.value = ''

  nextTick(() => {
    if (messagesRef.value) {
      messagesRef.value.scrollTop = messagesRef.value.scrollHeight
    }
  })

  setTimeout(() => {
    messages.value.push({
      type: 'ai',
      text: '收到！让我想想... 你可以试试问我更具体的问题，我会给出更详细的回答。'
    })
    nextTick(() => {
      if (messagesRef.value) {
        messagesRef.value.scrollTop = messagesRef.value.scrollHeight
      }
    })
  }, 800)
}
</script>

<template>
  <div
    ref="petRef"
    class="ai-pet"
    :class="{ dragging: isDragging }"
    @mousedown="onMouseDown"
    @touchstart="onMouseDown"
  >
    AI
  </div>

  <div class="ai-pet-panel" :class="{ open: isPanelOpen }">
    <div class="ai-pet-header">
      <div class="ai-pet-header-avatar">
        <span class="ai-badge">AI</span>
      </div>
      <div class="ai-pet-header-info">
        <div class="ai-pet-header-name">AI 助手</div>
        <div class="ai-pet-header-status">随时为你服务</div>
      </div>
      <button class="ai-pet-close" @click.stop="isPanelOpen = false">
        <svg viewBox="0 0 24 24"><path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z" /></svg>
      </button>
    </div>
    <div ref="messagesRef" class="ai-pet-messages">
      <div style="text-align:center;font-size:12px;color:var(--wx-text-tertiary)">AI 助手已就绪</div>
      <div v-for="(msg, i) in messages" :key="i" class="msg-row" :class="{ 'msg-row-mine': msg.type === 'user' }" style="max-width:85%">
        <template v-if="msg.type === 'ai'">
          <div style="width:28px;height:28px;border-radius:8px;background:var(--wx-green);display:flex;align-items:center;justify-content:center;flex-shrink:0;font-size:10px;font-weight:700;color:#fff">AI</div>
          <div class="msg-bubble msg-bubble-ai" style="font-size:13px;padding:8px 12px">{{ msg.text }}</div>
        </template>
        <template v-else>
          <div style="width:28px;height:28px;border-radius:50%;background:var(--wx-green);display:flex;align-items:center;justify-content:center;flex-shrink:0;font-size:12px;color:#fff;font-weight:600">我</div>
          <div class="msg-bubble msg-bubble-mine" style="font-size:13px;padding:8px 12px">{{ msg.text }}</div>
        </template>
      </div>
    </div>
    <div class="ai-pet-input-area">
      <input ref="inputRef" class="ai-pet-input" placeholder="问 AI 助手..." @keydown.enter.prevent="sendMessage">
      <button class="ai-pet-send" @click="sendMessage">
        <svg viewBox="0 0 24 24"><path d="M2.01 21L23 12 2.01 3 2 10l15 2-15 2z" /></svg>
      </button>
    </div>
  </div>
</template>

<style scoped>
.ai-pet{
  position:fixed;bottom:80px;right:20px;z-index:200;
  width:52px;height:52px;border-radius:50%;background:var(--wx-green);
  display:flex;align-items:center;justify-content:center;cursor:pointer;
  box-shadow:0 4px 16px rgba(7,193,96,.35);transition:box-shadow .2s;
  font-size:18px;font-weight:700;color:#fff;user-select:none;-webkit-user-select:none;
}
.ai-pet:hover{box-shadow:0 6px 24px rgba(7,193,96,.45)}
.ai-pet.dragging{box-shadow:0 8px 32px rgba(7,193,96,.5);transform:scale(1.08);cursor:grabbing}
.ai-pet.hidden{display:none}

.ai-pet-panel{
  position:fixed;z-index:201;
  width:340px;height:480px;background:var(--wx-white);border-radius:16px;
  box-shadow:0 8px 40px rgba(0,0,0,.15);display:none;flex-direction:column;overflow:hidden;
  border:1px solid var(--wx-border);
}
.ai-pet-panel.open{display:flex}
.ai-pet-header{
  padding:14px 16px;background:var(--wx-green);color:#fff;
  display:flex;align-items:center;gap:10px;
}
.ai-pet-header-avatar{
  width:32px;height:32px;border-radius:50%;
  display:flex;align-items:center;justify-content:center;
}
.ai-pet-header-info{flex:1}
.ai-pet-header-name{font-size:15px;font-weight:600}
.ai-pet-header-status{font-size:11px;opacity:.8}
.ai-pet-close{
  width:28px;height:28px;border:none;background:rgba(255,255,255,.2);border-radius:50%;
  cursor:pointer;display:flex;align-items:center;justify-content:center;
}
.ai-pet-close svg{width:16px;height:16px;fill:#fff}
.ai-pet-messages{flex:1;overflow-y:auto;padding:12px;display:flex;flex-direction:column;gap:10px}
.ai-pet-input-area{
  padding:8px 12px;border-top:1px solid var(--wx-border);display:flex;gap:8px;align-items:center;
}
.ai-pet-input{
  flex:1;padding:8px 12px;border:1px solid var(--wx-border);border-radius:20px;
  font-size:14px;outline:none;font-family:var(--font-body);
}
.ai-pet-input:focus{border-color:var(--wx-green)}
.ai-pet-send{
  width:32px;height:32px;border:none;border-radius:50%;background:var(--wx-green);
  cursor:pointer;display:flex;align-items:center;justify-content:center;
}
.ai-pet-send svg{width:16px;height:16px;fill:#fff}

.msg-row{display:flex;gap:10px;max-width:80%}
.msg-row-mine{align-self:flex-end;flex-direction:row-reverse}
.msg-bubble{
  padding:10px 14px;border-radius:var(--wx-radius-lg);font-size:15px;line-height:1.5;
  position:relative;word-break:break-word;max-width:100%;
}
.msg-bubble-other{background:var(--wx-bubble-other);border:1px solid var(--wx-border)}
.msg-bubble-mine{background:var(--wx-bubble-me)}
.msg-bubble-ai{
  background:var(--wx-bubble-ai);border:1px solid rgba(7,193,96,.15);
  border-radius:var(--wx-radius-lg);padding:12px 16px;
}

@media(max-width:768px){
  .ai-pet-panel{width:calc(100vw - 32px);max-width:340px}
}
</style>
