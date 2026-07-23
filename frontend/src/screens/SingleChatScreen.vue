<script setup>
import { ref, watch, nextTick } from 'vue'
import store from '../store'

const messagesArea = ref(null)
const fileInput = ref(null)
const previewImage = ref(null)
const showDeleteConfirm = ref(false)

watch(() => store.messages.length, () => {
  nextTick(() => {
    if (messagesArea.value) messagesArea.value.scrollTop = messagesArea.value.scrollHeight
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
  store.sendMessage(text, 'single')
  textarea.value = ''
  textarea.style.height = 'auto'
}

function insertAtAI() {
  const textarea = document.querySelector('.chat-screen textarea')
  if (textarea) {
    textarea.value = '@AI ' + textarea.value
    textarea.focus()
    autoResize(textarea)
  }
}

function triggerImageUpload() {
  if (fileInput.value) fileInput.value.click()
}

function handleImageChange(e) {
  const file = e.target.files[0]
  if (!file) return
  if (!file.type.startsWith('image/')) return
  if (file.size > 2 * 1024 * 1024) {
    alert('图片大小不能超过 2MB')
    return
  }
  const reader = new FileReader()
  reader.onload = (ev) => {
    previewImage.value = ev.target.result
  }
  reader.readAsDataURL(file)
  e.target.value = ''
}

function sendImage() {
  if (!previewImage.value) return
  store.sendMessage(`[图片]${previewImage.value}`, 'single')
  previewImage.value = null
}

function cancelImage() {
  previewImage.value = null
}

function confirmDeleteFriend() {
  showDeleteConfirm.value = true
}

function deleteFriend() {
  store.deleteFriend(store.currentChat.id)
  showDeleteConfirm.value = false
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
        <button class="top-bar-action" @click="confirmDeleteFriend" title="删除好友">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="#FA5151"><path d="M15 16h4v2h-4zm0-8h7v2h-7zm0 4h6v2h-6zM3 18c0 1.1.9 2 2 2h6c1.1 0 2-.9 2-2V8H3v10zM14 5h-3.5l-1-1h-5l-1 1H0v2h14V5z" /></svg>
        </button>
        <button class="top-bar-action" @click="store.switchScreen('ai-settings')">
          <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><circle cx="12" cy="5" r="2" /><circle cx="12" cy="12" r="2" /><circle cx="12" cy="19" r="2" /></svg>
        </button>
      </div>
    </div>

    <div ref="messagesArea" class="messages-area">
      <template v-for="(msg, i) in store.messages" :key="i">
        <!-- 图片消息 -->
        <div v-if="msg.text && msg.text.startsWith('[图片]')" :class="msg.type === 'mine' ? 'msg-row msg-row-mine' : 'msg-row'">
          <div class="msg-avatar-sm" :style="{ background: msg.type === 'mine' ? 'var(--wx-green)' : (msg.color || '#576B95') }">
            {{ msg.type === 'mine' ? '我' : (msg.sender || '?')[0] }}
          </div>
          <div>
            <div class="msg-bubble" :class="msg.type === 'mine' ? 'msg-bubble-mine' : 'msg-bubble-other'">
              <img :src="msg.text.substring(4)" class="msg-image" />
            </div>
            <div class="msg-time" :style="{ textAlign: msg.type === 'mine' ? 'right' : 'left' }">{{ msg.time }}</div>
          </div>
        </div>
        <!-- 文本消息 -->
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
              <div class="ai-bubble-header"><span class="ai-badge">AI</span>{{ msg.name || 'AI助手' }}</div>
              <span style="white-space:pre-line">{{ msg.text }}</span>
              <span v-if="msg.streaming" class="streaming-cursor">▌</span>
            </div>
            <div class="msg-time">{{ msg.time }}</div>
          </div>
        </div>
        <div v-else class="msg-row">
          <div class="msg-avatar-sm" :style="{ background: msg.color || '#576B95' }">{{ (msg.sender || '?')[0] }}</div>
          <div>
            <div class="msg-bubble msg-bubble-other">{{ msg.text }}</div>
            <div class="msg-time">{{ msg.time }}</div>
          </div>
        </div>
      </template>
    </div>

    <!-- 图片预览 -->
    <div v-if="previewImage" class="image-preview-bar">
      <img :src="previewImage" class="preview-thumb" />
      <button class="preview-send" @click="sendImage">发送</button>
      <button class="preview-cancel" @click="cancelImage">取消</button>
    </div>

    <div class="input-bar">
      <input ref="fileInput" type="file" accept="image/*" style="display:none" @change="handleImageChange" />
      <button class="input-bar-btn" @click="triggerImageUpload">
        <svg viewBox="0 0 24 24"><path d="M21 19V5c0-1.1-.9-2-2-2H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2zM8.5 13.5l2.5 3.01L14.5 12l4.5 6H5l3.5-4.5z" /></svg>
      </button>
      <button class="input-bar-btn" title="@AI 助手" @click="insertAtAI">
        <span style="color:var(--wx-green);font-size:13px;font-weight:700">@AI</span>
      </button>
      <textarea class="input-field" placeholder="输入消息..." rows="1" @input="autoResize($event.target)" @keydown.enter.exact.prevent="sendMessage"></textarea>
      <button class="input-send-btn" @click="sendMessage">
        <svg viewBox="0 0 24 24"><path d="M2.01 21L23 12 2.01 3 2 10l15 2-15 2z" /></svg>
      </button>
    </div>

    <!-- 删除好友确认弹窗 -->
    <div v-if="showDeleteConfirm" class="modal-overlay" @click.self="showDeleteConfirm=false">
      <div class="modal-card fade-in">
        <div class="modal-title">删除好友</div>
        <div class="modal-desc">确定要删除好友「{{ store.currentChat.name }}」吗？删除后将从好友列表中移除。</div>
        <div class="modal-actions">
          <button class="modal-btn modal-btn-cancel" @click="showDeleteConfirm=false">取消</button>
          <button class="modal-btn modal-btn-danger" @click="deleteFriend">删除</button>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.chat-screen{background:var(--wx-bg)}
.messages-area{flex:1;overflow-y:auto;padding:16px;display:flex;flex-direction:column;gap:16px}
.streaming-cursor{animation:blink 0.8s infinite;color:var(--wx-green);font-weight:bold}
@keyframes blink{0%,50%{opacity:1}51%,100%{opacity:0}}
.msg-image{max-width:200px;max-height:200px;border-radius:8px;display:block}
.image-preview-bar{display:flex;align-items:center;gap:12px;padding:8px 16px;background:var(--wx-white);border-top:1px solid var(--wx-border)}
.preview-thumb{width:48px;height:48px;object-fit:cover;border-radius:6px}
.preview-send{padding:6px 16px;background:var(--wx-green);color:#fff;border:none;border-radius:6px;font-size:13px;cursor:pointer}
.preview-cancel{padding:6px 16px;background:var(--wx-bg);color:var(--wx-text-secondary);border:1px solid var(--wx-border);border-radius:6px;font-size:13px;cursor:pointer}
.top-bar-actions{display:flex;gap:8px;align-items:center}

.modal-overlay{position:fixed;inset:0;background:rgba(0,0,0,.4);z-index:200;display:flex;align-items:center;justify-content:center;padding:24px}
.modal-card{background:var(--wx-white);border-radius:16px;padding:28px 24px;width:100%;max-width:320px;box-shadow:0 12px 40px rgba(0,0,0,.15)}
.modal-title{font-size:18px;font-weight:600;margin-bottom:12px;text-align:center}
.modal-desc{font-size:14px;color:var(--wx-text-secondary);text-align:center;margin-bottom:20px;line-height:1.5}
.modal-actions{display:flex;gap:12px}
.modal-btn{flex:1;padding:11px;border:none;border-radius:8px;font-size:15px;font-weight:500;cursor:pointer;transition:all .15s}
.modal-btn-cancel{background:var(--wx-bg);color:var(--wx-text-secondary)}
.modal-btn-cancel:hover{background:var(--wx-border)}
.modal-btn-danger{background:#FA5151;color:#fff}
.modal-btn-danger:hover{background:#E04040}
</style>
