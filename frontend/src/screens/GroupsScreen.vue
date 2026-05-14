<script setup>
import { ref, onMounted } from 'vue'
import store from '../store'

const showCreateGroup = ref(false)
const newGroupName = ref('')
const showJoinGroup = ref(false)
const joinGroupId = ref('')
const joinGroupMsg = ref('')
const showGroupRequests = ref(false)
const selectedGroupId = ref('')

function submitCreateGroup() {
  if (!newGroupName.value.trim()) return
  store.createGroup(newGroupName.value.trim())
  showCreateGroup.value = false
  newGroupName.value = ''
}

function submitJoinGroup() {
  if (!joinGroupId.value.trim()) return
  store.joinGroup(joinGroupId.value.trim(), joinGroupMsg.value.trim())
  showJoinGroup.value = false
  joinGroupId.value = ''
  joinGroupMsg.value = ''
}

function openGroupRequests(groupId) {
  selectedGroupId.value = groupId
  store.loadGroupRequests(groupId)
  showGroupRequests.value = true
}

function agreeRequest(fromUserId) {
  store.agreeGroupJoin(selectedGroupId.value, fromUserId)
}

function rejectRequest(fromUserId) {
  store.rejectGroupJoin(selectedGroupId.value, fromUserId)
}

onMounted(() => {
  store.loadGroups()
})
</script>

<template>
  <div class="screen active">
    <div class="top-bar">
      <div class="top-bar-title">群聊</div>
      <button class="top-bar-action" @click="showCreateGroup=true">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><path d="M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z" /></svg>
      </button>
    </div>

    <!-- 提示信息 -->
    <div v-if="store.successMessage" class="toast-success">{{ store.successMessage }}</div>
    <div v-if="store.errorMessage" class="toast-error">{{ store.errorMessage }}</div>

    <div style="flex:1;overflow-y:auto">
      <div class="search-box" style="margin:12px 16px">
        <svg viewBox="0 0 24 24"><path d="M15.5 14h-.79l-.28-.27A6.471 6.471 0 0016 9.5 6.5 6.5 0 109.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z" /></svg>
        <input type="text" placeholder="搜索群聊">
      </div>

      <!-- 加入群聊入口 -->
      <div class="group-card" @click="showJoinGroup=true" style="background:rgba(7,193,96,.05);border-style:dashed">
        <div class="group-card-header">
          <div class="group-card-avatar" style="background:var(--wx-green)">
            <svg width="24" height="24" viewBox="0 0 24 24" fill="#fff"><path d="M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z" /></svg>
          </div>
          <div>
            <div class="group-card-name" style="color:var(--wx-green)">通过群ID加入群聊</div>
            <div class="group-card-count">输入群聊ID发起加群申请</div>
          </div>
        </div>
      </div>

      <!-- 群聊列表 -->
      <div v-for="group in store.groups" :key="group.groupId" class="group-card">
        <div class="group-card-header" @click="store.openChat('group', group.groupId, group.groupName)">
          <div class="group-card-avatar">
            <div v-for="(color, j) in group.colors" :key="j" :style="{ background: color }"></div>
          </div>
          <div style="flex:1">
            <div class="group-card-name">{{ group.groupName }}</div>
            <div class="group-card-count">{{ group.memberCount }} 人</div>
          </div>
          <!-- 群主可管理请求 -->
          <button class="group-manage-btn" @click.stop="openGroupRequests(group.groupId)" title="管理加群请求">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><path d="M16 11c1.66 0 2.99-1.34 2.99-3S17.66 5 16 5c-1.66 0-3 1.34-3 3s1.34 3 3 3zm-8 0c1.66 0 2.99-1.34 2.99-3S9.66 5 8 5C6.34 5 5 6.34 5 8s1.34 3 3 3zm0 2c-2.33 0-7 1.17-7 3.5V19h14v-2.5c0-2.33-4.67-3.5-7-3.5zm8 0c-.29 0-.62.02-.97.05 1.16.84 1.97 1.97 1.97 3.45V19h6v-2.5c0-2.33-4.67-3.5-7-3.5z" /></svg>
          </button>
        </div>
        <div class="group-card-id">群ID: {{ group.groupId }}</div>
        <div class="group-card-preview">{{ group.preview || '暂无消息' }}</div>
      </div>

      <div v-if="store.groups.length === 0" class="empty-hint">暂无群聊，点击右上角创建</div>
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
          <button class="modal-btn modal-btn-confirm" @click="submitJoinGroup">发送申请</button>
        </div>
      </div>
    </div>

    <!-- 群聊请求管理弹窗 -->
    <div v-if="showGroupRequests" class="modal-overlay" @click.self="showGroupRequests=false">
      <div class="modal-card fade-in" style="max-width:420px">
        <div class="modal-title">加群请求管理</div>
        <div class="requests-list">
          <div v-if="store.groupRequests.length === 0" class="empty-requests">暂无待处理的加群请求</div>
          <div v-for="req in store.groupRequests" :key="req.requestId" class="request-item">
            <div class="request-avatar" :style="{ background: req.color }">
              {{ (req.fromUsername || '?')[0] }}
            </div>
            <div class="request-info">
              <div class="request-name">{{ req.fromUsername || req.fromUserId }}</div>
              <div class="request-msg">{{ req.requestMsg || '请求加入群聊' }}</div>
            </div>
            <div class="request-actions">
              <button class="request-btn request-btn-agree" @click="agreeRequest(req.fromUserId)">同意</button>
              <button class="request-btn request-btn-reject" @click="rejectRequest(req.fromUserId)">拒绝</button>
            </div>
          </div>
        </div>
        <div class="modal-actions" style="margin-top:16px">
          <button class="modal-btn modal-btn-cancel" style="flex:1" @click="showGroupRequests=false">关闭</button>
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
.group-card-header{display:flex;align-items:center;gap:12px;margin-bottom:6px}
.group-card-avatar{
  width:44px;height:44px;border-radius:var(--wx-radius);display:grid;grid-template-columns:1fr 1fr;gap:2px;
  overflow:hidden;flex-shrink:0;background:var(--wx-bg);padding:2px;
}
.group-card-avatar div{border-radius:2px}
.group-card-name{font-size:15px;font-weight:500;flex:1}
.group-card-count{font-size:12px;color:var(--wx-text-secondary)}
.group-card-id{font-size:12px;color:var(--wx-green);margin-bottom:4px;font-family:var(--font-mono)}
.group-card-preview{font-size:13px;color:var(--wx-text-secondary);white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.empty-hint{padding:40px 16px;text-align:center;font-size:14px;color:var(--wx-text-tertiary)}

.group-manage-btn{
  background:none;border:none;cursor:pointer;padding:6px;border-radius:6px;transition:background .15s;
}
.group-manage-btn:hover{background:var(--wx-bg)}

.toast-success{margin:8px 16px;padding:8px 12px;background:rgba(7,193,96,.1);color:var(--wx-green);border-radius:8px;font-size:13px;text-align:center}
.toast-error{margin:8px 16px;padding:8px 12px;background:rgba(250,81,81,.1);color:#FA5151;border-radius:8px;font-size:13px;text-align:center}

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

.requests-list{max-height:300px;overflow-y:auto}
.empty-requests{padding:20px;text-align:center;font-size:14px;color:var(--wx-text-tertiary)}
.request-item{display:flex;align-items:center;gap:12px;padding:12px 0;border-bottom:1px solid var(--wx-border)}
.request-item:last-child{border-bottom:none}
.request-avatar{width:40px;height:40px;border-radius:50%;display:flex;align-items:center;justify-content:center;color:#fff;font-size:16px;font-weight:500;flex-shrink:0}
.request-info{flex:1;min-width:0}
.request-name{font-size:14px;font-weight:500;margin-bottom:2px}
.request-msg{font-size:12px;color:var(--wx-text-secondary);white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.request-actions{display:flex;gap:8px;flex-shrink:0}
.request-btn{padding:6px 14px;border:none;border-radius:6px;font-size:13px;cursor:pointer;transition:all .15s}
.request-btn-agree{background:var(--wx-green);color:#fff}
.request-btn-agree:hover{background:var(--wx-green-dark)}
.request-btn-reject{background:var(--wx-bg);color:var(--wx-text-secondary)}
.request-btn-reject:hover{background:var(--wx-border)}
</style>
