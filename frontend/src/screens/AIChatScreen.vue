<script setup>
import { ref, watch, nextTick } from 'vue'
import store from '../store'

const messagesArea = ref(null)

// 监听消息变化，自动滚动到底部
watch(() => store.messages.length, () => {
  nextTick(() => {
    if (messagesArea.value) {
      messagesArea.value.scrollTop = messagesArea.value.scrollHeight
    }
  })
})

function autoResize(el) {
  el.style.height = 'auto'
  el.style.height = Math.min(el.scrollHeight, 120) + 'px'
}

function sendMessage(e) {
  const textarea = e.target.closest('.input-bar').querySelector('textarea')
  const text = textarea.value.trim()
  if (!text) return
  store.sendMessage(text, 'ai')
  textarea.value = ''
  textarea.style.height = 'auto'
}
</script>

<template>
  <div class="screen active chat-screen">
    <div class="top-bar" style="background:var(--wx-green);color:#fff;border-bottom-color:var(--wx-green)">
      <button class="top-bar-back" @click="store.switchScreen('chatlist')">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="#fff"><path d="M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z" /></svg>
      </button>
      <div class="top-bar-title" style="color:#fff">AI 助手</div>
      <button class="top-bar-action" @click="store.switchScreen('ai-settings')">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="rgba(255,255,255,.8)"><path d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.07.62-.07.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z" /></svg>
      </button>
    </div>

    <div ref="messagesArea" class="messages-area" style="background:#F5F7F5">
      <!-- 欢迎消息 -->
      <div v-if="store.messages.length === 0" class="msg-row">
        <div class="msg-avatar-sm" style="background:var(--wx-green);border-radius:8px">AI</div>
        <div>
          <div class="msg-bubble msg-bubble-ai">
            <div class="ai-bubble-header"><span class="ai-badge">AI</span>AI 助手</div>
            你好！我是你的 AI 助手，有什么我可以帮你的吗？
          </div>
        </div>
      </div>

      <template v-for="(msg, i) in store.messages" :key="i">
        <div v-if="msg.type === 'mine'" class="msg-row msg-row-mine">
          <div class="msg-avatar-sm" style="background:var(--wx-green)">我</div>
          <div>
            <div class="msg-bubble msg-bubble-mine">{{ msg.text }}</div>
            <div class="msg-time" style="text-align:right">{{ msg.time }}</div>
          </div>
        </div>
        <div v-else class="msg-row">
          <div class="msg-avatar-sm" style="background:var(--wx-green);border-radius:8px">AI</div>
          <div>
            <div class="msg-bubble msg-bubble-ai">
              <div class="ai-bubble-header"><span class="ai-badge">AI</span>AI 助手</div>
              <span style="white-space:pre-line">{{ msg.text }}</span>
            </div>
            <div class="msg-time">{{ msg.time }}</div>
          </div>
        </div>
      </template>
    </div>

    <div class="input-bar">
      <textarea class="input-field" placeholder="问 AI 助手任何问题..." rows="1" @input="autoResize($event.target)" @keydown.enter.exact.prevent="sendMessage"></textarea>
      <button class="input-send-btn" @click="sendMessage">
        <svg viewBox="0 0 24 24"><path d="M2.01 21L23 12 2.01 3 2 10l15 2-15 2z" /></svg>
      </button>
    </div>
  </div>
</template>

<style scoped>
.chat-screen{background:var(--wx-bg)}
.messages-area{flex:1;overflow-y:auto;padding:16px;display:flex;flex-direction:column;gap:16px}
</style>
