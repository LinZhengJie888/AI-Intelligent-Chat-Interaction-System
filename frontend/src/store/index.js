/**
 * 全局状态管理（纯前端）
 *
 * 管理页面导航、用户信息、聊天数据等前端状态。
 * 后续对接后端时在 api/ 层统一处理通信。
 */

import { reactive } from 'vue'

// ========== 默认模拟数据 ==========
const DEFAULT_FRIENDS = [
  { userId: 'zhang3', username: '张三', avatarPath: '', color: '#576B95' },
  { userId: 'li4', username: '李四', avatarPath: '', color: '#07C160' },
  { userId: 'wang5', username: '王五', avatarPath: '', color: '#FA5151' },
  { userId: 'zhaol', username: '赵六', avatarPath: '', color: '#FF8800' },
  { userId: 'aimi', username: '艾米', avatarPath: '', color: '#E91E63' },
  { userId: 'baij', username: '白洁', avatarPath: '', color: '#9C27B0' },
  { userId: 'chenm', username: '陈明', avatarPath: '', color: '#2196F3' },
  { userId: 'liuy', username: '刘洋', avatarPath: '', color: '#FF9800' }
]

const DEFAULT_GROUPS = [
  { groupId: 'group_001', groupName: '产品讨论组', memberCount: 12, preview: '李四: 新版本的设计稿已经更新了', colors: ['#576B95', '#07C160', '#FA5151', '#FF8800'] },
  { groupId: 'group_002', groupName: '前端技术交流', memberCount: 86, preview: '赵六: React 19 的新特性太棒了', colors: ['#FF8800', '#2196F3', '#E91E63', '#9C27B0'] },
  { groupId: 'group_003', groupName: '家人群', memberCount: 6, preview: '周末聚餐定在哪里？', colors: ['#FA5151', '#07C160', '#576B95', '#FF9800'] },
  { groupId: 'group_004', groupName: '项目 Alpha', memberCount: 24, preview: '下周一之前提交代码审查', colors: ['#9C27B0', '#2196F3', '#FF8800', '#E91E63'] }
]

const DEFAULT_RECENT_CHATS = [
  { type: 'single', id: 'zhang3', name: '张三', preview: '明天下午三点开会，记得带上方案', time: '14:32', timestamp: Date.now() - 3600000, badge: 3, color: '#576B95' },
  { type: 'group', id: 'group_001', name: '产品讨论组', preview: '李四: 新版本的设计稿已经更新了', time: '13:15', timestamp: Date.now() - 7200000, color: '#576B95' },
  { type: 'single', id: 'li4', name: '李四', preview: '好的，收到了', time: '12:08', timestamp: Date.now() - 10800000, color: '#07C160' },
  { type: 'single', id: 'wang5', name: '王五', preview: '周末有空吗？一起去爬山', time: '昨天', timestamp: Date.now() - 86400000, color: '#FA5151' },
  { type: 'group', id: 'group_002', name: '前端技术交流', preview: '赵六: React 19 的新特性太棒了', time: '昨天', timestamp: Date.now() - 90000000, color: '#FF8800' }
]

const DEFAULT_FRIEND_REQUESTS = [
  { fromUserId: 'newuser1', fromUsername: '新用户', requestMsg: '你好，我是新来的', avatarPath: '', color: '#E91E63', timestamp: Date.now() - 1800000 },
  { fromUserId: 'testuser', fromUsername: '测试用户', requestMsg: '加个好友吧', avatarPath: '', color: '#9C27B0', timestamp: Date.now() - 3600000 }
]

const store = reactive({
  // ========== 用户状态 ==========
  isLoggedIn: false,
  currentUser: {
    userId: '',
    username: '',
    nickname: '',
    phone: '',
    avatarPath: ''
  },

  // ========== 页面导航 ==========
  currentScreen: 'login',

  // ========== 当前聊天上下文 ==========
  currentChat: { type: '', id: '', name: '' },

  // ========== 数据列表 ==========
  friends: [...DEFAULT_FRIENDS],
  groups: [...DEFAULT_GROUPS],
  recentChats: [...DEFAULT_RECENT_CHATS],
  friendRequests: [...DEFAULT_FRIEND_REQUESTS],
  messages: [],

  // ========== AI 设置（聊天级别，共享） ==========
  // 每个聊天独立的AI设置，key格式：single:userId / group:groupId / ai:ai
  chatAISettings: {
    'single:zhang3': { nickname: '小智', tone: 1, priority: 2 },
    'single:li4': { nickname: '助手', tone: 0, priority: 1 },
    'group:group_001': { nickname: '群助', tone: 2, priority: 0 },
    'group:group_002': { nickname: 'AI', tone: 3, priority: 1 },
    'ai:ai': { nickname: 'AI助手', tone: 0, priority: 1 }
  },
  // 当前聊天的AI设置（响应式）
  currentAISettings: { nickname: 'AI助手', tone: 0, priority: 1 },

  // ========== 错误信息 ==========
  errorMessage: '',

  // ==================== 页面切换 ====================
  switchScreen(id) {
    this.currentScreen = id
    localStorage.setItem('current-screen', id)
  },

  openChat(type, id, name) {
    this.currentChat = { type, id: id || '', name: name || '' }
    // 加载当前聊天的AI设置
    const chatKey = this.getChatKey(type, id)
    this.loadAISettings(chatKey)
    if (type === 'group') this.switchScreen('group-chat')
    else if (type === 'ai') this.switchScreen('ai-chat')
    else this.switchScreen('single-chat')
    this.messages = []
  },

  /** 获取聊天key */
  getChatKey(type, id) {
    if (type === 'ai') return 'ai:ai'
    return `${type}:${id}`
  },

  /** 加载聊天的AI设置 */
  loadAISettings(chatKey) {
    const settings = this.chatAISettings[chatKey]
    if (settings) {
      this.currentAISettings = { ...settings }
    } else {
      this.currentAISettings = { nickname: 'AI助手', tone: 0, priority: 1 }
    }
  },

  /** 更新聊天的AI设置 */
  updateChatAISettings(chatKey, settings) {
    this.chatAISettings[chatKey] = { ...settings }
    this.currentAISettings = { ...settings }
  },

  // ==================== 用户操作 ====================

  /** 登录 */
  login(userId, username) {
    this.isLoggedIn = true
    this.currentUser.userId = userId
    this.currentUser.username = username || userId
    localStorage.setItem('user-info', JSON.stringify(this.currentUser))
    this.switchScreen('chatlist')
  },

  /** 登出 */
  logout() {
    this.isLoggedIn = false
    this.currentUser = { userId: '', username: '', nickname: '', phone: '', avatarPath: '' }
    this.messages = []
    // 重置为默认模拟数据
    this.friends = [...DEFAULT_FRIENDS]
    this.groups = [...DEFAULT_GROUPS]
    this.recentChats = [...DEFAULT_RECENT_CHATS]
    this.friendRequests = [...DEFAULT_FRIEND_REQUESTS]
    localStorage.removeItem('user-info')
    this.switchScreen('login')
  },

  /** 更新用户信息 */
  updateProfile(info) {
    Object.assign(this.currentUser, info)
    localStorage.setItem('user-info', JSON.stringify(this.currentUser))
  },

  // ==================== 聊天操作 ====================

  /** 发送消息（本地模拟） */
  sendMessage(text, type) {
    const ts = Date.now()
    this.messages.push({
      type: 'mine',
      sender: this.currentUser.userId,
      text,
      time: this._fmtTime(ts),
      timestamp: ts
    })
  },

  /** 更新最近聊天 */
  upsertRecent(type, id, name, content) {
    const ts = Date.now()
    const existing = this.recentChats.find(c => c.type === type && c.id === id)
    if (existing) {
      existing.preview = content
      existing.time = this._fmtTime(ts)
      existing.timestamp = ts
      this.recentChats.sort((a, b) => b.timestamp - a.timestamp)
    } else {
      this.recentChats.unshift({
        type, id, name: name || id,
        preview: content,
        time: this._fmtTime(ts),
        timestamp: ts
      })
    }
  },

  // ==================== 好友操作 ====================

  addFriendRequest(req) {
    this.friendRequests.push(req)
  },

  agreeFriend(userId) {
    this.friendRequests = this.friendRequests.filter(r => r.fromUserId !== userId)
  },

  rejectFriend(userId) {
    this.friendRequests = this.friendRequests.filter(r => r.fromUserId !== userId)
  },

  // ==================== 工具方法 ====================

  _fmtTime(ts) {
    if (!ts) return ''
    const d = new Date(typeof ts === 'number' ? ts : parseInt(ts))
    if (isNaN(d.getTime())) return ''
    const now = new Date()
    if (d.toDateString() === now.toDateString()) {
      return `${d.getHours().toString().padStart(2, '0')}:${d.getMinutes().toString().padStart(2, '0')}`
    }
    const yesterday = new Date(now)
    yesterday.setDate(yesterday.getDate() - 1)
    if (d.toDateString() === yesterday.toDateString()) return '昨天'
    return `${d.getMonth() + 1}/${d.getDate()}`
  },

  // ==================== 初始化 ====================
  init() {
    const saved = localStorage.getItem('user-info')
    if (saved) {
      try {
        const u = JSON.parse(saved)
        if (u.userId) {
          this.currentUser = u
          this.isLoggedIn = true
        }
      } catch (_) {}
    }
    const screen = localStorage.getItem('current-screen')
    if (screen && this.isLoggedIn) this.currentScreen = screen
  }
})

store.init()

export default store
