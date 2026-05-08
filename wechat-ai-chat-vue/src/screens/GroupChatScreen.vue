<script setup>
import { ref } from 'vue'
import store from '../store'

const inputField = ref(null)

const messages = [
  { type: 'system', text: '———— 昨天 ————' },
  { type: 'other', sender: '李', name: '李四', color: '#576B95', text: '新版本的设计稿已经更新了，大家看看', time: '13:10' },
  { type: 'other', sender: '王', name: '王五', color: '#FA5151', text: '收到，我先看看导航部分', time: '13:12' },
  { type: 'mine', text: '好的，我负责看表单那块', time: '13:15' },
  { type: 'system', text: '———— 今天 ————' },
  { type: 'other', sender: '赵', name: '赵六', color: '#FF8800', text: '表单的交互我有几个建议，等下开会讨论', time: '09:30' },
  { type: 'ai', text: '已记录本次会议待办事项，会后我会整理成清单发给大家。', time: '09:31' },
  { type: 'other', sender: '李', name: '李四', color: '#576B95', text: '👍 辛苦了', time: '09:35' }
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
      <div class="top-bar-title">{{ store.groupTitle }}</div>
      <button class="top-bar-action">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><path d="M12 12.75c1.63 0 3.07.39 4.24.9 1.08.48 1.76 1.56 1.76 2.73V18H6v-1.61c0-1.18.68-2.26 1.76-2.73 1.17-.52 2.61-.91 4.24-.91zM4 13c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zm1.13 1.1c-.37-.06-.74-.1-1.13-.1-.99 0-1.93.21-2.78.58A2.01 2.01 0 000 16.43V18h4.5v-1.61c0-.83.23-1.61.63-2.29zM20 13c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zm4 3.43c0-.81-.48-1.53-1.22-1.85A6.95 6.95 0 0020 14c-.39 0-.76.04-1.13.1.4.68.63 1.46.63 2.29V18H24v-1.57zM12 6c1.66 0 3 1.34 3 3s-1.34 3-3 3-3-1.34-3-3 1.34-3 3-3z" /></svg>
      </button>
    </div>
    <div class="messages-area">
      <template v-for="(msg, i) in messages" :key="i">
        <div v-if="msg.type === 'system'" class="msg-system">{{ msg.text }}</div>
        <div v-else-if="msg.type === 'other'" class="msg-row">
          <div class="msg-avatar-sm" :style="{ background: msg.color }">{{ msg.sender }}</div>
          <div>
            <div class="msg-sender">{{ msg.name }}</div>
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
          <div class="msg-avatar-sm" style="background:var(--wx-green);border-radius:8px">AI</div>
          <div>
            <div class="msg-sender" style="color:var(--wx-green)">AI 助手 <span class="chat-name-tag tag-ai">AI</span></div>
            <div class="msg-bubble msg-bubble-ai">
              <div class="ai-bubble-header"><span class="ai-badge">AI</span>群助手</div>
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
.chat-name-tag{
  font-size:10px;padding:1px 5px;border-radius:3px;font-weight:500;
}
.tag-ai{background:rgba(7,193,96,.12);color:var(--wx-green)}
</style>
