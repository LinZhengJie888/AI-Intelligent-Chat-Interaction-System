/**
 * 全局状态管理
 *
 * 管理页面导航、用户信息、聊天数据等前端状态。
 * 通过WebSocket与后端通信。
 */

import { reactive } from 'vue'
import { wsClient, MessageType } from '../api/websocket'

// ========== 颜色生成器 ==========
const AVATAR_COLORS = ['#576B95', '#07C160', '#FA5151', '#FF8800', '#E91E63', '#9C27B0', '#2196F3', '#FF9800']
function getColorForId(id) {
  let hash = 0
  const str = String(id)
  for (let i = 0; i < str.length; i++) {
    hash = str.charCodeAt(i) + ((hash << 5) - hash)
  }
  return AVATAR_COLORS[Math.abs(hash) % AVATAR_COLORS.length]
}

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
  friends: [],
  groups: [],
  recentChats: [],
  friendRequests: [],
  messages: [],

  // ========== AI 设置（聊天级别，共享） ==========
  chatAISettings: {},
  currentAISettings: { nickname: 'AI助手', tone: 0, priority: 1 },

  // ========== WebSocket状态 ==========
  wsConnected: false,

  // ========== 错误信息 ==========
  errorMessage: '',

  // ==================== WebSocket连接 ====================
  
  /** 初始化WebSocket连接 */
  async connectWebSocket() {
    try {
      await wsClient.connect('ws://localhost:8081')
      this.wsConnected = true
      this._registerHandlers()
      console.log('[Store] WebSocket连接成功')
    } catch (err) {
      console.error('[Store] WebSocket连接失败:', err)
      this.wsConnected = false
    }
  },

  /** 注册消息处理器 */
  _registerHandlers() {
    // 登录响应
    wsClient.on(MessageType.LOGIN, (msg) => {
      if (msg.code === 0 && msg.data) {
        const user = msg.data
        this.isLoggedIn = true
        this.currentUser = {
          userId: user.user_id || '',
          username: user.username || '',
          nickname: user.nickname || '',
          phone: user.phone || '',
          avatarPath: user.avatar_path || ''
        }
        localStorage.setItem('user-info', JSON.stringify(this.currentUser))
        this.switchScreen('chatlist')
        // 登录成功后加载好友列表和群聊列表
        this.loadFriends()
        this.loadGroups()
      } else {
        this.errorMessage = msg.msg || '登录失败'
      }
    })

    // 注册响应
    wsClient.on(MessageType.REGISTER, (msg) => {
      if (msg.code === 0) {
        this.errorMessage = ''
        // 注册成功，提示用户登录
      } else {
        this.errorMessage = msg.msg || '注册失败'
      }
    })

    // 好友列表响应
    wsClient.on(MessageType.FRIEND_LIST, (msg) => {
      if (msg.code === 0 && msg.data) {
        this.friends = (Array.isArray(msg.data) ? msg.data : []).map(f => ({
          userId: f.user_id || f.userId || '',
          username: f.username || '',
          avatarPath: f.avatar_path || f.avatarPath || '',
          color: getColorForId(f.user_id || f.userId)
        }))
      }
    })

    // 群聊列表响应
    wsClient.on(MessageType.GROUP_LIST, (msg) => {
      if (msg.code === 0 && msg.data) {
        this.groups = (Array.isArray(msg.data) ? msg.data : []).map(g => ({
          groupId: g.group_id || g.groupId || '',
          groupName: g.group_name || g.groupName || '',
          memberCount: g.member_count || g.memberCount || 0,
          preview: '',
          colors: AVATAR_COLORS.slice(0, 4)
        }))
      }
    })

    // 好友请求列表响应
    wsClient.on(MessageType.FRIEND_REQUEST_LIST, (msg) => {
      if (msg.code === 0 && msg.data) {
        this.friendRequests = (Array.isArray(msg.data) ? msg.data : []).map(r => ({
          fromUserId: r.from_user_id || r.fromUserId || '',
          fromUsername: r.from_username || r.fromUsername || '',
          requestMsg: r.request_msg || r.requestMsg || '',
          avatarPath: r.avatar_path || r.avatarPath || '',
          color: getColorForId(r.from_user_id || r.fromUserId)
        }))
      }
    })

    // 私聊消息响应
    wsClient.on(MessageType.CHAT_PRIVATE, (msg) => {
      if (msg.code === 0 && msg.data) {
        const data = msg.data
        // 如果是自己发的消息，不重复添加
        if (data.from_user_id === this.currentUser.userId) return
        
        this.messages.push({
          type: 'other',
          sender: data.from_user_id,
          text: data.content || '',
          time: this._fmtTime(Date.now()),
          timestamp: Date.now(),
          color: getColorForId(data.from_user_id)
        })
        
        // 更新最近聊天
        this.upsertRecent('single', data.from_user_id, data.from_user_id, data.content)
      }
    })

    // 群聊消息响应
    wsClient.on(MessageType.GROUP_MESSAGE, (msg) => {
      if (msg.code === 0 && msg.data) {
        const data = msg.data
        // 如果是自己发的消息，不重复添加
        if (data.from_user_id === this.currentUser.userId) return
        
        const sender = this.friends.find(f => f.userId === data.from_user_id)
        this.messages.push({
          type: 'other',
          sender: data.from_user_id,
          name: sender ? sender.username : data.from_user_id,
          text: data.content || '',
          time: this._fmtTime(Date.now()),
          timestamp: Date.now(),
          color: getColorForId(data.from_user_id)
        })
        
        // 更新最近聊天
        this.upsertRecent('group', data.group_id, data.group_id, `${sender ? sender.username : data.from_user_id}: ${data.content}`)
      }
    })

    // 聊天记录响应
    wsClient.on(MessageType.CHAT_HISTORY, (msg) => {
      if (msg.code === 0 && msg.data) {
        const records = Array.isArray(msg.data) ? msg.data : []
        this.messages = records.map(r => ({
          type: r.is_ai ? 'ai' : (r.sender_id === this.currentUser.userId ? 'mine' : 'other'),
          sender: r.sender_id,
          text: r.content || '',
          time: this._fmtTime(r.send_time),
          timestamp: new Date(r.send_time).getTime(),
          color: getColorForId(r.sender_id)
        }))
      }
    })

    // AI请求响应
    wsClient.on(MessageType.AI_REQUEST, (msg) => {
      if (msg.code === 0) {
        console.log('[Store] AI请求已接收')
      }
    })

    // AI回复（通过私聊或群聊消息接收）
    // AI设置响应
    wsClient.on(MessageType.AI_SETTING, (msg) => {
      if (msg.code === 0 && msg.data) {
        const data = msg.data
        const chatKey = data.chat_key
        if (chatKey) {
          this.chatAISettings[chatKey] = {
            nickname: data.nickname || 'AI助手',
            tone: data.tone || 0,
            priority: data.priority || 1
          }
          // 如果是当前聊天，更新currentAISettings
          const currentChatKey = this.getChatKey(this.currentChat.type, this.currentChat.id)
          if (chatKey === currentChatKey) {
            this.currentAISettings = { ...this.chatAISettings[chatKey] }
          }
        }
      }
    })

    // 好友添加请求通知
    wsClient.on(MessageType.FRIEND_ADD, (msg) => {
      if (msg.code === 0 && msg.data) {
        // 收到好友请求通知
        console.log('[Store] 收到好友请求:', msg.data)
      }
    })

    // 通用响应处理
    wsClient.on(MessageType.RESPONSE_OK, (msg) => {
      console.log('[Store] 操作成功:', msg.msg)
    })

    wsClient.on(MessageType.RESPONSE_ERROR, (msg) => {
      console.error('[Store] 操作失败:', msg.msg)
      this.errorMessage = msg.msg || '操作失败'
    })
  },

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
    // 加载聊天记录
    this.loadChatHistory(type, id)
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
    // 发送到后端
    wsClient.send({
      type: MessageType.AI_SETTING,
      from_user_id: this.currentUser.userId,
      to_user_id: '',
      content: '',
      extra: JSON.stringify({
        chatKey,
        nickname: settings.nickname,
        tone: settings.tone,
        priority: settings.priority
      }),
      timestamp: String(Date.now())
    })
  },

  // ==================== 用户操作 ====================

  /** 登录 */
  login(userId, password) {
    wsClient.send({
      type: MessageType.LOGIN,
      from_user_id: userId,
      to_user_id: '',
      content: '',
      extra: JSON.stringify({
        password: password
      }),
      timestamp: String(Date.now())
    })
  },

  /** 注册 */
  register(userId, username, password, phone) {
    wsClient.send({
      type: MessageType.REGISTER,
      from_user_id: userId,
      to_user_id: '',
      content: '',
      extra: JSON.stringify({
        username,
        password,
        phone: phone || ''
      }),
      timestamp: String(Date.now())
    })
  },

  /** 登出 */
  logout() {
    wsClient.send({
      type: MessageType.LOGOUT,
      from_user_id: this.currentUser.userId,
      to_user_id: '',
      content: '',
      extra: '',
      timestamp: String(Date.now())
    })
    
    this.isLoggedIn = false
    this.currentUser = { userId: '', username: '', nickname: '', phone: '', avatarPath: '' }
    this.messages = []
    this.friends = []
    this.groups = []
    this.recentChats = []
    this.friendRequests = []
    localStorage.removeItem('user-info')
    this.switchScreen('login')
    
    // 断开WebSocket连接
    wsClient.disconnect()
    this.wsConnected = false
  },

  /** 更新用户信息 */
  updateProfile(info) {
    Object.assign(this.currentUser, info)
    localStorage.setItem('user-info', JSON.stringify(this.currentUser))
  },

  /** 修改用户名 */
  updateUsername(newUsername) {
    wsClient.send({
      type: MessageType.UPDATE_USERNAME,
      from_user_id: this.currentUser.userId,
      to_user_id: '',
      content: '',
      extra: JSON.stringify({ username: newUsername }),
      timestamp: String(Date.now())
    })
    this.currentUser.username = newUsername
    localStorage.setItem('user-info', JSON.stringify(this.currentUser))
  },

  // ==================== 数据加载 ====================

  /** 加载好友列表 */
  loadFriends() {
    wsClient.send({
      type: MessageType.FRIEND_LIST,
      from_user_id: this.currentUser.userId,
      to_user_id: '',
      content: '',
      extra: '',
      timestamp: String(Date.now())
    })
  },

  /** 加载群聊列表 */
  loadGroups() {
    wsClient.send({
      type: MessageType.GROUP_LIST,
      from_user_id: this.currentUser.userId,
      to_user_id: '',
      content: '',
      extra: '',
      timestamp: String(Date.now())
    })
  },

  /** 加载好友请求列表 */
  loadFriendRequests() {
    wsClient.send({
      type: MessageType.FRIEND_REQUEST_LIST,
      from_user_id: this.currentUser.userId,
      to_user_id: '',
      content: '',
      extra: '',
      timestamp: String(Date.now())
    })
  },

  /** 加载聊天记录 */
  loadChatHistory(type, id) {
    wsClient.send({
      type: MessageType.CHAT_HISTORY,
      from_user_id: this.currentUser.userId,
      to_user_id: id,
      content: '',
      extra: JSON.stringify({ is_group: type === 'group' }),
      timestamp: String(Date.now())
    })
  },

  // ==================== 聊天操作 ====================

  /** 发送消息 */
  sendMessage(text, type) {
    const ts = Date.now()
    
    if (type === 'ai') {
      // AI请求
      wsClient.send({
        type: MessageType.AI_REQUEST,
        from_user_id: this.currentUser.userId,
        to_user_id: 'ai',
        content: text,
        extra: JSON.stringify({
          chatKey: 'ai:ai',
          is_group: false
        }),
        timestamp: String(ts)
      })
    } else if (type === 'group') {
      // 群聊消息
      wsClient.send({
        type: MessageType.GROUP_MESSAGE,
        from_user_id: this.currentUser.userId,
        to_user_id: this.currentChat.id,
        content: text,
        extra: '',
        timestamp: String(ts)
      })
    } else {
      // 私聊消息
      wsClient.send({
        type: MessageType.CHAT_PRIVATE,
        from_user_id: this.currentUser.userId,
        to_user_id: this.currentChat.id,
        content: text,
        extra: '',
        timestamp: String(ts)
      })
    }
    
    // 本地添加消息
    this.messages.push({
      type: 'mine',
      sender: this.currentUser.userId,
      text,
      time: this._fmtTime(ts),
      timestamp: ts
    })
    
    // 更新最近聊天
    const name = type === 'ai' ? (this.currentAISettings.nickname || 'AI助手') : this.currentChat.name
    this.upsertRecent(type, this.currentChat.id, name, text)
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
        timestamp: ts,
        color: getColorForId(id)
      })
    }
  },

  // ==================== 好友操作 ====================

  /** 添加好友 */
  addFriend(friendId, message) {
    wsClient.send({
      type: MessageType.FRIEND_ADD,
      from_user_id: this.currentUser.userId,
      to_user_id: friendId,
      content: message || '',
      extra: '',
      timestamp: String(Date.now())
    })
  },

  /** 同意好友请求 */
  agreeFriend(fromUserId) {
    wsClient.send({
      type: MessageType.FRIEND_AGREE,
      from_user_id: this.currentUser.userId,
      to_user_id: fromUserId,
      content: '',
      extra: '',
      timestamp: String(Date.now())
    })
    this.friendRequests = this.friendRequests.filter(r => r.fromUserId !== fromUserId)
  },

  /** 拒绝好友请求 */
  rejectFriend(fromUserId) {
    wsClient.send({
      type: MessageType.FRIEND_REJECT,
      from_user_id: this.currentUser.userId,
      to_user_id: fromUserId,
      content: '',
      extra: '',
      timestamp: String(Date.now())
    })
    this.friendRequests = this.friendRequests.filter(r => r.fromUserId !== fromUserId)
  },

  /** 删除好友 */
  deleteFriend(friendId) {
    wsClient.send({
      type: MessageType.FRIEND_DELETE,
      from_user_id: this.currentUser.userId,
      to_user_id: friendId,
      content: '',
      extra: '',
      timestamp: String(Date.now())
    })
    this.friends = this.friends.filter(f => f.userId !== friendId)
  },

  // ==================== 群聊操作 ====================

  /** 创建群聊 */
  createGroup(groupName) {
    wsClient.send({
      type: MessageType.GROUP_CREATE,
      from_user_id: this.currentUser.userId,
      to_user_id: '',
      content: '',
      extra: JSON.stringify({ group_name: groupName }),
      timestamp: String(Date.now())
    })
  },

  /** 加入群聊 */
  joinGroup(groupId, message) {
    wsClient.send({
      type: MessageType.GROUP_JOIN,
      from_user_id: this.currentUser.userId,
      to_user_id: groupId,
      content: message || '',
      extra: '',
      timestamp: String(Date.now())
    })
  },

  /** 退出群聊 */
  leaveGroup(groupId) {
    wsClient.send({
      type: MessageType.GROUP_LEAVE,
      from_user_id: this.currentUser.userId,
      to_user_id: groupId,
      content: '',
      extra: '',
      timestamp: String(Date.now())
    })
    this.groups = this.groups.filter(g => g.groupId !== groupId)
  },

  /** 修改群名 */
  modifyGroupName(groupId, newName) {
    wsClient.send({
      type: MessageType.GROUP_MODIFY_NAME,
      from_user_id: this.currentUser.userId,
      to_user_id: groupId,
      content: '',
      extra: JSON.stringify({ group_name: newName }),
      timestamp: String(Date.now())
    })
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
  async init() {
    // 从localStorage恢复用户信息
    const saved = localStorage.getItem('user-info')
    if (saved) {
      try {
        const u = JSON.parse(saved)
        if (u.userId) {
          this.currentUser = u
          // 注意：这里只恢复用户信息，不设置isLoggedIn
          // 登录状态需要通过WebSocket重新验证
        }
      } catch (_) {}
    }
    
    // 初始化WebSocket连接
    await this.connectWebSocket()
  }
})

store.init()

export default store
