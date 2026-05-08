<script setup>
import store from '../store'
</script>

<template>
  <div class="screen active">
    <div class="top-bar">
      <div class="top-bar-title">个人信息</div>
      <button class="top-bar-action">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="var(--wx-text-secondary)"><path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04c.39-.39.39-1.02 0-1.41l-2.34-2.34c-.39-.39-1.02-.39-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z" /></svg>
      </button>
    </div>
    <div class="profile-card">
      <div class="profile-header">
        <div class="profile-avatar">我</div>
        <div>
          <div class="profile-name">用户</div>
          <div class="profile-id">微信号: wxid_2025</div>
        </div>
      </div>
      <div class="profile-stats">
        <div class="profile-stat">
          <div class="profile-stat-num">128</div>
          <div class="profile-stat-label">好友</div>
        </div>
        <div class="profile-stat">
          <div class="profile-stat-num">12</div>
          <div class="profile-stat-label">群聊</div>
        </div>
        <div class="profile-stat">
          <div class="profile-stat-num">3</div>
          <div class="profile-stat-label">AI 对话</div>
        </div>
      </div>
      <div class="profile-actions">
        <div class="profile-action">
          <svg viewBox="0 0 24 24"><path d="M12 12.75c1.63 0 3.07.39 4.24.9 1.08.48 1.76 1.56 1.76 2.73V18H6v-1.61c0-1.18.68-2.26 1.76-2.73 1.17-.52 2.61-.91 4.24-.91zM4 13c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zm1.13 1.1c-.37-.06-.74-.1-1.13-.1-.99 0-1.93.21-2.78.58A2.01 2.01 0 000 16.43V18h4.5v-1.61c0-.83.23-1.61.63-2.29zM20 13c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zm4 3.43c0-.81-.48-1.53-1.22-1.85A6.95 6.95 0 0020 14c-.39 0-.76.04-1.13.1.4.68.63 1.46.63 2.29V18H24v-1.57zM12 6c1.66 0 3 1.34 3 3s-1.34 3-3 3-3-1.34-3-3 1.34-3 3-3z" /></svg>
          <span>个人信息</span>
          <span class="profile-action-arrow">›</span>
        </div>
        <div class="profile-action">
          <svg viewBox="0 0 24 24"><path d="M18 16.08c-.76 0-1.44.3-1.96.77L8.91 12.7c.05-.23.09-.46.09-.7s-.04-.47-.09-.7l7.05-4.11c.54.5 1.25.81 2.04.81 1.66 0 3-1.34 3-3s-1.34-3-3-3-3 1.34-3 3c0 .24.04.47.09.7L8.04 9.81C7.5 9.31 6.79 9 6 9c-1.66 0-3 1.34-3 3s1.34 3 3 3c.79 0 1.5-.31 2.04-.81l7.12 4.16c-.05.21-.08.43-.08.65 0 1.61 1.31 2.92 2.92 2.92 1.61 0 2.92-1.31 2.92-2.92s-1.31-2.92-2.92-2.92z" /></svg>
          <span>分享名片</span>
          <span class="profile-action-arrow">›</span>
        </div>
        <div class="profile-action">
          <svg viewBox="0 0 24 24"><path d="M12 22c1.1 0 2-.9 2-2h-4c0 1.1.89 2 2 2zm6-6v-5c0-3.07-1.64-5.64-4.5-6.32V4c0-.83-.67-1.5-1.5-1.5s-1.5.67-1.5 1.5v.68C7.63 5.36 6 7.92 6 11v5l-2 2v1h16v-1l-2-2z" /></svg>
          <span>消息通知</span>
          <span class="profile-action-arrow">›</span>
        </div>
        <div class="profile-action">
          <svg viewBox="0 0 24 24"><path d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.07.62-.07.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z" /></svg>
          <span>设置</span>
          <span class="profile-action-arrow">›</span>
        </div>
        <div class="profile-action" @click="store.switchScreen('ai-chat')">
          <span class="ai-badge" style="width:24px;height:24px;font-size:11px;border-radius:6px">AI</span>
          <span style="color:var(--wx-green)">AI 助手</span>
          <span class="profile-action-arrow">›</span>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.profile-card{padding:24px 16px;display:flex;flex-direction:column;gap:20px}
.profile-header{display:flex;align-items:center;gap:16px}
.profile-avatar{
  width:64px;height:64px;border-radius:var(--wx-radius-lg);background:var(--wx-green);
  display:flex;align-items:center;justify-content:center;font-size:28px;font-weight:700;color:#fff;
}
.profile-name{font-size:20px;font-weight:600}
.profile-id{font-size:13px;color:var(--wx-text-secondary)}
.profile-stats{display:flex;gap:0;border:1px solid var(--wx-border);border-radius:var(--wx-radius-lg);overflow:hidden}
.profile-stat{
  flex:1;text-align:center;padding:16px 8px;border-right:1px solid var(--wx-border);
}
.profile-stat:last-child{border-right:none}
.profile-stat-num{font-size:20px;font-weight:700;color:var(--wx-green)}
.profile-stat-label{font-size:12px;color:var(--wx-text-secondary);margin-top:2px}
.profile-actions{display:flex;flex-direction:column;gap:8px}
.profile-action{
  display:flex;align-items:center;gap:12px;padding:14px 16px;
  background:var(--wx-white);border-radius:var(--wx-radius);border:1px solid var(--wx-border);
  cursor:pointer;transition:all .15s;font-size:15px;
}
.profile-action+.profile-action{margin-top:-1px;border-top:none}
.profile-action:hover{border-color:var(--wx-green)}
.profile-action svg{width:20px;height:20px;fill:var(--wx-text-secondary)}
.profile-action-arrow{margin-left:auto;color:var(--wx-text-tertiary)}
</style>
