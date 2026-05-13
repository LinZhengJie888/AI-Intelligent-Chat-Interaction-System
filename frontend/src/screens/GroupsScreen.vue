<script setup>
import { ref, computed } from 'vue'
import store from '../store'

const showCreateGroup = ref(false)
const showJoinGroup = ref(false)
const newGroupName = ref('')
const joinGroupId = ref('')
const joinGroupMsg = ref('')
const searchText = ref('')

const filteredGroups = computed(() => {
  const keyword = searchText.value.trim().toLowerCase()
  if (!keyword) return store.groups
  return store.groups.filter(g =>
    g.groupName.toLowerCase().includes(keyword) ||
    g.groupId.toLowerCase().includes(keyword)
  )
})

function submitCreateGroup() {
  if (!newGroupName.value.trim()) return
  // 通过WebSocket创建群聊
  store.createGroup(newGroupName.value.trim())
  showCreateGroup.value = false
  newGroupName.value = ''
}

function submitJoinGroup() {
  if (!joinGroupId.value.trim()) return
  // 通过WebSocket加入群聊
  store.joinGroup(joinGroupId.value.trim(), joinGroupMsg.value || '申请加入群聊')
  showJoinGroup.value = false
  joinGroupId.value = ''
  joinGroupMsg.value = ''
}
</script>

<template>
  <div class="screen active">
    <div class="top-bar">
      <div class="top-bar-title">群聊</div>
      <div style="display:flex;gap:8px">
        <button class="top-bar-action" @click="showJoinGroup=true" title="加入群聊">
          <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><path d="M15 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm-9-2V7H4v3H1v2h3v3h2v-3h3v-2H6zm9 4c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z" /></svg>
        </button>
        <button class="top-bar-action" @click="showCreateGroup=true" title="创建群聊">
          <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><path d="M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z" /></svg>
        </button>
      </div>
    </div>
    <div style="flex:1;overflow-y:auto">
      <div class="search-box" style="margin:12px 16px">
        <svg viewBox="0 0 24 24"><path d="M15.5 14h-.79l-.28-.27A6.471 6.471 0 0016 9.5 6.5 6.5 0 109.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z" /></svg>
        <input v-model="searchText" type="text" placeholder="搜索群聊">
      </div>

      <div v-for="group in filteredGroups" :key="group.groupId" class="group-card" @click="store.openChat('group', group.groupId, group.groupName)">
        <div class="group-card-header">
          <div class="group-card-avatar">
            <div v-for="(color, j) in group.colors" :key="j" :style="{ background: color }"></div>
          </div>
          <div>
            <div class="group-card-name">{{ group.groupName }}</div>
            <div class="group-card-count">{{ group.memberCount }} 人</div>
          </div>
        </div>
        <div class="group-card-preview">{{ group.preview }}</div>
      </div>

      <div v-if="filteredGroups.length === 0" style="padding:40px 16px;text-align:center;color:var(--wx-text-secondary)">
        {{ searchText ? '未找到匹配的群聊' : '暂无群聊' }}
      </div>

      <div style="height:60px"></div>
    </div>

    <!-- 创建群聊弹窗 -->
    <div v-if="showCreateGroup" class="modal-overlay" @click.self="showCreateGroup=false">
      <div class="modal-card fade-in">
        <div class="modal-title">创建群聊</div>
        <div class="login-field">
          <label>群聊名称</label>
          <input v-model="newGroupName" type="text" placeholder="请输入群聊名称">
        </div>
        <div class="modal-actions">
          <button class="modal-btn modal-btn-cancel" @click="showCreateGroup=false">取消</button>
          <button class="modal-btn modal-btn-confirm" @click="submitCreateGroup">创建</button>
        </div>
      </div>
    </div>

    <!-- 加入群聊弹窗 -->
    <div v-if="showJoinGroup" class="modal-overlay" @click.self="showJoinGroup=false">
      <div class="modal-card fade-in">
        <div class="modal-title">加入群聊</div>
        <div class="login-field">
          <label>群聊ID</label>
          <input v-model="joinGroupId" type="text" placeholder="请输入群聊ID">
        </div>
        <div class="login-field">
          <label>验证消息（选填）</label>
          <input v-model="joinGroupMsg" type="text" placeholder="请输入验证消息">
        </div>
        <div class="modal-actions">
          <button class="modal-btn modal-btn-cancel" @click="showJoinGroup=false">取消</button>
          <button class="modal-btn modal-btn-confirm" @click="submitJoinGroup">申请加入</button>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.group-card{
  background:var(--wx-white);border:1px solid var(--wx-border);border-radius:var(--wx-radius-lg);
  padding:14px;margin:0 16px 8px;cursor:pointer;transition:all .15s;
}
.group-card:hover{border-color:var(--wx-green)}
.group-card-header{display:flex;align-items:center;gap:12px;margin-bottom:8px}
.group-card-avatar{
  width:44px;height:44px;border-radius:var(--wx-radius);display:grid;grid-template-columns:1fr 1fr;gap:2px;
  overflow:hidden;flex-shrink:0;background:var(--wx-bg);padding:2px;
}
.group-card-avatar div{border-radius:2px}
.group-card-name{font-size:15px;font-weight:500;flex:1}
.group-card-count{font-size:12px;color:var(--wx-text-secondary)}
.group-card-preview{font-size:13px;color:var(--wx-text-secondary);white-space:nowrap;overflow:hidden;text-overflow:ellipsis}

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
</style>
