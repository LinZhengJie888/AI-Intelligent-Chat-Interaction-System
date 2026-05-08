<script setup>
import { ref } from 'vue'
import store from '../store'

const activeTab = ref('login')

function handleLogin(e) {
  e.preventDefault()
  store.login()
}
</script>

<template>
  <div class="screen active login-screen">
    <div class="login-card fade-in">
      <div class="login-logo">
        <svg viewBox="0 0 24 24"><path d="M20 2H4c-1.1 0-2 .9-2 2v18l4-4h14c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zm0 14H5.17L4 17.17V4h16v12z" /></svg>
      </div>
      <h1 class="login-title">WeChat AI</h1>
      <p class="login-subtitle">智能聊天，从这里开始</p>
      <div class="login-tabs">
        <button class="login-tab" :class="{ active: activeTab === 'login' }" @click="activeTab = 'login'">登录</button>
        <button class="login-tab" :class="{ active: activeTab === 'register' }" @click="activeTab = 'register'">注册</button>
      </div>
      <form @submit="handleLogin">
        <div class="login-field">
          <label>手机号</label>
          <input type="tel" placeholder="请输入手机号" maxlength="11">
        </div>
        <div class="login-field">
          <label>密码</label>
          <input type="password" placeholder="请输入密码">
        </div>
        <button type="submit" class="login-btn login-btn-primary">登录</button>
        <button type="button" class="login-btn login-btn-ghost">忘记密码？</button>
      </form>
      <div class="login-divider">其他方式</div>
      <div class="login-social">
        <button class="login-social-btn"><svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-1 17.93c-3.95-.49-7-3.85-7-7.93 0-.62.08-1.21.21-1.79L9 15v1c0 1.1.9 2 2 2v1.93zm6.9-2.54c-.26-.81-1-1.39-1.9-1.39h-1v-3c0-.55-.45-1-1-1H8v-2h2c.55 0 1-.45 1-1V7h2c1.1 0 2-.9 2-2v-.41c2.93 1.19 5 4.06 5 7.41 0 2.08-.8 3.97-2.1 5.39z" /></svg></button>
        <button class="login-social-btn"><svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor"><path d="M12 2C6.477 2 2 6.477 2 12c0 4.991 3.657 9.128 8.438 9.879V14.89h-2.54V12h2.54V9.797c0-2.506 1.492-3.89 3.777-3.89 1.094 0 2.238.195 2.238.195v2.46h-1.26c-1.243 0-1.63.771-1.63 1.562V12h2.773l-.443 2.89h-2.33v6.989C18.343 21.129 22 16.99 22 12c0-5.523-4.477-10-10-10z" /></svg></button>
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
  width:100%;max-width:380px;background:rgba(255,255,255,.7);
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
.login-subtitle{text-align:center;font-size:14px;color:var(--wx-text-secondary);margin-bottom:32px}
.login-tabs{display:flex;gap:0;margin-bottom:24px;border-bottom:1px solid var(--wx-border)}
.login-tab{
  flex:1;padding:10px;text-align:center;font-size:15px;font-weight:500;
  border:none;background:transparent;cursor:pointer;color:var(--wx-text-secondary);
  border-bottom:2px solid transparent;transition:all .2s;
}
.login-tab.active{color:var(--wx-green);border-bottom-color:var(--wx-green)}
.login-field{margin-bottom:16px}
.login-field label{display:block;font-size:13px;color:var(--wx-text-secondary);margin-bottom:6px}
.login-field input{
  width:100%;padding:12px 14px;border:1px solid rgba(0,0,0,.1);border-radius:8px;
  background:rgba(255,255,255,.7);font-size:16px;outline:none;transition:all .2s;
  font-family:var(--font-body);
}
.login-field input:focus{border-color:var(--wx-green);box-shadow:0 0 0 3px rgba(7,193,96,.12)}
.login-field input::placeholder{color:var(--wx-text-tertiary)}
.login-btn{
  width:100%;padding:13px;border:none;border-radius:8px;font-size:16px;font-weight:600;
  cursor:pointer;transition:all .15s;font-family:var(--font-body);
}
.login-btn-primary{background:var(--wx-green);color:#fff}
.login-btn-primary:hover{background:var(--wx-green-dark)}
.login-btn-ghost{background:transparent;color:var(--wx-green);margin-top:8px}
.login-divider{display:flex;align-items:center;gap:12px;margin:20px 0;color:var(--wx-text-tertiary);font-size:13px}
.login-divider::before,.login-divider::after{content:'';flex:1;height:1px;background:rgba(0,0,0,.1)}
.login-social{display:flex;justify-content:center;gap:20px}
.login-social-btn{
  width:44px;height:44px;border-radius:50%;border:1px solid rgba(0,0,0,.1);
  background:rgba(255,255,255,.7);cursor:pointer;display:flex;align-items:center;justify-content:center;
  transition:all .15s;
}
.login-social-btn:hover{background:var(--wx-green);border-color:var(--wx-green);color:#fff}

@media(max-width:480px){
  .login-card{padding:28px 20px}
}
</style>
