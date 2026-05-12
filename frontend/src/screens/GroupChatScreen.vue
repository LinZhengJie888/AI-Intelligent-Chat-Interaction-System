<script setup>
import { ref, computed } from 'vue'
import store from '../store'
import ChatAISettingsModal from '../components/ChatAISettingsModal.vue'

const inputField = ref(null)
const showMembers = ref(false)
const showAISettings = ref(false)

const currentGroup = computed(() => {
  return store.groups.find(g => g.groupId === store.currentChat.id)
})

const groupMembers = computed(() => {
  return store.friends.slice(0, currentGroup.value?.memberCount || 6)
})

const chatKey = computed(() => store.getChatKey('group', store.currentChat.id))

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
  store.sendMessage(text, 'group')
  store.upsertRecent('group', store.currentChat.id, store.currentChat.name, `${store.currentUser.username}: ${text}`)
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
        <button class="top-bar-action" @click="showMembers = true">
          <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><path d="M12 12.75c1.63 0 3.07.39 4.24.9 1.08.48 1.76 1.56 1.76 2.73V18H6v-1.61c0-1.18.68-2.26 1.76-2.73 1.17-.52 2.61-.91 4.24-.91zM4 13c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zm1.13 1.1c-.37-.06-.74-.1-1.13-.1-.99 0-1.93.21-2.78.58A2.01 2.01 0 000 16.43V18h4.5v-1.61c0-.83.23-1.61.63-2.29zM20 13c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zm4 3.43c0-.81-.48-1.53-1.22-1.85A6.95 6.95 0 0020 14c-.39 0-.76.04-1.13.1.4.68.63 1.46.63 2.29V18H24v-1.57zM12 6c1.66 0 3 1.34 3 3s-1.34 3-3 3-3-1.34-3-3 1.34-3 3-3z" /></svg>
        </button>
      </div>
    </div>
    <div class="messages-area">
      <template v-for="(msg, i) in store.messages" :key="i">
        <div v-if="msg.type === 'system'" class="msg-system">{{ msg.text }}</div>
        <div v-else-if="msg.type === 'other'" class="msg-row">
          <div class="msg-avatar-sm" :style="{ background: msg.color || '#576B95' }">{{ msg.sender }}</div>
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
            <div class="msg-sender" style="color:var(--wx-green)">{{ store.currentAISettings.nickname }} <span class="tag-ai">AI</span></div>
            <div class="msg-bubble msg-bubble-ai">
              <div class="ai-bubble-header"><span class="ai-badge">AI</span>{{ store.currentAISettings.nickname }}</div>
              {{ msg.text }}
            </div>
            <div class="msg-time">{{ msg.time }}</div>
          </div>
        </div>
      </template>
      <div v-if="store.messages.length === 0" class="empty-chat">
        <div class="empty-chat-icon">👥</div>
        <div class="empty-chat-text">暂无消息，发送一条消息开始群聊</div>
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

    <!-- 群成员弹窗 -->
    <div v-if="showMembers" class="modal-overlay" @click.self="showMembers=false">
      <div class="modal-card fade-in">
        <div class="modal-title">群成员 ({{ groupMembers.length }})</div>
        <div class="members-list">
          <div v-for="member in groupMembers" :key="member.userId" class="member-item">
            <div class="member-avatar" :style="{ background: member.color || '#576B95' }">{{ member.username[0] }}</div>
            <div class="member-info">
              <div class="member-name">{{ member.username }}</div>
              <div class="member-id">ID: {{ member.userId }}</div>
            </div>
          </div>
        </div>
        <div class="modal-actions">
          <button class="modal-btn modal-btn-confirm" @click="showMembers=false">关闭</button>
        </div>
      </div>
    </div>

    <ChatAISettingsModal :show="showAISettings" :chat-key="chatKey" @close="showAISettings = false" />
  </div>
</template>

<style scoped>
.chat-screen{background:var(--wx-bg)}
.messages-area{flex:1;overflow-y:auto;padding:16px;display:flex;flex-direction:column;gap:16px}
.tag-ai{font-size:10px;padding:1px 5px;border-radius:3px;font-weight:500;background:rgba(7,193,96,.12);color:var(--wx-green)}
.empty-chat{flex:1;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:12px;color:var(--wx-text-secondary)}
.empty-chat-icon{font-size:48px}
.empty-chat-text{font-size:14px}
.top-bar-actions{display:flex;gap:4px}

/* 弹窗 */
.modal-overlay{
  position:fixed;inset:0;background:rgba(0,0,0,.4);z-index:200;
  display:flex;align-items:center;justify-content:center;padding:24px;
}
.modal-card{
  background:var(--wx-white);border-radius:16px;padding:28px 24px;width:100%;max-width:380px;
  box-shadow:0 12px 40px rgba(0,0,0,.15);
}
.modal-title{font-size:18px;font-weight:600;margin-bottom:20px;text-align:center}
.members-list{max-height:300px;overflow-y:auto;display:flex;flex-direction:column;gap:8px}
.member-item{display:flex;align-items:center;gap:12px;padding:8px;border-radius:8px}
.member-item:hover{background:var(--wx-bg)}
.member-avatar{
  width:36px;height:36px;border-radius:var(--wx-radius);display:flex;align-items:center;justify-content:center;
  font-size:14px;font-weight:600;color:#fff;flex-shrink:0;
}
.member-info{flex:1;min-width:0}
.member-name{font-size:14px;font-weight:500}
.member-id{font-size:12px;color:var(--wx-text-secondary)}
.modal-actions{display:flex;gap:12px;margin-top:20px}
.modal-btn{
  flex:1;padding:11px;border:none;border-radius:8px;font-size:15px;font-weight:500;
  cursor:pointer;transition:all .15s;font-family:var(--font-body);
}
.modal-btn-confirm{background:var(--wx-green);color:#fff}
.modal-btn-confirm:hover{background:var(--wx-green-dark)}
</style>
