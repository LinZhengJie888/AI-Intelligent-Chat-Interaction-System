<script setup>
import store from '../store'

const contacts = [
  { header: '新的朋友', type: 'special' },
  { header: 'A', name: '艾米', color: '#E91E63' },
  { header: 'B', name: '白洁', color: '#9C27B0' },
  { header: 'C', name: '陈明', color: '#2196F3' },
  { header: 'L', name: '李四', color: '#07C160' },
  { header: 'L', name: '刘洋', color: '#FF9800' },
  { header: 'W', name: '王五', color: '#FA5151' },
  { header: 'Z', name: '张三', color: '#576B95' },
  { header: 'Z', name: '赵六', color: '#FF8800' }
]
</script>

<template>
  <div class="screen active">
    <div class="top-bar">
      <div class="top-bar-title">通讯录</div>
      <button class="top-bar-action">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><path d="M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z" /></svg>
      </button>
    </div>
    <div class="contacts-content" style="flex:1;overflow-y:auto">
      <div class="search-box" style="margin:12px 16px">
        <svg viewBox="0 0 24 24"><path d="M15.5 14h-.79l-.28-.27A6.471 6.471 0 0016 9.5 6.5 6.5 0 109.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z" /></svg>
        <input type="text" placeholder="搜索联系人">
      </div>

      <template v-for="(contact, i) in contacts" :key="i">
        <div class="contact-header">{{ contact.header }}</div>
        <div v-if="contact.type === 'special'" class="contact-item">
          <div class="chat-avatar" style="background:var(--wx-green);border-radius:8px">
            <svg width="22" height="22" viewBox="0 0 24 24" fill="#fff"><path d="M15 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm-9-2V7H4v3H1v2h3v3h2v-3h3v-2H6zm9 4c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z" /></svg>
          </div>
          <div class="chat-info">
            <div class="chat-name">新的朋友</div>
            <div class="chat-preview">3 条好友请求待处理</div>
          </div>
        </div>
        <div v-else class="contact-item" @click="store.openChat('single', contact.name)">
          <div class="chat-avatar chat-avatar-user" :style="{ background: contact.color }">{{ contact.name[0] }}</div>
          <div class="chat-info">
            <div class="chat-name">{{ contact.name }}</div>
          </div>
        </div>
      </template>

      <div style="height:60px"></div>
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
</style>
