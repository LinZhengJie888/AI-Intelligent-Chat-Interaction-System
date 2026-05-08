<script setup>
import { ref } from 'vue'
import store from '../store'

const inputField = ref(null)

const messages = [
  { type: 'system', text: '———— 14:20 ————' },
  { type: 'other', sender: '张', color: '#576B95', text: '明天下午三点开会，记得带上方案', time: '14:20' },
  { type: 'mine', text: '好的，我准备一下', time: '14:21' },
  { type: 'other', sender: '张', color: '#576B95', text: '另外，把上次的数据报告也带上', time: '14:22' },
  { type: 'mine', text: '收到，我会整理好的 👍', time: '14:23' },
  { type: 'system', text: '———— 今天 ————' },
  { type: 'other', sender: '张', color: '#576B95', text: '对了，你可以让 AI 助手帮你整理数据', time: '09:15' },
  { type: 'ai', text: '我可以帮你整理上次的数据报告，需要我分析哪些数据维度？', time: '09:15' },
  { type: 'mine', text: '帮我整理一下上个月的销售数据', time: '09:20' }
]

function autoResize(el) {
  el.style.height = 'auto'
  el.style.height = Math.min(el.scrollHeight, 120) + 'px'
}

function insertAtAI() {
  if (inputField.value) {
    inputField.value.value = '@AI ' + inputField.value.value
    inputField.value.focus()
    autoResize(inputField.value)
  }
}
</script>

<template>
  <div class="screen active chat-screen">
    <div class="top-bar">
      <button class="top-bar-back" @click="store.switchScreen('chatlist')">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text)"><path d="M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z" /></svg>
      </button>
      <div class="top-bar-title">{{ store.chatTitle }}</div>
      <button class="top-bar-action">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><circle cx="12" cy="5" r="2" /><circle cx="12" cy="12" r="2" /><circle cx="12" cy="19" r="2" /></svg>
      </button>
    </div>
    <div class="messages-area">
      <template v-for="(msg, i) in messages" :key="i">
        <div v-if="msg.type === 'system'" class="msg-system">{{ msg.text }}</div>
        <div v-else-if="msg.type === 'other'" class="msg-row">
          <div class="msg-avatar-sm" :style="{ background: msg.color }">{{ msg.sender }}</div>
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
              <div class="ai-bubble-header"><span class="ai-badge">AI</span>AI 助手</div>
              {{ msg.text }}
            </div>
            <div class="msg-time">{{ msg.time }}</div>
          </div>
        </div>
      </template>
    </div>
    <div class="input-bar">
      <button class="input-bar-btn">
        <svg viewBox="0 0 24 24"><path d="M12 14c1.66 0 2.99-1.34 2.99-3L15 5c0-1.66-1.34-3-3-3S9 3.34 9 5v6c0 1.66 1.34 3 3 3zm5.3-3c0 3-2.54 5.1-5.3 5.1S6.7 14 6.7 11H5c0 3.41 2.72 6.23 6 6.72V21h2v-3.28c3.28-.48 6-3.3 6-6.72h-1.7z" /></svg>
      </button>
      <button class="input-bar-btn">
        <svg viewBox="0 0 24 24"><path d="M21 19V5c0-1.1-.9-2-2-2H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2zM8.5 13.5l2.5 3.01L14.5 12l4.5 6H5l3.5-4.5z" /></svg>
      </button>
      <button class="input-bar-btn" title="@AI 助手" @click="insertAtAI">
        <span style="color:var(--wx-green);font-size:13px;font-weight:700">@AI</span>
      </button>
      <textarea ref="inputField" class="input-field" placeholder="输入消息..." rows="1" @input="autoResize($event.target)"></textarea>
      <button class="input-send-btn">
        <svg viewBox="0 0 24 24"><path d="M2.01 21L23 12 2.01 3 2 10l15 2-15 2z" /></svg>
      </button>
    </div>
  </div>
</template>

<style scoped>
.chat-screen{background:var(--wx-bg)}
.messages-area{flex:1;overflow-y:auto;padding:16px;display:flex;flex-direction:column;gap:16px}
</style>
