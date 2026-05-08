<script setup>
import store from '../store'

const groups = [
  { name: '产品讨论组', count: 12, preview: '李四: 新版本的设计稿已经更新了', colors: ['#576B95', '#07C160', '#FA5151', '#FF8800'] },
  { name: '前端技术交流', count: 86, preview: '赵六: React 19 的新特性太棒了', colors: ['#FF8800', '#2196F3', '#E91E63', '#9C27B0'] },
  { name: '家人群', count: 6, preview: '周末聚餐定在哪里？', colors: ['#FA5151', '#07C160', '#576B95', '#FF9800'] },
  { name: '项目 Alpha', count: 24, preview: '下周一之前提交代码审查', colors: ['#9C27B0', '#2196F3', '#FF8800', '#E91E63'] }
]
</script>

<template>
  <div class="screen active">
    <div class="top-bar">
      <div class="top-bar-title">群聊</div>
      <button class="top-bar-action">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><path d="M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z" /></svg>
      </button>
    </div>
    <div style="flex:1;overflow-y:auto">
      <div class="search-box" style="margin:12px 16px">
        <svg viewBox="0 0 24 24"><path d="M15.5 14h-.79l-.28-.27A6.471 6.471 0 0016 9.5 6.5 6.5 0 109.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z" /></svg>
        <input type="text" placeholder="搜索群聊">
      </div>

      <div v-for="group in groups" :key="group.name" class="group-card" @click="store.openChat('group', group.name)">
        <div class="group-card-header">
          <div class="group-card-avatar">
            <div v-for="(color, j) in group.colors" :key="j" :style="{ background: color }"></div>
          </div>
          <div>
            <div class="group-card-name">{{ group.name }}</div>
            <div class="group-card-count">{{ group.count }} 人</div>
          </div>
        </div>
        <div class="group-card-preview">{{ group.preview }}</div>
      </div>

      <div style="height:60px"></div>
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
</style>
