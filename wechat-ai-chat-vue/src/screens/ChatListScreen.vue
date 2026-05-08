<script setup>
import store from '../store'

const recentChats = [
  { type: 'single', name: '张三', preview: '明天下午三点开会，记得带上方案', time: '14:32', badge: 3, color: '#576B95' },
  { type: 'group', name: '产品讨论组', preview: '李四: 新版本的设计稿已经更新了', time: '13:15', color: '#576B95' },
  { type: 'single', name: '李四', preview: '好的，收到了', time: '12:08', color: '#07C160' },
  { type: 'single', name: '王五', preview: '周末有空吗？一起去爬山', time: '昨天', color: '#FA5151' },
  { type: 'group', name: '前端技术交流', preview: '赵六: React 19 的新特性太棒了', time: '昨天', color: '#FF8800' }
]
</script>

<template>
  <div class="screen active">
    <div class="top-bar">
      <div class="top-bar-title">WeChat AI</div>
      <button class="top-bar-action" @click="store.switchScreen('contacts')">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><path d="M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z" /></svg>
      </button>
    </div>
    <div class="bento-grid">
      <div class="bento-top-row">
        <div class="bento-card" style="padding:8px 12px">
          <div class="search-box">
            <svg viewBox="0 0 24 24"><path d="M15.5 14h-.79l-.28-.27A6.471 6.471 0 0016 9.5 6.5 6.5 0 109.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z" /></svg>
            <input type="text" placeholder="搜索">
          </div>
        </div>
        <div class="bento-card bento-ai" @click="store.switchScreen('ai-chat')">
          <div class="ai-card-avatar">AI</div>
          <div class="ai-card-info">
            <div class="ai-card-name">AI 助手 <span class="ai-card-badge">智能</span></div>
            <div class="ai-card-desc">问答 · 写作 · 翻译 · 编程</div>
          </div>
        </div>
      </div>

      <div class="bento-card bento-recent" style="overflow:hidden;padding:0">
        <div style="padding:12px 16px;font-size:15px;font-weight:600;border-bottom:1px solid var(--wx-border)">最近聊天</div>
        <div class="chat-list" style="flex:1;overflow-y:auto">
          <div
            v-for="chat in recentChats"
            :key="chat.name"
            class="chat-item"
            @click="store.openChat(chat.type, chat.name)"
          >
            <div class="chat-avatar" :class="chat.type === 'group' ? 'chat-avatar-group' : 'chat-avatar-user'" :style="{ background: chat.color }">
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
  .bento-grid{
    grid-template-columns:repeat(4,1fr);gap:10px;padding:12px;
  }
  .bento-top-row{grid-column:span 4}
  .bento-recent{grid-column:span 4;grid-row:span 2}
}
@media(max-width:480px){
  .bento-top-row{flex-direction:column}
}
</style>
