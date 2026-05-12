<script setup>
import { ref, computed } from 'vue'
import store from '../store'

const searchText = ref('')

const filteredChats = computed(() => {
  const keyword = searchText.value.trim().toLowerCase()
  if (!keyword) return store.recentChats
  return store.recentChats.filter(chat =>
    chat.name.toLowerCase().includes(keyword) ||
    chat.preview.toLowerCase().includes(keyword)
  )
})
</script>

<template>
  <div class="screen active">
    <div class="top-bar">
      <div class="top-bar-title">AI 智能聊天</div>
      <button class="top-bar-action" @click="store.switchScreen('contacts')">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><path d="M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z" /></svg>
      </button>
    </div>
    <div class="bento-grid">
      <div class="bento-top-row">
        <div class="bento-card" style="padding:8px 12px">
          <div class="search-box">
            <svg viewBox="0 0 24 24"><path d="M15.5 14h-.79l-.28-.27A6.471 6.471 0 0016 9.5 6.5 6.5 0 109.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z" /></svg>
            <input v-model="searchText" type="text" placeholder="搜索">
          </div>
        </div>
        <div class="bento-card bento-ai" @click="store.openChat('ai', 'ai', 'AI 助手')">
          <div class="ai-card-avatar">AI</div>
          <div class="ai-card-info">
            <div class="ai-card-name">AI 助手 <span class="ai-card-badge">智能</span></div>
            <div class="ai-card-desc">问答 / 写作 / 翻译 / 编程</div>
          </div>
        </div>
      </div>

      <div class="bento-card bento-recent" style="overflow:hidden;padding:0">
        <div style="padding:12px 16px;font-size:15px;font-weight:600;border-bottom:1px solid var(--wx-border)">最近聊天</div>
        <div class="chat-list" style="flex:1;overflow-y:auto">
          <div
            v-for="chat in filteredChats"
            :key="chat.id"
            class="chat-item"
            @click="store.openChat(chat.type, chat.id, chat.name)"
          >
            <div class="chat-avatar" :class="chat.type === 'group' ? 'chat-avatar-group' : 'chat-avatar-user'" :style="{ background: chat.color || '#576B95' }">
              {{ chat.name[0] }}
            </div>
            <div class="chat-info">
              <div class="chat-name">
                {{ chat.name }}
                <span v-if="chat.type === 'group'" class="chat-name-tag tag-group">群聊</span>
              </div>
              <div class="chat-preview">{{ chat.preview }}</div>
            </div>
            <div class="chat-meta">
              <span class="chat-time">{{ chat.time }}</span>
              <span v-if="chat.badge" class="chat-badge">{{ chat.badge }}</span>
            </div>
          </div>
          <div v-if="filteredChats.length === 0" style="padding:40px 16px;text-align:center;color:var(--wx-text-secondary)">
            {{ searchText ? '未找到匹配的聊天' : '暂无聊天记录' }}
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.bento-grid{
  display:grid;gap:12px;padding:16px;flex:1;overflow-y:auto;
  grid-template-columns:repeat(12,1fr);grid-auto-rows:minmax(80px,auto);
}
.bento-card{
  background:var(--wx-white);border-radius:var(--wx-radius-lg);border:1px solid var(--wx-border);
  padding:16px;cursor:pointer;transition:all .15s;display:flex;flex-direction:column;
}
.bento-card:hover{border-color:var(--wx-green);box-shadow:0 2px 12px rgba(7,193,96,.1)}
.bento-top-row{grid-column:span 12;display:flex;gap:12px}
.bento-top-row .bento-card{flex:1;margin:0}
.bento-ai{flex-direction:row;align-items:center;gap:12px;background:linear-gradient(135deg,#E8F5E9 0%,#C8E6C9 100%);border-color:rgba(7,193,96,.2);padding:12px 16px}
.bento-recent{grid-column:span 12;grid-row:span 2}
.ai-card-avatar{
  width:40px;height:40px;border-radius:10px;background:var(--wx-green);
  display:flex;align-items:center;justify-content:center;flex-shrink:0;
  box-shadow:0 2px 8px rgba(7,193,96,.2);font-size:15px;font-weight:700;color:#fff;
}
.ai-card-info{flex:1;min-width:0}
.ai-card-name{font-size:15px;font-weight:600;margin-bottom:2px;display:flex;align-items:center;gap:6px}
.ai-card-desc{font-size:12px;color:var(--wx-text-secondary);line-height:1.4}
.ai-card-badge{
  padding:4px 10px;background:var(--wx-green);color:#fff;border-radius:12px;
  font-size:12px;font-weight:500;white-space:nowrap;
}
@media(max-width:768px){
  .bento-grid{grid-template-columns:repeat(4,1fr);gap:10px;padding:12px}
  .bento-top-row{grid-column:span 4}
  .bento-recent{grid-column:span 4;grid-row:span 2}
}
@media(max-width:480px){.bento-top-row{flex-direction:column}}
</style>
