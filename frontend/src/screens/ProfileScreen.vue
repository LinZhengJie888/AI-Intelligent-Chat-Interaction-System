<script setup>
import { ref, onMounted } from 'vue'
import store from '../store'

const showEdit = ref(false)
const editUsername = ref('')
const fileInput = ref(null)

function openEdit() {
  editUsername.value = store.currentUser.username || ''
  showEdit.value = true
  store.clearMessages()
}

function saveProfile() {
  if (!editUsername.value.trim()) return
  store.updateProfile({ username: editUsername.value.trim() })
  showEdit.value = false
}

function triggerAvatarUpload() {
  if (fileInput.value) fileInput.value.click()
}

function handleAvatarChange(e) {
  const file = e.target.files[0]
  if (!file) return

  // 检查文件类型
  if (!file.type.startsWith('image/')) {
    store.errorMessage = '请选择图片文件'
    return
  }

  // 检查文件大小（限制 5MB）
  if (file.size > 5 * 1024 * 1024) {
    store.errorMessage = '图片大小不能超过 5MB'
    return
  }

  // 压缩图片后再上传
  const reader = new FileReader()
  reader.onload = (ev) => {
    const img = new Image()
    img.onload = () => {
      const canvas = document.createElement('canvas')
      const size = 200 // 头像尺寸 200x200
      canvas.width = size
      canvas.height = size
      const ctx = canvas.getContext('2d')
      
      // 居中裁剪
      const min = Math.min(img.width, img.height)
      const sx = (img.width - min) / 2
      const sy = (img.height - min) / 2
      ctx.drawImage(img, sx, sy, min, min, 0, 0, size, size)
      
      const compressed = canvas.toDataURL('image/jpeg', 0.8)
      store.uploadAvatar(compressed, 'jpg')
      store.currentUser.avatarPath = compressed
      localStorage.setItem('user-info', JSON.stringify(store.currentUser))
    }
    img.src = ev.target.result
  }
  reader.readAsDataURL(file)
  e.target.value = ''
}
</script>

<template>
  <div class="screen active">
    <div class="top-bar">
      <div class="top-bar-title">个人信息</div>
      <button class="top-bar-action" @click="openEdit">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04c.39-.39.39-1.02 0-1.41l-2.34-2.34c-.39-.39-1.02-.39-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z" /></svg>
      </button>
    </div>

    <!-- 提示信息 -->
    <div v-if="store.successMessage" class="toast-success">{{ store.successMessage }}</div>
    <div v-if="store.errorMessage" class="toast-error">{{ store.errorMessage }}</div>

    <div class="profile-card">
      <div class="profile-header">
        <div class="profile-avatar-wrap" @click="triggerAvatarUpload">
          <img v-if="store.currentUser.avatarPath" :src="store.currentUser.avatarPath" class="profile-avatar-img" />
          <div v-else class="profile-avatar">{{ (store.currentUser.username || store.currentUser.userId || '我')[0] }}</div>
          <div class="avatar-overlay">更换头像</div>
        </div>
        <input ref="fileInput" type="file" accept="image/*" style="display:none" @change="handleAvatarChange" />
        <div>
          <div class="profile-name">{{ store.currentUser.username || '用户' }}</div>
          <div class="profile-id">用户ID: {{ store.currentUser.userId || '-' }}</div>
        </div>
      </div>
      <div class="profile-stats">
        <div class="profile-stat">
          <div class="profile-stat-num">{{ store.friends.length }}</div>
          <div class="profile-stat-label">好友</div>
        </div>
        <div class="profile-stat">
          <div class="profile-stat-num">{{ store.groups.length }}</div>
          <div class="profile-stat-label">群聊</div>
        </div>
        <div class="profile-stat">
          <div class="profile-stat-num">1</div>
          <div class="profile-stat-label">AI 对话</div>
        </div>
      </div>
      <div class="profile-actions">
        <div class="profile-action" @click="openEdit">
          <svg viewBox="0 0 24 24"><path d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z" /></svg>
          <span>个人信息</span>
          <span class="arrow">›</span>
        </div>
        <div class="profile-action" @click="store.openChat('ai', 'ai', 'AI 助手')">
          <span class="ai-badge" style="width:24px;height:24px;font-size:11px;border-radius:6px">AI</span>
          <span style="color:var(--wx-green)">AI 助手</span>
          <span class="arrow">›</span>
        </div>
        <div class="profile-action" @click="store.switchScreen('ai-settings')">
          <svg viewBox="0 0 24 24"><path d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.07.62-.07.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z" /></svg>
          <span>AI 设置</span>
          <span class="arrow">›</span>
        </div>
        <div class="profile-action" @click="store.logout()" style="color:var(--wx-danger)">
          <svg viewBox="0 0 24 24"><path d="M17 7l-1.41 1.41L18.17 11H8v2h10.17l-2.58 2.58L17 17l5-5zM4 5h8V3H4c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h8v-2H4V5z" /></svg>
          <span>退出登录</span>
          <span class="arrow">›</span>
        </div>
      </div>
    </div>

    <!-- 编辑弹窗 -->
    <div v-if="showEdit" class="modal-overlay" @click.self="showEdit=false">
      <div class="modal-card fade-in">
        <div class="modal-title">编辑个人信息</div>
        <div class="login-field">
          <label>用户名</label>
          <input v-model="editUsername" type="text" placeholder="请输入用户名" maxlength="20">
        </div>
        <div class="login-field">
          <label>用户ID</label>
          <input type="text" :value="store.currentUser.userId" disabled style="opacity:.6">
        </div>
        <div class="modal-actions">
          <button class="modal-btn modal-btn-cancel" @click="showEdit=false">取消</button>
          <button class="modal-btn modal-btn-confirm" @click="saveProfile">保存</button>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.profile-card{padding:24px 16px;display:flex;flex-direction:column;gap:20px}
.profile-header{display:flex;align-items:center;gap:16px}
.profile-avatar-wrap{position:relative;cursor:pointer;width:64px;height:64px}
.profile-avatar{width:64px;height:64px;border-radius:var(--wx-radius-lg);background:var(--wx-green);display:flex;align-items:center;justify-content:center;font-size:28px;font-weight:700;color:#fff}
.profile-avatar-img{width:64px;height:64px;border-radius:var(--wx-radius-lg);object-fit:cover}
.avatar-overlay{position:absolute;inset:0;background:rgba(0,0,0,.5);border-radius:var(--wx-radius-lg);display:flex;align-items:center;justify-content:center;color:#fff;font-size:11px;opacity:0;transition:opacity .2s}
.profile-avatar-wrap:hover .avatar-overlay{opacity:1}
.profile-name{font-size:20px;font-weight:600}
.profile-id{font-size:13px;color:var(--wx-text-secondary)}
.profile-stats{display:flex;gap:0;border:1px solid var(--wx-border);border-radius:var(--wx-radius-lg);overflow:hidden}
.profile-stat{flex:1;text-align:center;padding:16px 8px;border-right:1px solid var(--wx-border)}
.profile-stat:last-child{border-right:none}
.profile-stat-num{font-size:20px;font-weight:700;color:var(--wx-green)}
.profile-stat-label{font-size:12px;color:var(--wx-text-secondary);margin-top:2px}
.profile-actions{display:flex;flex-direction:column;gap:8px}
.profile-action{display:flex;align-items:center;gap:12px;padding:14px 16px;background:var(--wx-white);border-radius:var(--wx-radius);border:1px solid var(--wx-border);cursor:pointer;transition:all .15s;font-size:15px}
.profile-action+.profile-action{margin-top:-1px;border-top:none}
.profile-action:hover{border-color:var(--wx-green)}
.profile-action svg{width:20px;height:20px;fill:var(--wx-text-secondary)}
.arrow{margin-left:auto;color:var(--wx-text-tertiary)}

.toast-success{margin:0 16px 8px;padding:8px 12px;background:rgba(7,193,96,.1);color:var(--wx-green);border-radius:8px;font-size:13px;text-align:center}
.toast-error{margin:0 16px 8px;padding:8px 12px;background:rgba(250,81,81,.1);color:#FA5151;border-radius:8px;font-size:13px;text-align:center}

.modal-overlay{position:fixed;inset:0;background:rgba(0,0,0,.4);z-index:200;display:flex;align-items:center;justify-content:center;padding:24px}
.modal-card{background:var(--wx-white);border-radius:16px;padding:28px 24px;width:100%;max-width:380px;box-shadow:0 12px 40px rgba(0,0,0,.15)}
.modal-title{font-size:18px;font-weight:600;margin-bottom:20px;text-align:center}
.modal-card .login-field{margin-bottom:14px}
.modal-card .login-field label{display:block;font-size:13px;color:var(--wx-text-secondary);margin-bottom:6px}
.modal-card .login-field input{width:100%;padding:10px 12px;border:1px solid rgba(0,0,0,.1);border-radius:8px;font-size:14px;outline:none;font-family:var(--font-body);background:rgba(255,255,255,.7)}
.modal-card .login-field input:focus{border-color:var(--wx-green)}
.modal-actions{display:flex;gap:12px;margin-top:20px}
.modal-btn{flex:1;padding:11px;border:none;border-radius:8px;font-size:15px;font-weight:500;cursor:pointer;transition:all .15s;font-family:var(--font-body)}
.modal-btn-cancel{background:var(--wx-bg);color:var(--wx-text-secondary)}
.modal-btn-confirm{background:var(--wx-green);color:#fff}
</style>
