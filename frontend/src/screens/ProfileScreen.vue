<script setup>
import { ref } from 'vue'
import store from '../store'

const showEditProfile = ref(false)
const editUsername = ref('')
const editNickname = ref('')

function openEditProfile() {
  editUsername.value = store.currentUser.username
  editNickname.value = store.currentUser.nickname || ''
  showEditProfile.value = true
}

function saveProfile() {
  if (editUsername.value.trim()) {
    store.updateProfile({
      username: editUsername.value.trim(),
      nickname: editNickname.value.trim()
    })
  }
  showEditProfile.value = false
}
</script>

<template>
  <div class="screen active">
    <div class="top-bar">
      <div class="top-bar-title">个人信息</div>
      <button class="top-bar-action" @click="openEditProfile">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04c.39-.39.39-1.02 0-1.41l-2.34-2.34c-.39-.39-1.02-.39-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z" /></svg>
      </button>
    </div>
    <div class="profile-card">
      <div class="profile-header">
        <div class="profile-avatar" :style="{ background: store.currentUser.avatarPath ? 'transparent' : 'var(--wx-green)' }">
          <img v-if="store.currentUser.avatarPath" :src="store.currentUser.avatarPath" alt="avatar">
          <span v-else>{{ (store.currentUser.username || store.currentUser.userId || '我')[0] }}</span>
        </div>
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
          <div class="profile-stat-num">{{ store.recentChats.filter(c => c.type === 'ai').length }}</div>
          <div class="profile-stat-label">AI 对话</div>
        </div>
      </div>
      <div class="profile-actions">
        <div class="profile-action" @click="openEditProfile">
          <svg viewBox="0 0 24 24"><path d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z" /></svg>
          <span>个人信息</span>
          <span class="arrow">›</span>
        </div>
        <div class="profile-action">
          <svg viewBox="0 0 24 24"><path d="M18 16.08c-.76 0-1.44.3-1.96.77L8.91 12.7c.05-.23.09-.46.09-.7s-.04-.47-.09-.7l7.05-4.11c.54.5 1.25.81 2.04.81 1.66 0 3-1.34 3-3s-1.34-3-3-3-3 1.34-3 3c0 .24.04.47.09.7L8.04 9.81C7.5 9.31 6.79 9 6 9c-1.66 0-3 1.34-3 3s1.34 3 3 3c.79 0 1.5-.31 2.04-.81l7.12 4.16c-.05.21-.08.43-.08.65 0 1.61 1.31 2.92 2.92 2.92 1.61 0 2.92-1.31 2.92-2.92s-1.31-2.92-2.92-2.92z" /></svg>
          <span>分享名片</span>
          <span class="arrow">›</span>
        </div>
        <div class="profile-action">
          <svg viewBox="0 0 24 24"><path d="M12 22c1.1 0 2-.9 2-2h-4c0 1.1.89 2 2 2zm6-6v-5c0-3.07-1.64-5.64-4.5-6.32V4c0-.83-.67-1.5-1.5-1.5s-1.5.67-1.5 1.5v.68C7.63 5.36 6 7.92 6 11v5l-2 2v1h16v-1l-2-2z" /></svg>
          <span>消息通知</span>
          <span class="arrow">›</span>
        </div>
        <div class="profile-action" @click="store.openChat('ai', 'ai', 'AI 助手')">
          <span class="ai-badge" style="width:24px;height:24px;font-size:11px;border-radius:6px">AI</span>
          <span style="color:var(--wx-green)">AI 助手</span>
          <span class="arrow">›</span>
        </div>
        <div class="profile-action" @click="store.logout()" style="color:var(--wx-danger)">
          <svg viewBox="0 0 24 24"><path d="M17 7l-1.41 1.41L18.17 11H8v2h10.17l-2.58 2.58L17 17l5-5zM4 5h8V3H4c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h8v-2H4V5z" /></svg>
          <span>退出登录</span>
          <span class="arrow">›</span>
        </div>
      </div>
    </div>

    <!-- 编辑个人信息弹窗 -->
    <div v-if="showEditProfile" class="modal-overlay" @click.self="showEditProfile=false">
      <div class="modal-card fade-in">
        <div class="modal-title">编辑个人信息</div>
        <div class="edit-avatar-section">
          <div class="edit-avatar">
            {{ (store.currentUser.username || '我')[0] }}
          </div>
          <div class="edit-avatar-hint">点击更换头像</div>
        </div>
        <div class="login-field">
          <label>用户名</label>
          <input v-model="editUsername" type="text" placeholder="请输入用户名">
        </div>
        <div class="login-field">
          <label>昵称（选填）</label>
          <input v-model="editNickname" type="text" placeholder="请输入昵称">
        </div>
        <div class="modal-actions">
          <button class="modal-btn modal-btn-cancel" @click="showEditProfile=false">取消</button>
          <button class="modal-btn modal-btn-confirm" @click="saveProfile">保存</button>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.profile-card{padding:24px 16px;display:flex;flex-direction:column;gap:20px}
.profile-header{display:flex;align-items:center;gap:16px}
.profile-avatar{
  width:64px;height:64px;border-radius:var(--wx-radius-lg);background:var(--wx-green);
  display:flex;align-items:center;justify-content:center;font-size:28px;font-weight:700;color:#fff;
  overflow:hidden;
}
.profile-avatar img{width:100%;height:100%;object-fit:cover}
.profile-name{font-size:20px;font-weight:600}
.profile-id{font-size:13px;color:var(--wx-text-secondary)}
.profile-stats{display:flex;gap:0;border:1px solid var(--wx-border);border-radius:var(--wx-radius-lg);overflow:hidden}
.profile-stat{flex:1;text-align:center;padding:16px 8px;border-right:1px solid var(--wx-border)}
.profile-stat:last-child{border-right:none}
.profile-stat-num{font-size:20px;font-weight:700;color:var(--wx-green)}
.profile-stat-label{font-size:12px;color:var(--wx-text-secondary);margin-top:2px}
.profile-actions{display:flex;flex-direction:column;gap:8px}
.profile-action{
  display:flex;align-items:center;gap:12px;padding:14px 16px;
  background:var(--wx-white);border-radius:var(--wx-radius);border:1px solid var(--wx-border);
  cursor:pointer;transition:all .15s;font-size:15px;
}
.profile-action+.profile-action{margin-top:-1px;border-top:none}
.profile-action:hover{border-color:var(--wx-green)}
.profile-action svg{width:20px;height:20px;fill:var(--wx-text-secondary)}
.arrow{margin-left:auto;color:var(--wx-text-tertiary)}

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
.edit-avatar-section{display:flex;flex-direction:column;align-items:center;margin-bottom:20px}
.edit-avatar{
  width:72px;height:72px;border-radius:var(--wx-radius-lg);background:var(--wx-green);
  display:flex;align-items:center;justify-content:center;font-size:32px;font-weight:700;color:#fff;
  cursor:pointer;transition:all .15s;
}
.edit-avatar:hover{opacity:0.9}
.edit-avatar-hint{font-size:12px;color:var(--wx-text-secondary);margin-top:8px}
.modal-card .login-field{margin-bottom:14px}
.modal-card .login-field label{display:block;font-size:13px;color:var(--wx-text-secondary);margin-bottom:6px}
.modal-card .login-field input{
  width:100%;padding:10px 12px;border:1px solid rgba(0,0,0,.1);border-radius:8px;
  font-size:14px;outline:none;font-family:var(--font-body);background:rgba(255,255,255,.7);
}
.modal-card .login-field input:focus{border-color:var(--wx-green)}
.modal-actions{display:flex;gap:12px;margin-top:20px}
.modal-btn{
  flex:1;padding:11px;border:none;border-radius:8px;font-size:15px;font-weight:500;
  cursor:pointer;transition:all .15s;font-family:var(--font-body);
}
.modal-btn-cancel{background:var(--wx-bg);color:var(--wx-text-secondary)}
.modal-btn-cancel:hover{background:var(--wx-border)}
.modal-btn-confirm{background:var(--wx-green);color:#fff}
.modal-btn-confirm:hover{background:var(--wx-green-dark)}
</style>
