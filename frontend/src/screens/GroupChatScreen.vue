<script setup>
import { ref, watch, nextTick, computed, onMounted } from 'vue'
import store from '../store'

const messagesArea = ref(null)
const fileInput = ref(null)
const previewImage = ref(null)
const showMembers = ref(false)
const showDissolveConfirm = ref(false)
const showKickConfirm = ref(false)
const showLeaveConfirm = ref(false)
const kickTargetId = ref('')

// 进入群聊时自动加载成员列表
onMounted(() => {
  store.loadGroupMembers(store.currentChat.id)
})

// 判断当前用户是否是群主
const isCreator = computed(() => {
  const me = store.groupMembers.find(m => m.userId === store.currentUser.userId)
  return me && me.role === 2
})

// 获取角色标签
function getRoleTag(role) {
  if (role === 2) return '群主'
  if (role === 1) return '管理员'
  return ''
}

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
  store.sendMessage(text, 'group')
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
  reader.onload = (ev) => { previewImage.value = ev.target.result }
  reader.readAsDataURL(file)
  e.target.value = ''
}

function sendImage() {
  if (!previewImage.value) return
  store.sendMessage(`[图片]${previewImage.value}`, 'group')
  previewImage.value = null
}

function cancelImage() {
  previewImage.value = null
}

// 打开成员列表
function openMembers() {
  store.loadGroupMembers(store.currentChat.id)
  showMembers.value = true
}

// 踢出成员
function confirmKick(userId) {
  kickTargetId.value = userId
  showKickConfirm.value = true
}

function doKick() {
  store.kickMember(store.currentChat.id, kickTargetId.value)
  showKickConfirm.value = false
  kickTargetId.value = ''
  // 刷新成员列表
  setTimeout(() => store.loadGroupMembers(store.currentChat.id), 500)
}

// 解散群聊
function doDissolve() {
  store.dissolveGroup(store.currentChat.id)
  showDissolveConfirm.value = false
  store.switchScreen('chatlist')
}

// 退出群聊
function doLeave() {
  store.leaveGroup(store.currentChat.id)
  showLeaveConfirm.value = false
  store.switchScreen('chatlist')
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
        <button class="top-bar-action" @click="openMembers" title="群成员">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><path d="M16 11c1.66 0 2.99-1.34 2.99-3S17.66 5 16 5c-1.66 0-3 1.34-3 3s1.34 3 3 3zm-8 0c1.66 0 2.99-1.34 2.99-3S9.66 5 8 5C6.34 5 5 6.34 5 8s1.34 3 3 3zm0 2c-2.33 0-7 1.17-7 3.5V19h14v-2.5c0-2.33-4.67-3.5-7-3.5zm8 0c-.29 0-.62.02-.97.05 1.16.84 1.97 1.97 1.97 3.45V19h6v-2.5c0-2.33-4.67-3.5-7-3.5z" /></svg>
        </button>
        <button class="top-bar-action" @click="store.switchScreen('ai-settings')">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><circle cx="12" cy="5" r="2" /><circle cx="12" cy="12" r="2" /><circle cx="12" cy="19" r="2" /></svg>
        </button>
        <!-- 退出/解散群聊按钮 -->
        <button v-if="isCreator" class="top-bar-action top-bar-action-danger" @click="showDissolveConfirm=true" title="解散群聊">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="#FA5151"><path d="M15 16h4v2h-4zm0-8h7v2h-7zm0 4h6v2h-6zM3 18c0 1.1.9 2 2 2h6c1.1 0 2-.9 2-2V8H3v10zM14 5h-3.5l-1-1h-5l-1 1H0v2h14V5z" /></svg>
        </button>
        <button v-if="!isCreator" class="top-bar-action top-bar-action-danger" @click="showLeaveConfirm=true" title="退出群聊">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="#FA5151"><path d="M10.09 15.59L11.5 17l5-5-5-5-1.41 1.41L12.67 11H3v2h9.67l-2.58 2.59zM19 3H5c-1.11 0-2 .9-2 2v4h2V5h14v14H5v-4H3v4c0 1.1.89 2 2 2h14c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2z" /></svg>
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
            <div v-if="msg.type !== 'mine'" class="msg-sender">{{ msg.name || msg.sender }}</div>
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
            <div class="msg-sender" style="color:var(--wx-green)">{{ msg.name || 'AI助手' }} <span class="tag-ai">AI</span></div>
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
            <div class="msg-sender">{{ msg.name || msg.sender }}</div>
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

    <!-- 群成员列表弹窗 -->
    <div v-if="showMembers" class="modal-overlay" @click.self="showMembers=false">
      <div class="modal-card modal-card-lg fade-in">
        <div class="modal-title">群成员 ({{ store.groupMembers.length }})</div>
        <div class="members-list">
          <div v-for="member in store.groupMembers" :key="member.userId" class="member-item">
            <div class="member-avatar" :style="{ background: member.color }">
              {{ (member.nickname || member.username || '?')[0] }}
            </div>
            <div class="member-info">
              <div class="member-name">
                {{ member.nickname || member.username }}
                <span v-if="member.role === 2" class="role-tag role-creator">群主</span>
                <span v-else-if="member.role === 1" class="role-tag role-admin">管理员</span>
              </div>
              <div class="member-id">ID: {{ member.userId }}</div>
            </div>
            <button v-if="isCreator && member.userId !== store.currentUser.userId" 
                    class="kick-btn" @click="confirmKick(member.userId)">踢出</button>
          </div>
        </div>
        <div class="modal-actions">
          <button class="modal-btn modal-btn-cancel" style="flex:1" @click="showMembers=false">关闭</button>
        </div>
      </div>
    </div>

    <!-- 踢出确认弹窗 -->
    <div v-if="showKickConfirm" class="modal-overlay" @click.self="showKickConfirm=false">
      <div class="modal-card fade-in">
        <div class="modal-title">踢出成员</div>
        <div class="modal-desc">确定要将该成员踢出群聊吗？</div>
        <div class="modal-actions">
          <button class="modal-btn modal-btn-cancel" @click="showKickConfirm=false">取消</button>
          <button class="modal-btn modal-btn-danger" @click="doKick">踢出</button>
        </div>
      </div>
    </div>

    <!-- 解散群聊确认弹窗 -->
    <div v-if="showDissolveConfirm" class="modal-overlay" @click.self="showDissolveConfirm=false">
      <div class="modal-card fade-in">
        <div class="modal-title">解散群聊</div>
        <div class="modal-desc">确定要解散群聊「{{ store.currentChat.name }}」吗？解散后所有成员将被移除，此操作不可撤销。</div>
        <div class="modal-actions">
          <button class="modal-btn modal-btn-cancel" @click="showDissolveConfirm=false">取消</button>
          <button class="modal-btn modal-btn-danger" @click="doDissolve">解散</button>
        </div>
      </div>
    </div>

    <!-- 退出群聊确认弹窗 -->
    <div v-if="showLeaveConfirm" class="modal-overlay" @click.self="showLeaveConfirm=false">
      <div class="modal-card fade-in">
        <div class="modal-title">退出群聊</div>
        <div class="modal-desc">确定要退出群聊「{{ store.currentChat.name }}」吗？</div>
        <div class="modal-actions">
          <button class="modal-btn modal-btn-cancel" @click="showLeaveConfirm=false">取消</button>
          <button class="modal-btn modal-btn-danger" @click="doLeave">退出</button>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.chat-screen{background:var(--wx-bg);position:relative}
.messages-area{flex:1;overflow-y:auto;padding:16px;display:flex;flex-direction:column;gap:16px}
.streaming-cursor{animation:blink 0.8s infinite;color:var(--wx-green);font-weight:bold}
@keyframes blink{0%,50%{opacity:1}51%,100%{opacity:0}}
.tag-ai{font-size:10px;padding:1px 5px;border-radius:3px;font-weight:500;background:rgba(7,193,96,.12);color:var(--wx-green)}
.msg-image{max-width:200px;max-height:200px;border-radius:8px;display:block}
.image-preview-bar{display:flex;align-items:center;gap:12px;padding:8px 16px;background:var(--wx-white);border-top:1px solid var(--wx-border)}
.preview-thumb{width:48px;height:48px;object-fit:cover;border-radius:6px}
.preview-send{padding:6px 16px;background:var(--wx-green);color:#fff;border:none;border-radius:6px;font-size:13px;cursor:pointer}
.preview-cancel{padding:6px 16px;background:var(--wx-bg);color:var(--wx-text-secondary);border:1px solid var(--wx-border);border-radius:6px;font-size:13px;cursor:pointer}
.top-bar-actions{display:flex;gap:8px;align-items:center}
.top-bar-action-danger{margin-left:4px}

/* 弹窗 */
.modal-overlay{position:fixed;inset:0;background:rgba(0,0,0,.4);z-index:200;display:flex;align-items:center;justify-content:center;padding:24px}
.modal-card{background:var(--wx-white);border-radius:16px;padding:28px 24px;width:100%;max-width:320px;box-shadow:0 12px 40px rgba(0,0,0,.15)}
.modal-card-lg{max-width:380px;max-height:70vh;display:flex;flex-direction:column}
.modal-title{font-size:18px;font-weight:600;margin-bottom:12px;text-align:center}
.modal-desc{font-size:14px;color:var(--wx-text-secondary);text-align:center;margin-bottom:20px;line-height:1.5}
.modal-actions{display:flex;gap:12px;margin-top:16px}
.modal-btn{flex:1;padding:11px;border:none;border-radius:8px;font-size:15px;font-weight:500;cursor:pointer;transition:all .15s}
.modal-btn-cancel{background:var(--wx-bg);color:var(--wx-text-secondary)}
.modal-btn-cancel:hover{background:var(--wx-border)}
.modal-btn-danger{background:#FA5151;color:#fff}
.modal-btn-danger:hover{background:#E04040}

/* 成员列表 */
.members-list{flex:1;overflow-y:auto;margin:0 -24px;padding:0 24px}
.member-item{display:flex;align-items:center;gap:12px;padding:12px 0;border-bottom:1px solid var(--wx-border)}
.member-item:last-child{border-bottom:none}
.member-avatar{width:40px;height:40px;border-radius:50%;display:flex;align-items:center;justify-content:center;color:#fff;font-size:16px;font-weight:500;flex-shrink:0}
.member-info{flex:1;min-width:0}
.member-name{font-size:14px;font-weight:500;display:flex;align-items:center;gap:6px}
.member-id{font-size:12px;color:var(--wx-text-secondary);margin-top:2px}
.role-tag{font-size:10px;padding:1px 6px;border-radius:3px;font-weight:500}
.role-creator{background:rgba(250,130,0,.1);color:#FF8200}
.role-admin{background:rgba(7,193,96,.1);color:var(--wx-green)}
.kick-btn{padding:6px 14px;background:rgba(250,81,81,.1);color:#FA5151;border:1px solid rgba(250,81,81,.2);border-radius:6px;font-size:13px;cursor:pointer;transition:all .15s;flex-shrink:0}
.kick-btn:hover{background:rgba(250,81,81,.2)}
</style>
