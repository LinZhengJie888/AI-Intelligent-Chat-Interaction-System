<script setup>
import store from '../store'

const navItems = [
  { id: 'chatlist', label: '聊天', icon: 'M20 2H4c-1.1 0-2 .9-2 2v18l4-4h14c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zm0 14H5.17L4 17.17V4h16v12z' },
  { id: 'contacts', label: '通讯录', icon: 'M16 11c1.66 0 2.99-1.34 2.99-3S17.66 5 16 5c-1.66 0-3 1.34-3 3s1.34 3 3 3zm-8 0c1.66 0 2.99-1.34 2.99-3S9.66 5 8 5C6.34 5 5 6.34 5 8s1.34 3 3 3zm0 2c-2.33 0-7 1.17-7 3.5V19h14v-2.5c0-2.33-4.67-3.5-7-3.5z' },
  { id: 'groups', label: '群聊', icon: 'M12 12.75c1.63 0 3.07.39 4.24.9 1.08.48 1.76 1.56 1.76 2.73V18H6v-1.61c0-1.18.68-2.26 1.76-2.73 1.17-.52 2.61-.91 4.24-.91zM4 13c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zm1.13 1.1c-.37-.06-.74-.1-1.13-.1-.99 0-1.93.21-2.78.58A2.01 2.01 0 000 16.43V18h4.5v-1.61c0-.83.23-1.61.63-2.29zM20 13c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zm4 3.43c0-.81-.48-1.53-1.22-1.85A6.95 6.95 0 0020 14c-.39 0-.76.04-1.13.1.4.68.63 1.46.63 2.29V18H24v-1.57zM12 6c1.66 0 3 1.34 3 3s-1.34 3-3 3-3-1.34-3-3 1.34-3 3-3z' },
  { id: 'profile', label: '我', icon: 'M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z' }
]
</script>

<template>
  <nav class="bottom-nav">
    <button
      v-for="item in navItems"
      :key="item.id"
      class="bottom-nav-btn"
      :class="{ active: store.currentScreen === item.id }"
      @click="store.switchScreen(item.id)"
    >
      <svg viewBox="0 0 24 24"><path :d="item.icon" /></svg>
      <span>{{ item.label }}</span>
    </button>
  </nav>
</template>

<style scoped>
.bottom-nav{
  display:none;height:56px;background:#FAFAFA;border-top:1px solid var(--wx-border);
  position:fixed;bottom:0;left:0;right:0;z-index:100;
}
.bottom-nav-btn{
  flex:1;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:2px;
  border:none;background:transparent;cursor:pointer;color:var(--wx-text-secondary);font-size:10px;
}
.bottom-nav-btn svg{width:22px;height:22px;fill:currentColor}
.bottom-nav-btn.active{color:var(--wx-green)}

@media(max-width:768px){
  .bottom-nav{display:flex}
}
@media(min-width:769px){
  .bottom-nav{display:none !important}
}
</style>
