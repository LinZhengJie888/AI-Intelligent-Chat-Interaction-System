<script setup>
import store from '../store'

const screens = [
  { id: 'chatlist', label: '聊天', icon: 'M20 2H4c-1.1 0-2 .9-2 2v18l4-4h14c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zm0 14H5.17L4 17.17V4h16v12z' },
  { id: 'contacts', label: '通讯录', icon: 'M16 11c1.66 0 2.99-1.34 2.99-3S17.66 5 16 5c-1.66 0-3 1.34-3 3s1.34 3 3 3zm-8 0c1.66 0 2.99-1.34 2.99-3S9.66 5 8 5C6.34 5 5 6.34 5 8s1.34 3 3 3zm0 2c-2.33 0-7 1.17-7 3.5V19h14v-2.5c0-2.33-4.67-3.5-7-3.5zm8 0c-.29 0-.62.02-.97.05 1.16.84 1.97 1.97 1.97 3.45V19h6v-2.5c0-2.33-4.67-3.5-7-3.5z' },
  { id: 'groups', label: '群聊', icon: 'M12 12.75c1.63 0 3.07.39 4.24.9 1.08.48 1.76 1.56 1.76 2.73V18H6v-1.61c0-1.18.68-2.26 1.76-2.73 1.17-.52 2.61-.91 4.24-.91zM4 13c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zm1.13 1.1c-.37-.06-.74-.1-1.13-.1-.99 0-1.93.21-2.78.58A2.01 2.01 0 000 16.43V18h4.5v-1.61c0-.83.23-1.61.63-2.29zM20 13c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zm4 3.43c0-.81-.48-1.53-1.22-1.85A6.95 6.95 0 0020 14c-.39 0-.76.04-1.13.1.4.68.63 1.46.63 2.29V18H24v-1.57zM12 6c1.66 0 3 1.34 3 3s-1.34 3-3 3-3-1.34-3-3 1.34-3 3-3z' }
]
</script>

<template>
  <nav class="sidebar">
    <div class="sidebar-avatar" @click="store.switchScreen('profile')">我</div>
    <button
      v-for="screen in screens"
      :key="screen.id"
      class="sidebar-btn"
      :class="{ active: store.currentScreen === screen.id }"
      @click="store.switchScreen(screen.id)"
    >
      <svg viewBox="0 0 24 24"><path :d="screen.icon" /></svg>
      <span>{{ screen.label }}</span>
    </button>
  </nav>
</template>

<style scoped>
.sidebar{
  width:80px;background:#2E2E2E;display:flex;flex-direction:column;align-items:center;
  padding:20px 0;gap:4px;flex-shrink:0;
}
.sidebar-avatar{
  width:44px;height:44px;border-radius:50%;background:var(--wx-green);
  display:flex;align-items:center;justify-content:center;color:#fff;font-weight:600;font-size:18px;margin-bottom:20px;cursor:pointer;
}
.sidebar-btn{
  width:56px;height:56px;border-radius:8px;border:none;background:transparent;
  display:flex;flex-direction:column;align-items:center;justify-content:center;gap:3px;cursor:pointer;transition:background .15s;color:#999;font-size:10px;
}
.sidebar-btn svg{width:22px;height:22px;fill:currentColor}
.sidebar-btn:hover{background:rgba(255,255,255,.08)}
.sidebar-btn.active{background:var(--wx-green);color:#fff}

@media(max-width:768px){
  .sidebar{display:none}
}
</style>
