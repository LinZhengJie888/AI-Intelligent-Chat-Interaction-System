<script setup>
import { ref, reactive, computed, onMounted } from 'vue'
import store from '../store'

const activeTab = ref('login')

// 登录表单
const loginForm = reactive({
  userId: '',
  password: '',
  captchaCode: ''
})

// 注册表单
const regForm = reactive({
  username: '',
  userId: '',
  password: '',
  confirmPassword: '',
  phone: ''
})

// 密码显隐
const showLoginPwd = ref(false)
const showRegPwd = ref(false)
const showRegConfirmPwd = ref(false)

// 错误信息
const errors = reactive({
  loginUserId: '',
  loginPassword: '',
  captcha: '',
  regUsername: '',
  regUserId: '',
  regPassword: '',
  regConfirm: '',
  regPhone: '',
  general: ''
})

// 验证码冷却
const captchaCooldown = ref(0)

// 密码规则
const pwdRules = computed(() => {
  const p = regForm.password
  return {
    length: p.length >= 8,
    upper: /[A-Z]/.test(p),
    lower: /[a-z]/.test(p),
    digit: /[0-9]/.test(p)
  }
})

const pwdStrength = computed(() => {
  const n = Object.values(pwdRules.value).filter(Boolean).length
  if (n <= 1) return { text: '弱', color: '#FA5151' }
  if (n <= 2) return { text: '中', color: '#FF8800' }
  return { text: '强', color: '#07C160' }
})

// 生成本地验证码 SVG（演示用）
const captchaSvg = ref('')
const captchaAnswer = ref('')

function generateCaptcha() {
  const chars = 'ABCDEFGHJKLMNPQRSTUVWXYZabcdefghjkmnpqrstuvwxyz23456789'
  let code = ''
  for (let i = 0; i < 6; i++) code += chars[Math.floor(Math.random() * chars.length)]
  captchaAnswer.value = code

  const colors = ['#E53935','#1E88E5','#43A047','#FB8C00','#8E24AA','#00ACC1']
  let svg = `<svg xmlns="http://www.w3.org/2000/svg" width="120" height="40" viewBox="0 0 120 40">
    <rect width="120" height="40" fill="#f0f0f0"/>`

  for (let i = 0; i < code.length; i++) {
    const color = colors[Math.floor(Math.random() * colors.length)]
    const rotate = Math.floor(Math.random() * 30) - 15
    const y = 26 + Math.floor(Math.random() * 6) - 3
    svg += `<text x="${14 + i * 18}" y="${y}" fill="${color}" font-size="22" font-weight="bold" font-family="monospace" transform="rotate(${rotate} ${14 + i * 18} ${y})">${code[i]}</text>`
  }

  for (let i = 0; i < 4; i++) {
    const x1 = Math.random() * 120, y1 = Math.random() * 40
    const x2 = Math.random() * 120, y2 = Math.random() * 40
    svg += `<line x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}" stroke="rgba(0,0,0,0.15)" stroke-width="1"/>`
  }

  svg += '</svg>'
  captchaSvg.value = svg
}

function refreshCaptcha() {
  if (captchaCooldown.value > 0) return
  generateCaptcha()
  captchaCooldown.value = 60
  const timer = setInterval(() => {
    captchaCooldown.value--
    if (captchaCooldown.value <= 0) clearInterval(timer)
  }, 1000)
}

function clearErrors() {
  Object.keys(errors).forEach(k => errors[k] = '')
}

function validateLogin() {
  clearErrors()
  let ok = true
  if (!loginForm.userId.trim()) { errors.loginUserId = '请输入用户ID'; ok = false }
  if (!loginForm.password) { errors.loginPassword = '请输入密码'; ok = false }
  else if (loginForm.password.length < 8) { errors.loginPassword = '密码至少8个字符'; ok = false }
  if (!loginForm.captchaCode.trim()) { errors.captcha = '请输入验证码'; ok = false }
  else if (loginForm.captchaCode.toLowerCase() !== captchaAnswer.value.toLowerCase()) {
    errors.captcha = '验证码错误'; ok = false
  }
  return ok
}

function validateRegister() {
  clearErrors()
  let ok = true
  if (!regForm.username.trim()) { errors.regUsername = '请输入用户名'; ok = false }
  if (!regForm.userId.trim()) { errors.regUserId = '请输入用户ID'; ok = false }
  else if (regForm.userId.length < 4) { errors.regUserId = '用户ID至少4个字符'; ok = false }
  if (!regForm.password) { errors.regPassword = '请输入密码'; ok = false }
  else if (regForm.password.length < 8) { errors.regPassword = '密码至少8个字符'; ok = false }
  if (!regForm.confirmPassword) { errors.regConfirm = '请确认密码'; ok = false }
  else if (regForm.password !== regForm.confirmPassword) { errors.regConfirm = '两次密码不一致'; ok = false }
  if (regForm.phone && !/^1[3-9]\d{9}$/.test(regForm.phone)) { errors.regPhone = '手机号格式不正确'; ok = false }
  return ok
}

function handleLogin() {
  if (!validateLogin()) return
  // 通过WebSocket发送登录请求
  store.login(loginForm.userId, loginForm.password)
}

function handleRegister() {
  if (!validateRegister()) return
  // 通过WebSocket发送注册请求
  store.register(regForm.userId, regForm.username, regForm.password, regForm.phone)
  // 注册成功后切到登录
  activeTab.value = 'login'
  errors.general = '注册请求已发送，请等待响应'
  regForm.username = ''
  regForm.userId = ''
  regForm.password = ''
  regForm.confirmPassword = ''
  regForm.phone = ''
  generateCaptcha()
}

function switchTab(tab) {
  activeTab.value = tab
  clearErrors()
  errors.general = ''
}

onMounted(() => {
  generateCaptcha()
})
</script>

<template>
  <div class="screen active login-screen">
    <div class="login-card fade-in">
      <div class="login-logo">
        <svg viewBox="0 0 24 24"><path d="M20 2H4c-1.1 0-2 .9-2 2v18l4-4h14c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zm0 14H5.17L4 17.17V4h16v12z" /></svg>
      </div>
      <h1 class="login-title">AI 智能聊天</h1>
      <p class="login-subtitle">智能沟通，从这里开始</p>

      <!-- Tab -->
      <div class="login-tabs">
        <button class="login-tab" :class="{ active: activeTab === 'login' }" @click="switchTab('login')">登录</button>
        <button class="login-tab" :class="{ active: activeTab === 'register' }" @click="switchTab('register')">注册</button>
      </div>

      <!-- 通用提示 -->
      <div v-if="errors.general" class="login-hint">{{ errors.general }}</div>

      <!-- ===== 登录表单 ===== -->
      <form v-if="activeTab === 'login'" @submit.prevent="handleLogin">
        <div class="login-field">
          <label>用户ID</label>
          <div class="input-wrap">
            <svg class="input-icon" viewBox="0 0 24 24"><path d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z" /></svg>
            <input v-model="loginForm.userId" type="text" placeholder="请输入用户ID" maxlength="20" @input="errors.loginUserId=''">
          </div>
          <span v-if="errors.loginUserId" class="field-err">{{ errors.loginUserId }}</span>
        </div>

        <div class="login-field">
          <label>密码</label>
          <div class="input-wrap">
            <svg class="input-icon" viewBox="0 0 24 24"><path d="M18 8h-1V6c0-2.76-2.24-5-5-5S7 3.24 7 6v2H6c-1.1 0-2 .9-2 2v10c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V10c0-1.1-.9-2-2-2zm-6 9c-1.1 0-2-.9-2-2s.9-2 2-2 2 .9 2 2-.9 2-2 2zm3.1-9H8.9V6c0-1.71 1.39-3.1 3.1-3.1 1.71 0 3.1 1.39 3.1 3.1v2z" /></svg>
            <input v-model="loginForm.password" :type="showLoginPwd?'text':'password'" placeholder="请输入密码" @input="errors.loginPassword=''">
            <button type="button" class="pwd-toggle" @click="showLoginPwd=!showLoginPwd">
              <svg v-if="showLoginPwd" viewBox="0 0 24 24"><path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z" /></svg>
              <svg v-else viewBox="0 0 24 24"><path d="M12 7c2.76 0 5 2.24 5 5 0 .65-.13 1.26-.36 1.83l2.92 2.92c1.51-1.26 2.7-2.89 3.43-4.75-1.73-4.39-6-7.5-11-7.5-1.4 0-2.74.25-3.98.7l2.16 2.16C10.74 7.13 11.35 7 12 7zM2 4.27l2.28 2.28.46.46C3.08 8.3 1.78 10.02 1 12c1.73 4.39 6 7.5 11 7.5 1.55 0 3.03-.3 4.38-.84l.42.42L19.73 22 21 20.73 3.27 3 2 4.27zM7.53 9.8l1.55 1.55c-.05.21-.08.43-.08.65 0 1.66 1.34 3 3 3 .22 0 .44-.03.65-.08l1.55 1.55c-.67.33-1.41.53-2.2.53-2.76 0-5-2.24-5-5 0-.79.2-1.53.53-2.2zm4.31-.78l3.15 3.15.02-.16c0-1.66-1.34-3-3-3l-.17.01z" /></svg>
            </button>
          </div>
          <span v-if="errors.loginPassword" class="field-err">{{ errors.loginPassword }}</span>
        </div>

        <div class="login-field">
          <label>验证码</label>
          <div class="captcha-row">
            <div class="input-wrap captcha-input">
              <input v-model="loginForm.captchaCode" type="text" placeholder="输入验证码" maxlength="6" @input="errors.captcha=''">
            </div>
            <div class="captcha-img" v-html="captchaSvg" @click="refreshCaptcha"></div>
            <button type="button" class="captcha-refresh" :class="{ disabled: captchaCooldown>0 }" :disabled="captchaCooldown>0" @click="refreshCaptcha">
              <svg viewBox="0 0 24 24"><path d="M17.65 6.35C16.2 4.9 14.21 4 12 4c-4.42 0-7.99 3.58-7.99 8s3.57 8 7.99 8c3.73 0 6.84-2.55 7.73-6h-2.08c-.82 2.33-3.04 4-5.65 4-3.31 0-6-2.69-6-6s2.69-6 6-6c1.66 0 3.14.69 4.22 1.78L13 11h7V4l-2.35 2.35z" /></svg>
            </button>
          </div>
          <span v-if="errors.captcha" class="field-err">{{ errors.captcha }}</span>
          <span v-if="captchaCooldown>0" class="captcha-tip">{{ captchaCooldown }}秒后可刷新</span>
        </div>

        <button type="submit" class="login-btn login-btn-primary">登 录</button>
      </form>

      <!-- ===== 注册表单 ===== -->
      <form v-else @submit.prevent="handleRegister">
        <div class="login-field">
          <label>用户名</label>
          <div class="input-wrap">
            <svg class="input-icon" viewBox="0 0 24 24"><path d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z" /></svg>
            <input v-model="regForm.username" type="text" placeholder="请输入用户名" maxlength="20" @input="errors.regUsername=''">
          </div>
          <span v-if="errors.regUsername" class="field-err">{{ errors.regUsername }}</span>
        </div>

        <div class="login-field">
          <label>用户ID</label>
          <div class="input-wrap">
            <svg class="input-icon" viewBox="0 0 24 24"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm0 3c1.66 0 3 1.34 3 3s-1.34 3-3 3-3-1.34-3-3 1.34-3 3-3zm0 14.2c-2.5 0-4.71-1.28-6-3.22.03-1.99 4-3.08 6-3.08 1.99 0 5.97 1.09 6 3.08-1.29 1.94-3.5 3.22-6 3.22z" /></svg>
            <input v-model="regForm.userId" type="text" placeholder="请输入用户ID（至少4位）" maxlength="20" @input="errors.regUserId=''">
          </div>
          <span v-if="errors.regUserId" class="field-err">{{ errors.regUserId }}</span>
        </div>

        <div class="login-field">
          <label>密码</label>
          <div class="input-wrap">
            <svg class="input-icon" viewBox="0 0 24 24"><path d="M18 8h-1V6c0-2.76-2.24-5-5-5S7 3.24 7 6v2H6c-1.1 0-2 .9-2 2v10c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V10c0-1.1-.9-2-2-2zm-6 9c-1.1 0-2-.9-2-2s.9-2 2-2 2 .9 2 2-.9 2-2 2zm3.1-9H8.9V6c0-1.71 1.39-3.1 3.1-3.1 1.71 0 3.1 1.39 3.1 3.1v2z" /></svg>
            <input v-model="regForm.password" :type="showRegPwd?'text':'password'" placeholder="请输入密码（至少8位）" @input="errors.regPassword=''">
            <button type="button" class="pwd-toggle" @click="showRegPwd=!showRegPwd">
              <svg v-if="showRegPwd" viewBox="0 0 24 24"><path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z" /></svg>
              <svg v-else viewBox="0 0 24 24"><path d="M12 7c2.76 0 5 2.24 5 5 0 .65-.13 1.26-.36 1.83l2.92 2.92c1.51-1.26 2.7-2.89 3.43-4.75-1.73-4.39-6-7.5-11-7.5-1.4 0-2.74.25-3.98.7l2.16 2.16C10.74 7.13 11.35 7 12 7zM2 4.27l2.28 2.28.46.46C3.08 8.3 1.78 10.02 1 12c1.73 4.39 6 7.5 11 7.5 1.55 0 3.03-.3 4.38-.84l.42.42L19.73 22 21 20.73 3.27 3 2 4.27zM7.53 9.8l1.55 1.55c-.05.21-.08.43-.08.65 0 1.66 1.34 3 3 3 .22 0 .44-.03.65-.08l1.55 1.55c-.67.33-1.41.53-2.2.53-2.76 0-5-2.24-5-5 0-.79.2-1.53.53-2.2zm4.31-.78l3.15 3.15.02-.16c0-1.66-1.34-3-3-3l-.17.01z" /></svg>
            </button>
          </div>
          <span v-if="errors.regPassword" class="field-err">{{ errors.regPassword }}</span>
          <div v-if="regForm.password" class="pwd-strength">
            <div class="strength-bar"><div class="strength-fill" :style="{ width: (Object.values(pwdRules).filter(Boolean).length/4*100)+'%', background: pwdStrength.color }"></div></div>
            <span :style="{ color: pwdStrength.color }">密码强度：{{ pwdStrength.text }}</span>
          </div>
          <div v-if="regForm.password" class="pwd-rules">
            <div class="rule" :class="{ ok: pwdRules.length }"><span class="dot"></span>至少8个字符</div>
            <div class="rule" :class="{ ok: pwdRules.upper }"><span class="dot"></span>包含大写字母</div>
            <div class="rule" :class="{ ok: pwdRules.lower }"><span class="dot"></span>包含小写字母</div>
            <div class="rule" :class="{ ok: pwdRules.digit }"><span class="dot"></span>包含数字</div>
          </div>
        </div>

        <div class="login-field">
          <label>确认密码</label>
          <div class="input-wrap">
            <svg class="input-icon" viewBox="0 0 24 24"><path d="M18 8h-1V6c0-2.76-2.24-5-5-5S7 3.24 7 6v2H6c-1.1 0-2 .9-2 2v10c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V10c0-1.1-.9-2-2-2zm-6 9c-1.1 0-2-.9-2-2s.9-2 2-2 2 .9 2 2-.9 2-2 2zm3.1-9H8.9V6c0-1.71 1.39-3.1 3.1-3.1 1.71 0 3.1 1.39 3.1 3.1v2z" /></svg>
            <input v-model="regForm.confirmPassword" :type="showRegConfirmPwd?'text':'password'" placeholder="请再次输入密码" @input="errors.regConfirm=''">
            <button type="button" class="pwd-toggle" @click="showRegConfirmPwd=!showRegConfirmPwd">
              <svg v-if="showRegConfirmPwd" viewBox="0 0 24 24"><path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z" /></svg>
              <svg v-else viewBox="0 0 24 24"><path d="M12 7c2.76 0 5 2.24 5 5 0 .65-.13 1.26-.36 1.83l2.92 2.92c1.51-1.26 2.7-2.89 3.43-4.75-1.73-4.39-6-7.5-11-7.5-1.4 0-2.74.25-3.98.7l2.16 2.16C10.74 7.13 11.35 7 12 7zM2 4.27l2.28 2.28.46.46C3.08 8.3 1.78 10.02 1 12c1.73 4.39 6 7.5 11 7.5 1.55 0 3.03-.3 4.38-.84l.42.42L19.73 22 21 20.73 3.27 3 2 4.27zM7.53 9.8l1.55 1.55c-.05.21-.08.43-.08.65 0 1.66 1.34 3 3 3 .22 0 .44-.03.65-.08l1.55 1.55c-.67.33-1.41.53-2.2.53-2.76 0-5-2.24-5-5 0-.79.2-1.53.53-2.2zm4.31-.78l3.15 3.15.02-.16c0-1.66-1.34-3-3-3l-.17.01z" /></svg>
            </button>
          </div>
          <span v-if="errors.regConfirm" class="field-err">{{ errors.regConfirm }}</span>
        </div>

        <div class="login-field">
          <label>手机号（选填）</label>
          <div class="input-wrap">
            <svg class="input-icon" viewBox="0 0 24 24"><path d="M17 1.01L7 1c-1.1 0-2 .9-2 2v18c0 1.1.9 2 2 2h10c1.1 0 2-.9 2-2V3c0-1.1-.9-1.99-2-1.99zM17 19H7V5h10v14z" /></svg>
            <input v-model="regForm.phone" type="tel" placeholder="请输入手机号（选填）" maxlength="11" @input="errors.regPhone=''">
          </div>
          <span v-if="errors.regPhone" class="field-err">{{ errors.regPhone }}</span>
        </div>

        <button type="submit" class="login-btn login-btn-primary">注 册</button>
      </form>

      <!-- 底部装饰 -->
      <div class="login-footer">
        <span class="tag">实时对话</span>
        <span class="tag">AI 助手</span>
        <span class="tag">群聊协作</span>
        <span class="tag">安全加密</span>
      </div>
    </div>
  </div>
</template>

<style scoped>
.login-screen{
  background:linear-gradient(135deg,#E8F5E9 0%,#C8E6C9 30%,#A5D6A7 60%,#81C784 100%);
  align-items:center;justify-content:center;padding:24px;
}
.login-card{
  width:100%;max-width:420px;background:rgba(255,255,255,.85);
  backdrop-filter:blur(24px) saturate(1.8);-webkit-backdrop-filter:blur(24px) saturate(1.8);
  border-radius:20px;padding:40px 32px;border:1px solid rgba(255,255,255,.6);
  box-shadow:0 8px 40px rgba(0,0,0,.06),0 1px 3px rgba(0,0,0,.04);
}
.login-logo{
  width:64px;height:64px;background:var(--wx-green);border-radius:16px;
  display:flex;align-items:center;justify-content:center;margin:0 auto 16px;
  box-shadow:0 4px 16px rgba(7,193,96,.3);
}
.login-logo svg{width:36px;height:36px;fill:#fff}
.login-title{text-align:center;font-size:22px;font-weight:700;margin-bottom:4px}
.login-subtitle{text-align:center;font-size:14px;color:var(--wx-text-secondary);margin-bottom:24px}
.login-tabs{display:flex;margin-bottom:20px;border-bottom:1px solid var(--wx-border)}
.login-tab{
  flex:1;padding:10px;text-align:center;font-size:15px;font-weight:500;
  border:none;background:transparent;cursor:pointer;color:var(--wx-text-secondary);
  border-bottom:2px solid transparent;transition:all .2s;
}
.login-tab.active{color:var(--wx-green);border-bottom-color:var(--wx-green)}
.login-hint{
  background:rgba(7,193,96,.1);color:var(--wx-green);
  padding:10px 14px;border-radius:8px;font-size:13px;margin-bottom:16px;text-align:center;
}
.login-field{margin-bottom:14px}
.login-field label{display:block;font-size:13px;color:var(--wx-text-secondary);margin-bottom:6px;font-weight:500}
.input-wrap{
  display:flex;align-items:center;border:1px solid rgba(0,0,0,.1);border-radius:8px;
  background:rgba(255,255,255,.7);transition:all .2s;overflow:hidden;
}
.input-wrap:focus-within{border-color:var(--wx-green);box-shadow:0 0 0 3px rgba(7,193,96,.12)}
.input-icon{width:20px;height:20px;fill:var(--wx-text-tertiary);margin-left:12px;flex-shrink:0}
.input-wrap input{
  flex:1;padding:12px;border:none;background:transparent;font-size:15px;outline:none;
  font-family:var(--font-body);
}
.input-wrap input::placeholder{color:var(--wx-text-tertiary)}
.pwd-toggle{
  width:36px;height:36px;border:none;background:transparent;cursor:pointer;
  display:flex;align-items:center;justify-content:center;flex-shrink:0;
}
.pwd-toggle svg{width:20px;height:20px;fill:var(--wx-text-tertiary)}

/* 验证码 */
.captcha-row{display:flex;gap:8px;align-items:stretch}
.captcha-input{flex:1}
.captcha-input input{width:100%}
.captcha-img{
  width:120px;height:40px;border:1px solid rgba(0,0,0,.1);border-radius:8px;
  overflow:hidden;cursor:pointer;flex-shrink:0;display:flex;align-items:center;justify-content:center;background:#f5f5f5;
}
.captcha-img :deep(svg){width:100%;height:100%}
.captcha-refresh{
  width:40px;height:40px;border:1px solid rgba(0,0,0,.1);border-radius:8px;
  background:rgba(255,255,255,.7);cursor:pointer;display:flex;align-items:center;justify-content:center;flex-shrink:0;
}
.captcha-refresh:hover:not(.disabled){border-color:var(--wx-green)}
.captcha-refresh.disabled{opacity:.5;cursor:not-allowed}
.captcha-refresh svg{width:18px;height:18px;fill:var(--wx-text-secondary)}
.captcha-tip{font-size:12px;color:var(--wx-text-tertiary);margin-top:4px;display:block}
.field-err{font-size:12px;color:#FA5151;margin-top:4px;display:block}

/* 密码强度 */
.pwd-strength{margin-top:8px;display:flex;align-items:center;gap:8px}
.strength-bar{flex:1;height:4px;background:rgba(0,0,0,.06);border-radius:2px;overflow:hidden}
.strength-fill{height:100%;border-radius:2px;transition:width .3s,background .3s}
.pwd-strength span{font-size:12px;font-weight:500;white-space:nowrap}
.pwd-rules{margin-top:8px;display:grid;grid-template-columns:1fr 1fr;gap:4px}
.rule{font-size:12px;color:var(--wx-text-tertiary);display:flex;align-items:center;gap:6px}
.rule.ok{color:var(--wx-green)}
.dot{width:6px;height:6px;border-radius:50%;background:var(--wx-text-tertiary);flex-shrink:0}
.rule.ok .dot{background:var(--wx-green)}

.login-btn{
  width:100%;padding:13px;border:none;border-radius:8px;font-size:16px;font-weight:600;
  cursor:pointer;transition:all .15s;font-family:var(--font-body);margin-top:8px;
}
.login-btn-primary{background:var(--wx-green);color:#fff}
.login-btn-primary:hover{background:var(--wx-green-dark)}

.login-footer{margin-top:24px;text-align:center;display:flex;justify-content:center;gap:8px;flex-wrap:wrap}
.tag{padding:4px 10px;background:rgba(7,193,96,.08);color:var(--wx-green);border-radius:12px;font-size:12px}

@media(max-width:480px){.login-card{padding:28px 20px}}
</style>
