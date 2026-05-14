<script setup>
import { ref, onMounted } from 'vue'
import store from '../store'

const showAddFriend = ref(false)
const addFriendId = ref('')
const addFriendMsg = ref('')

function openAddFriend() {
  showAddFriend.value = true
  addFriendId.value = ''
  addFriendMsg.value = ''
  store.clearMessages()
}

function submitAddFriend() {
  if (!addFriendId.value.trim()) return
  store.addFriend(addFriendId.value.trim(), addFriendMsg.value)
  showAddFriend.value = false
  addFriendId.value = ''
  addFriendMsg.value = ''
}

onMounted(() => {
  store.loadFriendRequests()
  store.loadFriends()
})
</script>

<template>
  <div class="screen active">
    <div class="top-bar">
      <div class="top-bar-title">通讯录</div>
      <button class="top-bar-action" @click="openAddFriend">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><path d="M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z" /></svg>
      </button>
    </div>

    <!-- 提示信息 -->
    <div v-if="store.successMessage" class="toast-success">{{ store.successMessage }}</div>
    <div v-if="store.errorMessage" class="toast-error">{{ store.errorMessage }}</div>

    <div class="contacts-content" style="flex:1;overflow-y:auto">
      <div class="search-box" style="margin:12px 16px">
        <svg viewBox="0 0 24 24"><path d="M15.5 14h-.79l-.28-.27A6.471 6.471 0 0016 9.5 6.5 6.5 0 109.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z" /></svg>
        <input type="text" placeholder="搜索联系人">
      </div>

      <!-- 添加好友入口 -->
      <div class="contact-item" @click="openAddFriend">
        <div class="chat-avatar" style="background:var(--wx-green);border-radius:8px">
          <svg width="22" height="22" viewBox="0 0 24 24" fill="#fff"><path d="M15 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm-9-2V7H4v3H1v2h3v3h2v-3h3v-2H6zm9 4c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z" /></svg>
        </div>
        <div class="chat-info">
          <div class="chat-name">添加好友</div>
          <div class="chat-preview">输入用户ID发送好友请求</div>
        </div>
      </div>

      <!-- 好友请求区域 -->
      <div v-if="store.friendRequests.length > 0">
        <div class="contact-header">
          好友请求
          <span class="request-badge">{{ store.friendRequests.length }}</span>
        </div>
        <div v-for="req in store.friendRequests" :key="req.fromUserId" class="contact-item friend-request-item">
          <div class="chat-avatar chat-avatar-user" :style="{ background: req.color }">
            {{ (req.fromUsername || req.fromUserId)[0] }}
          </div>
          <div class="chat-info">
            <div class="chat-name">{{ req.fromUsername || req.fromUserId }}</div>
            <div class="chat-preview">{{ req.requestMsg || '请求加你为好友' }}</div>
          </div>
          <div class="friend-request-actions">
            <button class="btn-agree" @click="store.agreeFriend(req.fromUserId)">同意</button>
            <button class="btn-reject" @click="store.rejectFriend(req.fromUserId)">拒绝</button>
          </div>
        </div>
      </div>

      <!-- 好友列表 -->
      <div class="contact-header">好友 ({{ store.friends.length }})</div>
      <div v-if="store.friends.length === 0" class="empty-hint">暂无好友，点击上方添加</div>
      <div v-for="friend in store.friends" :key="friend.userId" class="contact-item" @click="store.openChat('single', friend.userId, friend.username)">
        <div class="chat-avatar chat-avatar-user" :style="{ background: friend.color }">
          {{ (friend.username || friend.userId)[0] }}
        </div>
        <div class="chat-info">
          <div class="chat-name">{{ friend.username || friend.userId }}</div>
        </div>
      </div>

      <div style="height:60px"></div>
    </div>

    <!-- 添加好友弹窗 -->
    <div v-if="showAddFriend" class="modal-overlay" @click.self="showAddFriend=false">
      <div class="modal-card fade-in">
        <div class="modal-title">添加好友</div>
        <div class="login-field">
          <label>对方用户ID</label>
          <input v-model="addFriendId" type="text" placeholder="请输入对方用户ID">
        </div>
        <div class="login-field">
          <label>验证消息（选填）</label>
          <input v-model="addFriendMsg" type="text" placeholder="请输入验证消息">
        </div>
        <div class="modal-actions">
          <button class="modal-btn modal-btn-cancel" @click="showAddFriend=false">取消</button>
          <button class="modal-btn modal-btn-confirm" @click="submitAddFriend">发送请求</button>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.contact-header{
  font-size:13px;color:var(--wx-text-secondary);padding:8px 16px;font-weight:500;
  background:var(--wx-bg);position:sticky;top:0;display:flex;align-items:center;gap:6px;
}
.contact-item{display:flex;align-items:center;gap:12px;padding:10px 16px;cursor:pointer;border-bottom:1px solid var(--wx-border)}
.contact-item:hover{background:rgba(0,0,0,.02)}
.empty-hint{padding:20px 16px;text-align:center;font-size:13px;color:var(--wx-text-tertiary)}
.request-badge{
  display:inline-flex;align-items:center;justify-content:center;
  min-width:18px;height:18px;padding:0 5px;border-radius:9px;
  background:var(--wx-danger);color:#fff;font-size:11px;font-weight:600;
}

.toast-success{margin:0 16px 8px;padding:8px 12px;background:rgba(7,193,96,.1);color:var(--wx-green);border-radius:8px;font-size:13px;text-align:center}
.toast-error{margin:0 16px 8px;padding:8px 12px;background:rgba(250,81,81,.1);color:#FA5151;border-radius:8px;font-size:13px;text-align:center}

.friend-request-item{background:rgba(7,193,96,.02)}
.friend-request-actions{display:flex;gap:8px;margin-left:auto;flex-shrink:0}
.btn-agree{padding:6px 16px;background:var(--wx-green);color:#fff;border:none;border-radius:6px;font-size:13px;cursor:pointer}
.btn-agree:hover{background:var(--wx-green-dark)}
.btn-reject{padding:6px 16px;background:var(--wx-bg);color:var(--wx-text-secondary);border:1px solid var(--wx-border);border-radius:6px;font-size:13px;cursor:pointer}
.btn-reject:hover{border-color:var(--wx-danger);color:var(--wx-danger)}

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
.modal-btn-cancel:hover{background:var(--wx-border)}
.modal-btn-confirm{background:var(--wx-green);color:#fff}
.modal-btn-confirm:hover{background:var(--wx-green-dark)}
</style>
