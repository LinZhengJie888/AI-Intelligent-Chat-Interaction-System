<script setup>
import { computed } from 'vue'
import store from './store'
import Sidebar from './components/Sidebar.vue'
import BottomNav from './components/BottomNav.vue'
import AIPet from './components/AIPet.vue'
import LoginScreen from './screens/LoginScreen.vue'
import ChatListScreen from './screens/ChatListScreen.vue'
import SingleChatScreen from './screens/SingleChatScreen.vue'
import GroupChatScreen from './screens/GroupChatScreen.vue'
import AIChatScreen from './screens/AIChatScreen.vue'
import ContactsScreen from './screens/ContactsScreen.vue'
import GroupsScreen from './screens/GroupsScreen.vue'
import ProfileScreen from './screens/ProfileScreen.vue'

const showAIPet = computed(() => store.currentScreen !== 'login')
</script>

<template>
  <div class="app">
    <Sidebar v-if="store.isLoggedIn" />
    <div class="main-panel">
      <LoginScreen v-if="store.currentScreen === 'login'" />
      <ChatListScreen v-if="store.currentScreen === 'chatlist'" />
      <SingleChatScreen v-if="store.currentScreen === 'single-chat'" />
      <GroupChatScreen v-if="store.currentScreen === 'group-chat'" />
      <AIChatScreen v-if="store.currentScreen === 'ai-chat'" />
      <ContactsScreen v-if="store.currentScreen === 'contacts'" />
      <GroupsScreen v-if="store.currentScreen === 'groups'" />
      <ProfileScreen v-if="store.currentScreen === 'profile'" />
    </div>
    <BottomNav v-if="store.isLoggedIn" />
    <AIPet v-if="showAIPet" />
  </div>
</template>

<style>
*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}

:root{
  --wx-green:#07C160;
  --wx-green-light:#95EC69;
  --wx-green-dark:#06AD56;
  --wx-bg:#EDEDED;
  --wx-header-bg:#EDEDED;
  --wx-bubble-other:#FFFFFF;
  --wx-bubble-me:#95EC69;
  --wx-bubble-ai:#E8F5E9;
  --wx-text:#111111;
  --wx-text-secondary:#999999;
  --wx-text-tertiary:#B2B2B2;
  --wx-border:#E6E6E6;
  --wx-white:#FFFFFF;
  --wx-danger:#FA5151;
  --wx-orange:#FF8800;
  --wx-blue:#576B95;
  --wx-radius:6px;
  --wx-radius-lg:12px;
  --font-body:-apple-system,BlinkMacSystemFont,'Segoe UI','PingFang SC','Hiragino Sans GB','Microsoft YaHei',sans-serif;
  --font-display:-apple-system,BlinkMacSystemFont,'SF Pro Display','PingFang SC',sans-serif;
  --font-mono:'SF Mono','Menlo',monospace;
}

html{font-family:var(--font-body);color:var(--wx-text);background:var(--wx-bg);font-size:16px;line-height:1.5;-webkit-font-smoothing:antialiased}

.app{display:flex;height:100vh;overflow:hidden;background:var(--wx-bg)}

.main-panel{flex:1;display:flex;flex-direction:column;min-width:0;position:relative}

.screen{display:none;flex:1;flex-direction:column;overflow:hidden}
.screen.active{display:flex}

.top-bar{
  height:56px;background:var(--wx-header-bg);border-bottom:1px solid var(--wx-border);
  display:flex;align-items:center;padding:0 16px;gap:12px;flex-shrink:0;
}
.top-bar-title{font-size:17px;font-weight:600;flex:1}
.top-bar-back{width:32px;height:32px;border:none;background:transparent;cursor:pointer;display:flex;align-items:center;justify-content:center;border-radius:50%}
.top-bar-back:hover{background:rgba(0,0,0,.05)}
.top-bar-action{width:32px;height:32px;border:none;background:transparent;cursor:pointer;display:flex;align-items:center;justify-content:center;border-radius:50%}
.top-bar-action:hover{background:rgba(0,0,0,.05)}

.search-box{
  display:flex;align-items:center;gap:8px;padding:10px 14px;
  background:var(--wx-white);border-radius:var(--wx-radius);border:1px solid var(--wx-border);
}
.search-box svg{width:16px;height:16px;fill:var(--wx-text-tertiary);flex-shrink:0}
.search-box input{flex:1;border:none;outline:none;font-size:14px;background:transparent;font-family:var(--font-body)}

.chat-list{flex:1;overflow-y:auto}
.chat-item{
  display:flex;align-items:center;gap:12px;padding:12px 16px;cursor:pointer;
  transition:background .1s;border-bottom:1px solid var(--wx-border);
}
.chat-item:hover{background:rgba(0,0,0,.02)}
.chat-item:active{background:rgba(0,0,0,.05)}
.chat-avatar{
  width:44px;height:44px;border-radius:var(--wx-radius);flex-shrink:0;
  display:flex;align-items:center;justify-content:center;font-size:18px;font-weight:600;color:#fff;
}
.chat-avatar-ai{background:var(--wx-green);border-radius:12px}
.chat-avatar-group{background:var(--wx-blue)}
.chat-avatar-user{border-radius:var(--wx-radius)}
.chat-info{flex:1;min-width:0}
.chat-name{font-size:16px;font-weight:400;margin-bottom:2px;display:flex;align-items:center;gap:6px}
.chat-name-tag{
  font-size:10px;padding:1px 5px;border-radius:3px;font-weight:500;
}
.tag-ai{background:rgba(7,193,96,.12);color:var(--wx-green)}
.tag-group{background:rgba(87,107,149,.12);color:var(--wx-blue)}
.chat-preview{font-size:13px;color:var(--wx-text-secondary);white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.chat-meta{display:flex;flex-direction:column;align-items:flex-end;gap:4px;flex-shrink:0}
.chat-time{font-size:12px;color:var(--wx-text-tertiary)}
.chat-badge{
  width:18px;height:18px;background:var(--wx-danger);color:#fff;border-radius:50%;
  font-size:11px;display:flex;align-items:center;justify-content:center;font-weight:500;
}

.msg-system{
  text-align:center;font-size:12px;color:var(--wx-text-tertiary);
  padding:4px 0;
}
.msg-row{display:flex;gap:10px;max-width:80%}
.msg-row-mine{align-self:flex-end;flex-direction:row-reverse}
.msg-avatar-sm{width:36px;height:36px;border-radius:var(--wx-radius);flex-shrink:0;display:flex;align-items:center;justify-content:center;font-size:14px;font-weight:600;color:#fff}
.msg-bubble{
  padding:10px 14px;border-radius:var(--wx-radius-lg);font-size:15px;line-height:1.5;
  position:relative;word-break:break-word;max-width:100%;
}
.msg-bubble::after{
  content:'';position:absolute;top:14px;width:8px;height:8px;
  background:inherit;transform:rotate(45deg);
}
.msg-bubble-other{background:var(--wx-bubble-other);border:1px solid var(--wx-border)}
.msg-bubble-other::after{left:-4px;border-left:1px solid var(--wx-border);border-bottom:1px solid var(--wx-border)}
.msg-bubble-mine{background:var(--wx-bubble-me)}
.msg-bubble-mine::after{right:-4px}
.msg-bubble-ai{
  background:var(--wx-bubble-ai);border:1px solid rgba(7,193,96,.15);
  border-radius:var(--wx-radius-lg);padding:12px 16px;
}
.msg-bubble-ai .ai-bubble-header{
  display:flex;align-items:center;gap:6px;margin-bottom:6px;
  font-size:12px;color:var(--wx-green);font-weight:500;
}
.ai-badge{display:inline-flex;align-items:center;justify-content:center;width:18px;height:18px;border-radius:4px;background:var(--wx-green);color:#fff;font-size:9px;font-weight:700;letter-spacing:-.5px;flex-shrink:0}
.msg-sender{font-size:12px;color:var(--wx-text-secondary);margin-bottom:4px}
.msg-time{font-size:11px;color:var(--wx-text-tertiary);margin-top:4px}

.typing-indicator{display:flex;gap:4px;padding:4px 0}
.typing-dot{
  width:6px;height:6px;background:var(--wx-text-secondary);border-radius:50%;
  animation:typingBounce .6s ease-in-out infinite;
}
.typing-dot:nth-child(2){animation-delay:.15s}
.typing-dot:nth-child(3){animation-delay:.3s}
@keyframes typingBounce{0%,100%{transform:translateY(0)}50%{transform:translateY(-4px)}}

.input-bar{
  padding:8px 12px;background:var(--wx-header-bg);border-top:1px solid var(--wx-border);
  display:flex;align-items:flex-end;gap:8px;
}
.input-bar-btn{
  width:36px;height:36px;border:none;background:transparent;cursor:pointer;
  display:flex;align-items:center;justify-content:center;border-radius:50%;
  flex-shrink:0;transition:background .1s;
}
.input-bar-btn:hover{background:rgba(0,0,0,.05)}
.input-bar-btn svg{width:22px;height:22px;fill:var(--wx-text-secondary)}
.input-field{
  flex:1;min-height:36px;max-height:120px;padding:8px 12px;border:1px solid var(--wx-border);
  border-radius:var(--wx-radius);background:var(--wx-white);font-size:15px;
  outline:none;resize:none;font-family:var(--font-body);line-height:1.4;
}
.input-field:focus{border-color:var(--wx-green)}
.input-send-btn{
  width:36px;height:36px;border:none;border-radius:50%;background:var(--wx-green);
  cursor:pointer;display:flex;align-items:center;justify-content:center;flex-shrink:0;
  transition:background .15s;
}
.input-send-btn:hover{background:var(--wx-green-dark)}
.input-send-btn svg{width:18px;height:18px;fill:#fff}

@keyframes fadeIn{from{opacity:0;transform:translateY(8px)}to{opacity:1;transform:translateY(0)}}
.fade-in{animation:fadeIn .3s ease-out}

::-webkit-scrollbar{width:6px}
::-webkit-scrollbar-thumb{background:rgba(0,0,0,.15);border-radius:3px}
::-webkit-scrollbar-thumb:hover{background:rgba(0,0,0,.25)}

@media(max-width:768px){
  .msg-row{max-width:90%}
}
</style>
