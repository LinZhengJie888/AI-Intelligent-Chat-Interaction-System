<script setup>
import { ref, watch } from 'vue'
import store from '../store'

const props = defineProps({
  show: Boolean,
  chatKey: String
})

const emit = defineEmits(['close'])

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

watch(() => props.show, (val) => {
  if (val) {
    nickname.value = store.currentAISettings.nickname || 'AI助手'
    tone.value = store.currentAISettings.tone ?? 0
    priority.value = store.currentAISettings.priority ?? 1
  }
})

function save() {
  store.updateChatAISettings(props.chatKey, {
    nickname: nickname.value.trim() || 'AI助手',
    tone: tone.value,
    priority: priority.value
  })
  emit('close')
}
</script>

<template>
  <div v-if="show" class="modal-overlay" @click.self="emit('close')">
    <div class="modal-card fade-in">
      <div class="modal-header">
        <div class="modal-title">AI 助手设置</div>
        <div class="modal-subtitle">当前聊天的所有成员可见</div>
      </div>

      <div class="settings-section">
        <div class="settings-label">AI 昵称</div>
        <div class="settings-card">
          <input v-model="nickname" type="text" placeholder="请输入AI昵称" class="settings-input">
        </div>
      </div>

      <div class="settings-section">
        <div class="settings-label">回复语气</div>
        <div class="settings-card">
          <div
            v-for="option in toneOptions"
            :key="option.value"
            class="settings-option"
            :class="{ active: tone === option.value }"
            @click="tone = option.value"
          >
            <div class="radio-btn">
              <div v-if="tone === option.value" class="radio-inner"></div>
            </div>
            <div class="option-info">
              <div class="option-label">{{ option.label }}</div>
              <div class="option-desc">{{ option.desc }}</div>
            </div>
          </div>
        </div>
      </div>

      <div class="settings-section">
        <div class="settings-label">响应优先级</div>
        <div class="settings-card">
          <div
            v-for="option in priorityOptions"
            :key="option.value"
            class="settings-option"
            :class="{ active: priority === option.value }"
            @click="priority = option.value"
          >
            <div class="radio-btn">
              <div v-if="priority === option.value" class="radio-inner"></div>
            </div>
            <div class="option-info">
              <div class="option-label">{{ option.label }}</div>
              <div class="option-desc">{{ option.desc }}</div>
            </div>
          </div>
        </div>
      </div>

      <div class="modal-actions">
        <button class="modal-btn cancel" @click="emit('close')">取消</button>
        <button class="modal-btn confirm" @click="save">保存</button>
      </div>
    </div>
  </div>
</template>

<style scoped>
.modal-overlay{
  position:fixed;inset:0;background:rgba(0,0,0,.4);z-index:300;
  display:flex;align-items:center;justify-content:center;padding:24px;
}
.modal-card{
  background:var(--wx-white);border-radius:16px;padding:24px;width:100%;max-width:400px;
  box-shadow:0 12px 40px rgba(0,0,0,.15);max-height:80vh;overflow-y:auto;
}
.modal-header{margin-bottom:20px;text-align:center}
.modal-title{font-size:18px;font-weight:600}
.modal-subtitle{font-size:12px;color:var(--wx-text-secondary);margin-top:4px}

.settings-section{margin-bottom:16px}
.settings-label{font-size:13px;color:var(--wx-text-secondary);font-weight:500;margin-bottom:8px;padding:0 4px}
.settings-card{
  background:var(--wx-bg);border-radius:var(--wx-radius-lg);overflow:hidden;
}
.settings-input{
  width:100%;padding:12px 16px;border:none;outline:none;font-size:15px;
  font-family:var(--font-body);background:transparent;box-sizing:border-box;
}
.settings-option{
  display:flex;align-items:center;gap:12px;padding:12px 16px;cursor:pointer;
  border-bottom:1px solid var(--wx-border);transition:background .15s;
}
.settings-option:last-child{border-bottom:none}
.settings-option:hover{background:rgba(0,0,0,.02)}
.settings-option.active{background:rgba(7,193,96,.05)}

.radio-btn{
  width:18px;height:18px;border:2px solid var(--wx-border);border-radius:50%;
  display:flex;align-items:center;justify-content:center;flex-shrink:0;transition:border-color .15s;
}
.settings-option.active .radio-btn{border-color:var(--wx-green)}
.radio-inner{width:9px;height:9px;border-radius:50%;background:var(--wx-green)}

.option-info{flex:1}
.option-label{font-size:14px;font-weight:500}
.option-desc{font-size:12px;color:var(--wx-text-secondary);margin-top:2px}

.modal-actions{display:flex;gap:12px;margin-top:20px}
.modal-btn{
  flex:1;padding:11px;border:none;border-radius:8px;font-size:15px;font-weight:500;
  cursor:pointer;transition:all .15s;font-family:var(--font-body);
}
.modal-btn.cancel{background:var(--wx-bg);color:var(--wx-text-secondary)}
.modal-btn.cancel:hover{background:var(--wx-border)}
.modal-btn.confirm{background:var(--wx-green);color:#fff}
.modal-btn.confirm:hover{background:var(--wx-green-dark)}
</style>
