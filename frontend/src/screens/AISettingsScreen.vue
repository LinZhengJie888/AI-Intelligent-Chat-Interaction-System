<script setup>
import { ref, onMounted } from 'vue'
import store from '../store'

const nickname = ref('')
const tone = ref(0)
const priority = ref(1)

const toneOptions = [
  { value: 0, label: '默认', desc: '平衡、专业的回复风格' },
  { value: 1, label: '活泼', desc: '轻松、有趣的回复风格' },
  { value: 2, label: '严谨', desc: '正式、准确的回复风格' },
  { value: 3, label: '幽默', desc: '风趣、诙谐的回复风格' }
]

const priorityOptions = [
  { value: 0, label: '速度优先', desc: '快速响应，适合简单问题' },
  { value: 1, label: '均衡模式', desc: '速度与质量兼顾' },
  { value: 2, label: '质量优先', desc: '深度思考，适合复杂问题' }
]

onMounted(() => {
  nickname.value = store.aiSettings.nickname || 'AI助手'
  tone.value = store.aiSettings.tone ?? 0
  priority.value = store.aiSettings.priority ?? 1
})

function saveSettings() {
  store.aiSettings = {
    nickname: nickname.value.trim() || 'AI助手',
    tone: tone.value,
    priority: priority.value
  }
  store.switchScreen('profile')
}
</script>

<template>
  <div class="screen active">
    <div class="top-bar" style="background:var(--wx-green);color:#fff;border-bottom-color:var(--wx-green)">
      <button class="top-bar-back" @click="store.switchScreen('profile')">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="#fff"><path d="M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z" /></svg>
      </button>
      <div class="top-bar-title" style="color:#fff">AI 设置</div>
      <button class="top-bar-action" @click="saveSettings">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="rgba(255,255,255,.8)"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z" /></svg>
      </button>
    </div>
    <div class="settings-content">
      <div class="settings-section">
        <div class="settings-section-title">AI 助手昵称</div>
        <div class="settings-card">
          <div class="settings-item">
            <input v-model="nickname" type="text" placeholder="请输入AI助手昵称" class="settings-input">
          </div>
        </div>
        <div class="settings-hint">修改后在所有聊天中生效</div>
      </div>

      <div class="settings-section">
        <div class="settings-section-title">回复语气</div>
        <div class="settings-card">
          <div
            v-for="option in toneOptions"
            :key="option.value"
            class="settings-option"
            :class="{ active: tone === option.value }"
            @click="tone = option.value"
          >
            <div class="settings-option-radio">
              <div v-if="tone === option.value" class="settings-option-radio-inner"></div>
            </div>
            <div class="settings-option-info">
              <div class="settings-option-label">{{ option.label }}</div>
              <div class="settings-option-desc">{{ option.desc }}</div>
            </div>
          </div>
        </div>
      </div>

      <div class="settings-section">
        <div class="settings-section-title">响应优先级</div>
        <div class="settings-card">
          <div
            v-for="option in priorityOptions"
            :key="option.value"
            class="settings-option"
            :class="{ active: priority === option.value }"
            @click="priority = option.value"
          >
            <div class="settings-option-radio">
              <div v-if="priority === option.value" class="settings-option-radio-inner"></div>
            </div>
            <div class="settings-option-info">
              <div class="settings-option-label">{{ option.label }}</div>
              <div class="settings-option-desc">{{ option.desc }}</div>
            </div>
          </div>
        </div>
      </div>

      <div class="settings-actions">
        <button class="settings-save-btn" @click="saveSettings">保存设置</button>
      </div>
    </div>
  </div>
</template>

<style scoped>
.settings-content{flex:1;overflow-y:auto;padding:16px;display:flex;flex-direction:column;gap:20px}
.settings-section{display:flex;flex-direction:column;gap:8px}
.settings-section-title{font-size:13px;color:var(--wx-text-secondary);font-weight:500;padding:0 4px}
.settings-card{
  background:var(--wx-white);border:1px solid var(--wx-border);border-radius:var(--wx-radius-lg);
  overflow:hidden;
}
.settings-item{padding:14px 16px}
.settings-input{
  width:100%;padding:0;border:none;outline:none;font-size:15px;
  font-family:var(--font-body);background:transparent;
}
.settings-hint{font-size:12px;color:var(--wx-text-tertiary);padding:0 4px}
.settings-option{
  display:flex;align-items:center;gap:12px;padding:14px 16px;cursor:pointer;
  border-bottom:1px solid var(--wx-border);transition:all .15s;
}
.settings-option:last-child{border-bottom:none}
.settings-option:hover{background:rgba(0,0,0,.02)}
.settings-option.active{background:rgba(7,193,96,.05)}
.settings-option-radio{
  width:20px;height:20px;border:2px solid var(--wx-border);border-radius:50%;
  display:flex;align-items:center;justify-content:center;flex-shrink:0;transition:all .15s;
}
.settings-option.active .settings-option-radio{border-color:var(--wx-green)}
.settings-option-radio-inner{width:10px;height:10px;border-radius:50%;background:var(--wx-green)}
.settings-option-info{flex:1}
.settings-option-label{font-size:15px;font-weight:500}
.settings-option-desc{font-size:12px;color:var(--wx-text-secondary);margin-top:2px}
.settings-actions{padding:20px 0}
.settings-save-btn{
  width:100%;padding:14px;background:var(--wx-green);color:#fff;border:none;
  border-radius:var(--wx-radius-lg);font-size:16px;font-weight:600;cursor:pointer;
  transition:background .15s;font-family:var(--font-body);
}
.settings-save-btn:hover{background:var(--wx-green-dark)}
</style>
