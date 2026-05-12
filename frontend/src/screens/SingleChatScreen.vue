<script setup>
import { ref, computed } from 'vue'
import store from '../store'
import ChatAISettingsModal from '../components/ChatAISettingsModal.vue'

const inputField = ref(null)
const showAISettings = ref(false)

const currentFriend = computed(() => {
  return store.friends.find(f => f.userId === store.currentChat.id)
})

const chatKey = computed(() => store.getChatKey('single', store.currentChat.id))

function autoResize(el) {
  el.style.height = 'auto'
  el.style.height = Math.min(el.scrollHeight, 120) + 'px'
}

function insertAtAI() {
  if (inputField.value) {
    inputField.value.value = '@' + store.currentAISettings.nickname + ' ' + inputField.value.value
    inputField.value.focus()
    autoResize(inputField.value)
  }
}

function sendMessage() {
  const text = inputField.value?.value?.trim()
  if (!text) return
  store.sendMessage(text, 'single')
  store.upsertRecent('single', store.currentChat.id, store.currentChat.name, text)
  inputField.value.value = ''
  inputField.value.style.height = 'auto'
}
</script>

<template>
  <div class="screen active chat-screen">
    <div class="top-bar">
      <button class="top-bar-back" @click="store.switchScreen('chatlist')">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text)"><path d="M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z" /></svg>
      </button>
      <div class="top-bar-title">{{ store.currentChat.name }}</div>
      <div class="top-bar-actions">
        <button class="top-bar-action" title="AI 设置" @click="showAISettings = true">
          <span style="color:var(--wx-green);font-size:11px;font-weight:700">AI</span>
        </button>
        <button class="top-bar-action">
          <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><circle cx="12" cy="5" r="2" /><circle cx="12" cy="12" r="2" /><circle cx="12" cy="19" r="2" /></svg>
        </button>
      </div>
    </div>
    <div class="messages-area">
      <template v-for="(msg, i) in store.messages" :key="i">
        <div v-if="msg.type === 'system'" class="msg-system">{{ msg.text }}</div>
        <div v-else-if="msg.type === 'other'" class="msg-row">
          <div class="msg-avatar-sm" :style="{ background: currentFriend?.color || '#576B95' }">{{ store.currentChat.name[0] }}</div>
          <div>
            <div class="msg-bubble msg-bubble-other">{{ msg.text }}</div>
            <div class="msg-time">{{ msg.time }}</div>
          </div>
        </div>
        <div v-else-if="msg.type === 'mine'" class="msg-row msg-row-mine">
          <div class="msg-avatar-sm" style="background:var(--wx-green)">我</div>
          <div>
            <div class="msg-bubble msg-bubble-mine">{{ msg.text }}</div>
            <div class="msg-time" style="text-align:right">{{ msg.time }}</div>
          </div>
        </div>
        <div v-else-if="msg.type === 'ai'" class="msg-row">
          <div class="msg-avatar-sm" style="background:var(--wx-green)">AI</div>
          <div>
            <div class="msg-bubble msg-bubble-ai">
              <div class="ai-bubble-header"><span class="ai-badge">AI</span>{{ store.currentAISettings.nickname }}</div>
              {{ msg.text }}
            </div>
            <div class="msg-time">{{ msg.time }}</div>
          </div>
        </div>
      </template>
      <div v-if="store.messages.length === 0" class="empty-chat">
        <div class="empty-chat-icon">💬</div>
        <div class="empty-chat-text">暂无消息，发送一条消息开始聊天</div>
      </div>
    </div>
    <div class="input-bar">
      <button class="input-bar-btn">
        <svg viewBox="0 0 24 24"><path d="M12 14c1.66 0 2.99-1.34 2.99-3L15 5c0-1.66-1.34-3-3-3S9 3.34 9 5v6c0 1.66 1.34 3 3 3zm5.3-3c0 3-2.54 5.1-5.3 5.1S6.7 14 6.7 11H5c0 3.41 2.72 6.23 6 6.72V21h2v-3.28c3.28-.48 6-3.3 6-6.72h-1.7z" /></svg>
      </button>
      <button class="input-bar-btn">
        <svg viewBox="0 0 24 24"><path d="M21 19V5c0-1.1-.9-2-2-2H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2zM8.5 13.5l2.5 3.01L14.5 12l4.5 6H5l3.5-4.5z" /></svg>
      </button>
      <button class="input-bar-btn" :title="'@' + store.currentAISettings.nickname" @click="insertAtAI">
        <span style="color:var(--wx-green);font-size:12px;font-weight:700">@AI</span>
      </button>
      <textarea ref="inputField" class="input-field" placeholder="输入消息..." rows="1" @input="autoResize($event.target)" @keydown.enter.prevent="sendMessage"></textarea>
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
.empty-chat{flex:1;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:12px;color:var(--wx-text-secondary)}
.empty-chat-icon{font-size:48px}
.empty-chat-text{font-size:14px}
.top-bar-actions{display:flex;gap:4px}
</style>
