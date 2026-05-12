<script setup>
import { ref, computed } from 'vue'
import store from '../store'
import ChatAISettingsModal from '../components/ChatAISettingsModal.vue'

const inputField = ref(null)
const showAISettings = ref(false)

const chatKey = computed(() => store.getChatKey('ai', 'ai'))

function autoResize(el) {
  el.style.height = 'auto'
  el.style.height = Math.min(el.scrollHeight, 120) + 'px'
}

function sendMessage() {
  const text = inputField.value?.value?.trim()
  if (!text) return
  store.sendMessage(text, 'ai')
  store.upsertRecent('ai', 'ai', store.currentAISettings.nickname, text)
  inputField.value.value = ''
  inputField.value.style.height = 'auto'
}
</script>

<template>
  <div class="screen active chat-screen">
    <div class="top-bar" style="background:var(--wx-green);color:#fff;border-bottom-color:var(--wx-green)">
      <button class="top-bar-back" @click="store.switchScreen('chatlist')">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="#fff"><path d="M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z" /></svg>
      </button>
      <div class="top-bar-title" style="color:#fff">{{ store.currentAISettings.nickname }}</div>
      <button class="top-bar-action" @click="showAISettings = true">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="rgba(255,255,255,.8)"><path d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.07.62-.07.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z" /></svg>
      </button>
    </div>
    <div class="messages-area" style="background:#F5F7F5">
      <template v-for="(msg, i) in store.messages" :key="i">
        <div v-if="msg.type === 'system'" class="msg-system">{{ msg.text }}</div>
        <div v-else-if="msg.type === 'mine'" class="msg-row msg-row-mine">
          <div class="msg-avatar-sm" style="background:var(--wx-green)">我</div>
          <div>
            <div class="msg-bubble msg-bubble-mine">{{ msg.text }}</div>
            <div class="msg-time" style="text-align:right">{{ msg.time }}</div>
          </div>
        </div>
        <div v-else-if="msg.type === 'ai'" class="msg-row">
          <div class="msg-avatar-sm" style="background:var(--wx-green);border-radius:8px">AI</div>
          <div>
            <div class="msg-bubble msg-bubble-ai">
              <div class="ai-bubble-header"><span class="ai-badge">AI</span>{{ store.currentAISettings.nickname }}</div>
              <span v-html="msg.text.replace(/\n/g, '<br>')" />
            </div>
            <div class="msg-time">{{ msg.time }}</div>
          </div>
        </div>
      </template>
      <div v-if="store.messages.length === 0" class="welcome-message">
        <div class="welcome-avatar">AI</div>
        <div class="welcome-text">
          <div class="welcome-title">你好！我是{{ store.currentAISettings.nickname }}</div>
          <div class="welcome-desc">
            我可以帮你：<br>
            - 回答问题和查询信息<br>
            - 撰写和编辑文本<br>
            - 翻译多种语言<br>
            - 分析数据和生成报告<br><br>
            有什么我可以帮你的吗？
          </div>
        </div>
      </div>
    </div>
    <div class="input-bar">
      <button class="input-bar-btn">
        <svg viewBox="0 0 24 24"><path d="M12 14c1.66 0 2.99-1.34 2.99-3L15 5c0-1.66-1.34-3-3-3S9 3.34 9 5v6c0 1.66 1.34 3 3 3zm5.3-3c0 3-2.54 5.1-5.3 5.1S6.7 14 6.7 11H5c0 3.41 2.72 6.23 6 6.72V21h2v-3.28c3.28-.48 6-3.3 6-6.72h-1.7z" /></svg>
      </button>
      <button class="input-bar-btn">
        <svg viewBox="0 0 24 24"><path d="M21 19V5c0-1.1-.9-2-2-2H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2zM8.5 13.5l2.5 3.01L14.5 12l4.5 6H5l3.5-4.5z" /></svg>
      </button>
      <textarea ref="inputField" class="input-field" placeholder="问 AI 助手任何问题..." rows="1" @input="autoResize($event.target)" @keydown.enter.prevent="sendMessage"></textarea>
      <button class="input-send-btn" @click="sendMessage">
        <svg viewBox="0 0 24 24"><path d="M2.01 21L23 12 2.01 3 2 10l15 2-15 2z" /></svg>
      </button>
    </div>

    <ChatAISettingsModal :show="showAISettings" :chat-key="chatKey" @close="showAISettings = false" />
  </div>
</template>

<style scoped>
.chat-screen{background:var(--wx-bg)}
.messages-area{flex:1;overflow-y:auto;padding:16px;display:flex;flex-direction:column;gap:16px}
.welcome-message{display:flex;gap:16px;padding:20px;background:var(--wx-white);border-radius:var(--wx-radius-lg);border:1px solid var(--wx-border)}
.welcome-avatar{
  width:48px;height:48px;border-radius:12px;background:var(--wx-green);flex-shrink:0;
  display:flex;align-items:center;justify-content:center;font-size:18px;font-weight:700;color:#fff;
}
.welcome-text{flex:1}
.welcome-title{font-size:16px;font-weight:600;margin-bottom:8px}
.welcome-desc{font-size:14px;color:var(--wx-text-secondary);line-height:1.6}
</style>
