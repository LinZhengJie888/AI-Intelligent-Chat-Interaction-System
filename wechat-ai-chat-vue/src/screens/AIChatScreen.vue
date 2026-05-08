<script setup>
import { ref } from 'vue'
import store from '../store'

const inputField = ref(null)

const messages = [
  { type: 'system', text: '———— AI 助手 ————' },
  {
    type: 'ai',
    text: '你好！我是你的 AI 助手 👋\n\n我可以帮你：\n• 回答问题和查询信息\n• 撰写和编辑文本\n• 翻译多种语言\n• 分析数据和生成报告\n\n有什么我可以帮你的吗？',
    time: '09:00'
  },
  { type: 'mine', text: '帮我写一段产品介绍，关于我们的新聊天应用', time: '09:05' },
  {
    type: 'ai',
    text: 'WeChat AI — 让沟通更智能\n\nWeChat AI 是新一代智能聊天应用，将传统即时通讯与 AI 助手深度融合。在熟悉的聊天体验中，你可以随时唤起 AI 助手，让它帮你解答问题、整理信息、甚至参与群聊讨论。\n\n核心特色：\n• AI 融入对话 — 不用切换应用，AI 就在你的聊天列表里\n• 群聊智能 — 在群聊中召唤 AI，自动记录待办和会议纪要\n• 浮窗助手 — 和任何人聊天时都能随时唤出 AI 浮窗\n\n需要我调整语气或补充更多细节吗？',
    time: '09:05'
  },
  { type: 'mine', text: '写得不错，帮我翻译成英文', time: '09:10' },
  {
    type: 'ai',
    text: 'WeChat AI — Smarter Conversations\n\nWeChat AI is a next-generation chat app that seamlessly blends instant messaging with AI assistance. In a familiar chat experience, you can summon your AI assistant anytime to answer questions, organize information, or even join group discussions.\n\nKey Features:\n• AI in Your Chat — No app-switching; AI lives in your chat list\n• Group Intelligence — Summon AI in groups for meeting notes and action items\n• Floating Assistant — Pop out the AI widget in any conversation\n\nWant me to adjust the tone or add more details?',
    time: '09:10'
  }
]

function autoResize(el) {
  el.style.height = 'auto'
  el.style.height = Math.min(el.scrollHeight, 120) + 'px'
}
</script>

<template>
  <div class="screen active chat-screen">
    <div class="top-bar" style="background:var(--wx-green);color:#fff;border-bottom-color:var(--wx-green)">
      <button class="top-bar-back" @click="store.switchScreen('chatlist')">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="#fff"><path d="M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z" /></svg>
      </button>
      <div class="top-bar-title" style="color:#fff">AI 助手</div>
      <button class="top-bar-action">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="rgba(255,255,255,.8)"><circle cx="12" cy="5" r="2" /><circle cx="12" cy="12" r="2" /><circle cx="12" cy="19" r="2" /></svg>
      </button>
    </div>
    <div class="messages-area" style="background:#F5F7F5">
      <template v-for="(msg, i) in messages" :key="i">
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
              <div class="ai-bubble-header"><span class="ai-badge">AI</span>AI 助手</div>
              <span v-html="msg.text.replace(/\n/g, '<br>')" />
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
      <textarea ref="inputField" class="input-field" placeholder="问 AI 助手任何问题..." rows="1" @input="autoResize($event.target)"></textarea>
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
