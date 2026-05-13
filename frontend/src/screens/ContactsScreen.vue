<script setup>
import { ref, computed } from 'vue'
import store from '../store'

const showAddFriend = ref(false)
const showFriendRequests = ref(false)
const addFriendId = ref('')
const addFriendMsg = ref('')
const searchText = ref('')

const sortedFriends = computed(() => {
  const keyword = searchText.value.trim().toLowerCase()
  const list = keyword
    ? store.friends.filter(f => f.username.toLowerCase().includes(keyword) || f.userId.toLowerCase().includes(keyword))
    : store.friends
  return [...list].sort((a, b) => a.username.localeCompare(b.username, 'zh-CN'))
})

const groupedFriends = computed(() => {
  const groups = {}
  sortedFriends.value.forEach(friend => {
    const firstChar = friend.username[0].toUpperCase()
    const letter = /[A-Z]/.test(firstChar) ? firstChar : '#'
    if (!groups[letter]) groups[letter] = []
    groups[letter].push(friend)
  })
  return Object.entries(groups).sort(([a], [b]) => a.localeCompare(b))
})

function openAddFriend() {
  showAddFriend.value = true
  addFriendId.value = ''
  addFriendMsg.value = ''
}

function submitAddFriend() {
  if (!addFriendId.value.trim()) return
  // 通过WebSocket发送好友请求
  store.addFriend(addFriendId.value, addFriendMsg.value || '请求添加好友')
  showAddFriend.value = false
  addFriendId.value = ''
  addFriendMsg.value = ''
}

function agreeFriend(userId) {
  const request = store.friendRequests.find(r => r.fromUserId === userId)
  if (request) {
    store.friends.push({
      userId: request.fromUserId,
      username: request.fromUsername,
      avatarPath: '',
      color: request.color || '#576B95'
    })
  }
  store.agreeFriend(userId)
}

function rejectFriend(userId) {
  store.rejectFriend(userId)
}
</script>

<template>
  <div class="screen active">
    <div class="top-bar">
      <div class="top-bar-title">通讯录</div>
      <button class="top-bar-action" @click="openAddFriend">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><path d="M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z" /></svg>
      </button>
    </div>
    <div class="contacts-content" style="flex:1;overflow-y:auto">
      <div class="search-box" style="margin:12px 16px">
        <svg viewBox="0 0 24 24"><path d="M15.5 14h-.79l-.28-.27A6.471 6.471 0 0016 9.5 6.5 6.5 0 109.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z" /></svg>
        <input v-model="searchText" type="text" placeholder="搜索联系人">
      </div>

      <div class="contact-header">新的朋友</div>
      <div class="contact-item" @click="showFriendRequests = true">
        <div class="chat-avatar" style="background:var(--wx-green);border-radius:8px">
          <svg width="22" height="22" viewBox="0 0 24 24" fill="#fff"><path d="M15 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm-9-2V7H4v3H1v2h3v3h2v-3h3v-2H6zm9 4c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z" /></svg>
        </div>
        <div class="chat-info">
          <div class="chat-name">新的朋友</div>
          <div class="chat-preview">{{ store.friendRequests.length > 0 ? store.friendRequests.length + ' 条好友请求' : '点击添加好友' }}</div>
        </div>
        <span v-if="store.friendRequests.length > 0" class="request-badge">{{ store.friendRequests.length }}</span>
      </div>

      <template v-for="[letter, friends] in groupedFriends" :key="letter">
        <div class="contact-header">{{ letter }}</div>
        <div v-for="friend in friends" :key="friend.userId" class="contact-item" @click="store.openChat('single', friend.userId, friend.username)">
          <div class="chat-avatar chat-avatar-user" :style="{ background: friend.color || '#576B95' }">{{ friend.username[0] }}</div>
          <div class="chat-info">
            <div class="chat-name">{{ friend.username }}</div>
          </div>
        </div>
      </template>

      <div v-if="sortedFriends.length === 0 && searchText" style="padding:40px 16px;text-align:center;color:var(--wx-text-secondary)">
        未找到匹配的联系人
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

    <!-- 好友请求列表弹窗 -->
    <div v-if="showFriendRequests" class="modal-overlay" @click.self="showFriendRequests=false">
      <div class="modal-card fade-in" style="max-width:420px">
        <div class="modal-title">好友请求</div>
        <div v-if="store.friendRequests.length === 0" style="padding:20px;text-align:center;color:var(--wx-text-secondary)">
          暂无好友请求
        </div>
        <div v-else class="requests-list">
          <div v-for="request in store.friendRequests" :key="request.fromUserId" class="request-item">
            <div class="request-avatar" :style="{ background: request.color || '#576B95' }">{{ request.fromUsername[0] }}</div>
            <div class="request-info">
              <div class="request-name">{{ request.fromUsername }}</div>
              <div class="request-id">ID: {{ request.fromUserId }}</div>
              <div class="request-msg">{{ request.requestMsg }}</div>
            </div>
            <div class="request-actions">
              <button class="request-btn request-btn-agree" @click="agreeFriend(request.fromUserId)">同意</button>
              <button class="request-btn request-btn-reject" @click="rejectFriend(request.fromUserId)">拒绝</button>
            </div>
          </div>
        </div>
        <div class="modal-actions">
          <button class="modal-btn modal-btn-confirm" @click="showFriendRequests=false">关闭</button>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.contact-header{
  font-size:13px;color:var(--wx-text-secondary);padding:8px 16px;font-weight:500;
  background:var(--wx-bg);position:sticky;top:0;
}
.contact-item{
  display:flex;align-items:center;gap:12px;padding:10px 16px;cursor:pointer;
  border-bottom:1px solid var(--wx-border);
}
.contact-item:hover{background:rgba(0,0,0,.02)}
.request-badge{
  margin-left:auto;width:20px;height:20px;background:var(--wx-danger);color:#fff;border-radius:50%;
  font-size:12px;display:flex;align-items:center;justify-content:center;font-weight:500;
}

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

/* 好友请求列表 */
.requests-list{max-height:300px;overflow-y:auto;display:flex;flex-direction:column;gap:12px}
.request-item{display:flex;align-items:flex-start;gap:12px;padding:12px;background:var(--wx-bg);border-radius:8px}
.request-avatar{
  width:40px;height:40px;border-radius:var(--wx-radius);display:flex;align-items:center;justify-content:center;
  font-size:16px;font-weight:600;color:#fff;flex-shrink:0;
}
.request-info{flex:1;min-width:0}
.request-name{font-size:15px;font-weight:500}
.request-id{font-size:12px;color:var(--wx-text-secondary);margin-top:2px}
.request-msg{font-size:13px;color:var(--wx-text-secondary);margin-top:4px}
.request-actions{display:flex;gap:8px;flex-shrink:0}
.request-btn{
  padding:6px 16px;border:none;border-radius:6px;font-size:13px;font-weight:500;
  cursor:pointer;transition:all .15s;
}
.request-btn-agree{background:var(--wx-green);color:#fff}
.request-btn-agree:hover{background:var(--wx-green-dark)}
.request-btn-reject{background:var(--wx-bg);color:var(--wx-text-secondary)}
.request-btn-reject:hover{background:var(--wx-border)}
</style>
