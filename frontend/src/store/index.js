/**
 * 全局状态管理
 *
 * 与后端 Message 结构体对齐：type, from_user_id, to_user_id, content, extra, timestamp
 * 后端响应格式：{ type, code, msg, data, timestamp }
 */

import { reactive } from 'vue'
import { wsClient, MessageType } from '../api/websocket'

const AVATAR_COLORS = ['#576B95', '#07C160', '#FA5151', '#FF8800', '#E91E63', '#9C27B0', '#2196F3', '#FF9800']
function getColorForId(id) {
  let hash = 0
  const str = String(id)
  for (let i = 0; i < str.length; i++) {
    hash = str.charCodeAt(i) + ((hash << 5) - hash)
  }
  return AVATAR_COLORS[Math.abs(hash) % AVATAR_COLORS.length]
}

/** 拼接头像完整URL */
function fullAvatarUrl(path) {
  if (!path) return ''
  if (path.startsWith('data:') || path.startsWith('http')) return path
  return `http://${window.location.hostname}:8082${path}`
}

const store = reactive({
  // ========== 用户状态 ==========
  isLoggedIn: false,
  currentUser: { userId: '', username: '', nickname: '', phone: '', avatarPath: '' },

  // ========== 页面导航 ==========
  currentScreen: 'login',

  // ========== 当前聊天上下文 ==========
  currentChat: { type: '', id: '', name: '' },

  // ========== 数据列表 ==========
  friends: [],
  groups: [],
  recentChats: [],
  friendRequests: [],
  groupRequests: [],  // 群聊请求列表
  messagesMap: {},  // 按 chatKey 隔离消息
  messages: [],      // 当前显示的消息（引用 messagesMap 中的数据）

  // ========== AI 设置 ==========
  chatAISettings: {},
  currentAISettings: { nickname: 'AI助手', tone: 0, priority: 1 },

  // ========== 连接状态 ==========
  wsConnected: false,
  wsConnecting: false,

  // ========== 错误/成功信息 ==========
  errorMessage: '',
  successMessage: '',

  // ========== 回调 ==========
  _loginCallback: null,
  _registerCallback: null,

  // ==================== WebSocket 连接 ====================

  async connectWebSocket() {
    if (this.wsConnecting) return
    this.wsConnecting = true
    try {
      this._registerHandlers()
      await wsClient.connect('ws://localhost:8081')
      this.wsConnected = true
      this.wsConnecting = false
      console.log('[Store] WebSocket 连接成功')
      if (this.isLoggedIn) {
        this.loadFriends()
        this.loadGroups()
        this.loadFriendRequests()
      }
    } catch (err) {
      console.error('[Store] WebSocket 连接失败:', err)
      this.wsConnected = false
      this.wsConnecting = false
      this.errorMessage = '连接服务器失败，请检查后端是否启动'
    }
  },

  /** 获取当前聊天的 key */
  _chatKey(type, id) {
    return `${type}:${id}`
  },

  /** 获取当前聊天的消息数组引用 */
  _getMessages(type, id) {
    const key = this._chatKey(type, id)
    if (!this.messagesMap[key]) {
      this.messagesMap[key] = []
    }
    return this.messagesMap[key]
  },

  /** 注册消息处理器 */
  _registerHandlers() {
    wsClient.handlers.clear()

    // ===== 登录响应 =====
    wsClient.on(MessageType.LOGIN, (msg) => {
      console.log('[Store] 登录响应:', msg)
      if (msg.code === 0) {
        const user = msg.data || {}
        const savedInfo = localStorage.getItem('user-info')
        let localAvatar = ''
        try { localAvatar = savedInfo ? JSON.parse(savedInfo).avatarPath : '' } catch (_) {}
        const serverAvatar = fullAvatarUrl(user.avatar_path || '')
        const avatarPath = (localAvatar && localAvatar.startsWith('data:')) ? localAvatar : serverAvatar

        this.isLoggedIn = true
        this.currentUser = {
          userId: user.user_id || msg.from_user_id || '',
          username: user.username || '',
          nickname: user.nickname || '',
          phone: user.phone || '',
          avatarPath
        }
        this.errorMessage = ''
        this.successMessage = '登录成功'
        localStorage.setItem('user-info', JSON.stringify(this.currentUser))
        wsClient.setUserId(this.currentUser.userId)
        this.switchScreen('chatlist')
        this.loadFriends()
        this.loadGroups()
        this.loadFriendRequests()
        if (this._loginCallback) { this._loginCallback(true); this._loginCallback = null }
      } else {
        this.errorMessage = msg.msg || '登录失败'
        this.successMessage = ''
        if (this._loginCallback) { this._loginCallback(false); this._loginCallback = null }
      }
    })

    // ===== 注册响应 =====
    wsClient.on(MessageType.REGISTER, (msg) => {
      console.log('[Store] 注册响应:', msg)
      if (msg.code === 0) {
        this.errorMessage = ''
        this.successMessage = '注册成功，请登录'
        if (this._registerCallback) { this._registerCallback(true); this._registerCallback = null }
      } else {
        this.errorMessage = msg.msg || '注册失败'
        this.successMessage = ''
        if (this._registerCallback) { this._registerCallback(false); this._registerCallback = null }
      }
    })

    // ===== 好友列表响应 =====
    wsClient.on(MessageType.FRIEND_LIST, (msg) => {
      console.log('[Store] 好友列表响应:', msg)
      if (msg.code === 0) {
        const list = Array.isArray(msg.data) ? msg.data : []
        this.friends = list.map(f => ({
          userId: f.user_id || '',
          username: f.username || f.nickname || '',
          avatarPath: fullAvatarUrl(f.avatar_path || ''),
          color: getColorForId(f.user_id)
        }))
      }
    })

    // ===== 群聊列表响应 =====
    wsClient.on(MessageType.GROUP_LIST, (msg) => {
      console.log('[Store] 群聊列表响应:', msg)
      if (msg.code === 0) {
        const list = Array.isArray(msg.data) ? msg.data : []
        this.groups = list.map(g => ({
          groupId: g.group_id || '',
          groupName: g.group_name || '',
          memberCount: g.member_count || 0,
          preview: '',
          colors: AVATAR_COLORS.slice(0, 4)
        }))
      }
    })

    // ===== 好友请求列表响应 =====
    wsClient.on(MessageType.FRIEND_REQUEST_LIST, (msg) => {
      console.log('[Store] 好友请求列表响应:', msg)
      if (msg.code === 0) {
        const list = Array.isArray(msg.data) ? msg.data : []
        this.friendRequests = list.map(r => ({
          fromUserId: r.from_user_id || '',
          fromUsername: r.from_username || r.username || '',
          requestMsg: r.request_msg || '',
          avatarPath: fullAvatarUrl(r.from_avatar || r.avatar_path || ''),
          color: getColorForId(r.from_user_id)
        }))
      }
    })

    // ===== 私聊消息 =====
    wsClient.on(MessageType.CHAT_PRIVATE, (msg) => {
      console.log('[Store] 私聊消息:', msg)
      if (msg.code === 0 && msg.data) {
        const d = msg.data
        let isAi = false
        let chatKey = ''
        try {
          const extraObj = typeof d.extra === 'string' ? JSON.parse(d.extra) : (d.extra || {})
          isAi = extraObj.is_ai === true || extraObj.is_ai === 'true'
          chatKey = extraObj.chat_key || ''
        } catch (_) {}

        if (d.from_user_id === this.currentUser.userId && !isAi) return

        // AI回复：使用extra中的chat_key确定存储位置
        // 普通私聊：存入 single:对方ID
        let msgType = 'single'
        let chatId = d.from_user_id
        
        if (isAi && chatKey) {
          // 从chat_key解析类型和ID（例如 "single:1234" 或 "ai:ai"）
          const parts = chatKey.split(':')
          msgType = parts[0] || 'single'
          chatId = parts.slice(1).join(':') || d.from_user_id
        }
        
        const msgs = this._getMessages(msgType, chatId)
        msgs.push({
          type: isAi ? 'ai' : 'other',
          sender: d.from_user_id || 'AI',
          name: isAi ? (this.chatAISettings[chatKey]?.nickname || 'AI助手') : undefined,
          text: d.content || '',
          time: this._fmtTime(d.timestamp || Date.now()),
          timestamp: parseInt(d.timestamp) || Date.now(),
          color: isAi ? undefined : getColorForId(d.from_user_id)
        })
        this._saveMessages(msgType, chatId)
        this._refreshCurrentMessages()

        if (!isAi) {
          this.upsertRecent('single', d.from_user_id, d.from_user_id, d.content)
        }
      }
    })

    // ===== 群聊消息 =====
    wsClient.on(MessageType.GROUP_MESSAGE, (msg) => {
      console.log('[Store] 群聊消息:', msg)
      if (msg.code === 0 && msg.data) {
        const d = msg.data
        if (d.from_user_id === this.currentUser.userId) return

        let isAi = false
        try {
          const extraObj = typeof d.extra === 'string' ? JSON.parse(d.extra) : (d.extra || {})
          isAi = extraObj.is_ai === true || extraObj.is_ai === 'true'
        } catch (_) {}

        const sender = this.friends.find(f => f.userId === d.from_user_id)
        const groupId = d.group_id || d.to_user_id
        const msgs = this._getMessages('group', groupId)
        msgs.push({
          type: isAi ? 'ai' : 'other',
          sender: d.from_user_id || 'AI',
          name: isAi ? (this.chatAISettings[`group:${groupId}`]?.nickname || d.from_user_id || 'AI助手') : (sender ? sender.username : d.from_user_id),
          text: d.content || '',
          time: this._fmtTime(d.timestamp || Date.now()),
          timestamp: parseInt(d.timestamp) || Date.now(),
          color: isAi ? undefined : getColorForId(d.from_user_id)
        })
        this._saveMessages('group', groupId)
        this._refreshCurrentMessages()

        this.upsertRecent('group', groupId, groupId,
          `${sender ? sender.username : d.from_user_id}: ${d.content}`)
      }
    })

    // ===== 聊天记录响应 =====
    wsClient.on(MessageType.CHAT_HISTORY, (msg) => {
      console.log('[Store] 聊天记录响应:', msg)
      if (msg.code === 0) {
        const records = Array.isArray(msg.data) ? msg.data : []
        records.reverse()
        const historyMsgs = records.map(r => ({
          type: r.is_ai ? 'ai' : (String(r.sender_id) === String(this.currentUser.userId) ? 'mine' : 'other'),
          sender: String(r.sender_id),
          text: r.content || '',
          time: this._fmtTime(r.send_time),
          timestamp: new Date(r.send_time).getTime() || 0,
          color: r.is_ai ? undefined : getColorForId(r.sender_id)
        }))
        // 合并：保留本地已有的新消息，追加历史记录
        const key = this._chatKey(this.currentChat.type, this.currentChat.id)
        const localMsgs = this.messagesMap[key] || []
        const localTimestamps = new Set(localMsgs.map(m => m.timestamp))
        const newHistory = historyMsgs.filter(m => !localTimestamps.has(m.timestamp))
        this.messagesMap[key] = [...newHistory, ...localMsgs]
        this.messages = this.messagesMap[key]
      }
    })

    // ===== AI 请求确认 =====
    wsClient.on(MessageType.AI_REQUEST, (msg) => {
      console.log('[Store] AI 请求响应:', msg)
    })

    // ===== AI_AT 请求确认 =====
    wsClient.on(MessageType.AI_AT, (msg) => {
      console.log('[Store] AI_AT 响应:', msg)
    })

    // ===== 好友添加请求通知 =====
    wsClient.on(MessageType.FRIEND_ADD, (msg) => {
      console.log('[Store] 好友请求通知:', msg)
      if (msg.code === 0) {
        if (msg.data && msg.data.from_user_id) {
          const exists = this.friendRequests.find(r => r.fromUserId === msg.data.from_user_id)
          if (!exists) {
            this.friendRequests.push({
              fromUserId: msg.data.from_user_id,
              fromUsername: msg.data.from_username || msg.data.from_user_id,
              requestMsg: msg.data.request_msg || msg.msg || '',
              avatarPath: fullAvatarUrl(msg.data.avatar_path || ''),
              color: getColorForId(msg.data.from_user_id)
            })
          }
          this.successMessage = '收到新好友请求'
        } else {
          this.successMessage = msg.msg || '好友请求已发送'
        }
        this.errorMessage = ''
      } else {
        this.errorMessage = msg.msg || '好友请求失败'
      }
    })

    // ===== 好友同意通知 =====
    wsClient.on(MessageType.FRIEND_AGREE, (msg) => {
      console.log('[Store] 好友同意通知:', msg)
      if (msg.code === 0) {
        this.successMessage = msg.msg || '好友请求已同意'
        this.errorMessage = ''
        this.loadFriends()
        if (msg.data && msg.data.from_user_id) {
          this.friendRequests = this.friendRequests.filter(r => r.fromUserId !== msg.data.from_user_id)
        }
      }
    })

    // ===== 好友拒绝通知 =====
    wsClient.on(MessageType.FRIEND_REJECT, (msg) => {
      console.log('[Store] 好友拒绝通知:', msg)
      if (msg.code === 0) {
        this.successMessage = msg.msg || '好友请求已拒绝'
        this.errorMessage = ''
        if (msg.data && msg.data.from_user_id) {
          this.friendRequests = this.friendRequests.filter(r => r.fromUserId !== msg.data.from_user_id)
        }
      }
    })

    // ===== 群聊创建响应 =====
    wsClient.on(MessageType.GROUP_CREATE, (msg) => {
      console.log('[Store] 创建群聊响应:', msg)
      if (msg.code === 0) {
        const data = msg.data || {}
        this.successMessage = `群聊创建成功，群ID: ${data.group_id || ''}`
        this.errorMessage = ''
        this.loadGroups()
      } else {
        this.errorMessage = msg.msg || '创建群聊失败'
      }
    })

    // ===== 加群申请响应 =====
    wsClient.on(MessageType.GROUP_JOIN, (msg) => {
      console.log('[Store] 加群申请响应:', msg)
      if (msg.code === 0) {
        this.successMessage = msg.msg || '加群申请已发送'
        this.errorMessage = ''
      } else {
        this.errorMessage = msg.msg || '加群申请失败'
        this.successMessage = ''
      }
    })

    // ===== 群聊同意通知 =====
    wsClient.on(MessageType.GROUP_AGREE, (msg) => {
      console.log('[Store] 群聊同意通知:', msg)
      if (msg.code === 0) {
        this.successMessage = msg.msg || '加群申请已同意'
        this.errorMessage = ''
        this.loadGroups()
        this.loadGroupRequests()
      }
    })

    // ===== 群聊拒绝通知 =====
    wsClient.on(MessageType.GROUP_REJECT, (msg) => {
      console.log('[Store] 群聊拒绝通知:', msg)
      if (msg.code === 0) {
        this.successMessage = msg.msg || '加群申请已拒绝'
        this.errorMessage = ''
        this.loadGroupRequests()
      }
    })

    // ===== 群聊请求列表响应 =====
    wsClient.on(MessageType.GROUP_REQUEST_LIST, (msg) => {
      console.log('[Store] 群聊请求列表响应:', msg)
      if (msg.code === 0) {
        const list = Array.isArray(msg.data) ? msg.data : []
        this.groupRequests = list.map(r => ({
          requestId: r.request_id || 0,
          groupId: r.group_id || '',
          groupName: r.group_name || '',
          fromUserId: r.from_user_id || '',
          fromUsername: r.from_username || r.from_user_id || '',
          requestMsg: r.request_msg || '',
          avatarPath: fullAvatarUrl(r.from_avatar || r.avatar_path || ''),
          color: getColorForId(r.from_user_id)
        }))
      }
    })

    // ===== 头像上传响应 =====
    wsClient.on(MessageType.UPLOAD_AVATAR, (msg) => {
      console.log('[Store] 头像上传响应:', msg)
      if (msg.code === 0 && msg.data) {
        this.successMessage = '头像上传成功'
        this.errorMessage = ''
      } else {
        this.errorMessage = msg.msg || '头像上传失败'
      }
    })

    // ===== 通用成功响应 =====
    wsClient.on(MessageType.RESPONSE_OK, (msg) => {
      console.log('[Store] 操作成功:', msg.msg)
    })

    // ===== 通用错误响应 =====
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
    this._saveCurrentMessages()
    this.currentChat = { type, id: id || '', name: name || '' }
    if (type === 'group') this.switchScreen('group-chat')
    else if (type === 'ai') this.switchScreen('ai-chat')
    else this.switchScreen('single-chat')
    this.messages = this._getMessages(type, id)
    this.loadChatHistory(type, id)
    
    // 加载该聊天对应的AI设置
    const chatKey = `${type}:${id}`
    if (this.chatAISettings[chatKey]) {
      this.currentAISettings = { ...this.chatAISettings[chatKey] }
    } else {
      this.currentAISettings = { nickname: 'AI助手', tone: 0, priority: 1 }
    }
  },

  /** 保存当前聊天消息到 localStorage */
  _saveCurrentMessages() {
    if (this.currentChat.id && this.messages.length > 0) {
      const key = `chat-${this.currentChat.type}-${this.currentChat.id}`
      try {
        // 只保存最近100条，避免localStorage爆满
        const toSave = this.messages.slice(-100)
        localStorage.setItem(key, JSON.stringify(toSave))
      } catch (e) {
        console.warn('保存消息失败:', e)
      }
    }
  },

  /** 保存指定聊天的消息到 localStorage */
  _saveMessages(type, id) {
    const key = `chat-${type}-${id}`
    const msgs = this.messagesMap[this._chatKey(type, id)]
    if (msgs && msgs.length > 0) {
      try {
        const toSave = msgs.slice(-100)
        localStorage.setItem(key, JSON.stringify(toSave))
      } catch (e) {
        console.warn('保存消息失败:', e)
      }
    }
  },

  /** 刷新当前显示的消息引用 */
  _refreshCurrentMessages() {
    const key = this._chatKey(this.currentChat.type, this.currentChat.id)
    if (this.messagesMap[key]) {
      this.messages = this.messagesMap[key]
    }
  },

  /** 从 localStorage 加载聊天消息 */
  _loadMessagesFromStorage(type, id) {
    if (!id) return []
    const key = `chat-${type}-${id}`
    try {
      const saved = localStorage.getItem(key)
      return saved ? JSON.parse(saved) : []
    } catch (e) {
      return []
    }
  },

  // ==================== 用户操作 ====================

  login(userId, password) {
    this.errorMessage = ''
    this.successMessage = ''
    return new Promise((resolve) => {
      this._loginCallback = resolve
      wsClient.send({
        type: MessageType.LOGIN,
        from_user_id: userId,
        to_user_id: '',
        content: '',
        extra: JSON.stringify({ password }),
        timestamp: String(Date.now())
      })
    })
  },

  register(userId, username, password, phone) {
    this.errorMessage = ''
    this.successMessage = ''
    return new Promise((resolve) => {
      this._registerCallback = resolve
      wsClient.send({
        type: MessageType.REGISTER,
        from_user_id: userId,
        to_user_id: '',
        content: '',
        extra: JSON.stringify({ username, password, phone: phone || '' }),
        timestamp: String(Date.now())
      })
    })
  },

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
    this.messagesMap = {}
    this.friends = []
    this.groups = []
    this.recentChats = []
    this.friendRequests = []
    this.errorMessage = ''
    this.successMessage = ''
    localStorage.removeItem('user-info')
    localStorage.removeItem('recent-chats')
    this.switchScreen('login')
    wsClient.disconnect()
    this.wsConnected = false
  },

  updateProfile(info) {
    Object.assign(this.currentUser, info)
    localStorage.setItem('user-info', JSON.stringify(this.currentUser))
    wsClient.send({
      type: MessageType.UPDATE_USERNAME,
      from_user_id: this.currentUser.userId,
      to_user_id: '',
      content: '',
      extra: JSON.stringify({ username: info.username || this.currentUser.username }),
      timestamp: String(Date.now())
    })
  },

  uploadAvatar(base64Data, format) {
    wsClient.send({
      type: MessageType.UPLOAD_AVATAR,
      from_user_id: this.currentUser.userId,
      to_user_id: '',
      content: '',
      extra: JSON.stringify({ avatar_data: base64Data, format: format || 'png' }),
      timestamp: String(Date.now())
    })
  },

  // ==================== 数据加载 ====================

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

  sendMessage(text, type) {
    const ts = Date.now()
    const chatKey = `${type}:${this.currentChat.id}`

    // 检测 @AI 前缀
    if (type !== 'ai' && /^@AI[\s,，]/.test(text)) {
      const question = text.replace(/^@AI[\s,，]*/, '').trim()
      if (question) {
        // 使用当前聊天的AI设置
        const aiSettings = this.chatAISettings[chatKey] || { nickname: 'AI助手', tone: 0, priority: 1 }
        
        // 先发送普通消息给对方（让对方能看到@AI消息）
        if (type === 'group') {
          wsClient.send({
            type: MessageType.GROUP_MESSAGE,
            from_user_id: this.currentUser.userId,
            to_user_id: this.currentChat.id,
            content: text,
            extra: '',
            timestamp: String(ts)
          })
        } else {
          wsClient.send({
            type: MessageType.CHAT_PRIVATE,
            from_user_id: this.currentUser.userId,
            to_user_id: this.currentChat.id,
            content: text,
            extra: '',
            timestamp: String(ts)
          })
        }
        
        // 再发送AI_AT请求触发AI处理
        wsClient.send({
          type: MessageType.AI_AT,
          from_user_id: this.currentUser.userId,
          to_user_id: this.currentChat.id,
          content: question,
          extra: JSON.stringify({
            ai_nickname: aiSettings.nickname || 'AI助手',
            chatKey,
            is_group: type === 'group'
          }),
          timestamp: String(ts)
        })
        
        const msgs = this._getMessages(type, this.currentChat.id)
        msgs.push({
          type: 'mine',
          sender: this.currentUser.userId,
          text,
          time: this._fmtTime(ts),
          timestamp: ts
        })
        this._saveMessages(type, this.currentChat.id)
        this._refreshCurrentMessages()
        this.upsertRecent(type, this.currentChat.id, this.currentChat.name, text)
        return
      }
    }

    if (type === 'ai') {
      // 使用当前聊天的AI设置
      const aiSettings = this.chatAISettings[chatKey] || { nickname: 'AI助手', tone: 0, priority: 1 }
      wsClient.send({
        type: MessageType.AI_REQUEST,
        from_user_id: this.currentUser.userId,
        to_user_id: 'ai',
        content: text,
        extra: JSON.stringify({
          chatKey,
          is_group: false,
          ai_nickname: aiSettings.nickname || 'AI助手'
        }),
        timestamp: String(ts)
      })
    } else if (type === 'group') {
      wsClient.send({
        type: MessageType.GROUP_MESSAGE,
        from_user_id: this.currentUser.userId,
        to_user_id: this.currentChat.id,
        content: text,
        extra: '',
        timestamp: String(ts)
      })
    } else {
      wsClient.send({
        type: MessageType.CHAT_PRIVATE,
        from_user_id: this.currentUser.userId,
        to_user_id: this.currentChat.id,
        content: text,
        extra: '',
        timestamp: String(ts)
      })
    }

    const msgs = this._getMessages(type, this.currentChat.id)
    msgs.push({
      type: 'mine',
      sender: this.currentUser.userId,
      text,
      time: this._fmtTime(ts),
      timestamp: ts
    })
    this._saveMessages(type, this.currentChat.id)
    this._refreshCurrentMessages()

    const name = type === 'ai' ? (this.currentAISettings.nickname || 'AI助手') : this.currentChat.name
    this.upsertRecent(type, this.currentChat.id, name, text)
  },

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
    localStorage.setItem('recent-chats', JSON.stringify(this.recentChats))
  },

  // ==================== 好友操作 ====================

  addFriend(friendId, message) {
    this.errorMessage = ''
    this.successMessage = ''
    wsClient.send({
      type: MessageType.FRIEND_ADD,
      from_user_id: this.currentUser.userId,
      to_user_id: friendId,
      content: message || '',
      extra: '',
      timestamp: String(Date.now())
    })
  },

  agreeFriend(fromUserId) {
    wsClient.send({
      type: MessageType.FRIEND_AGREE,
      from_user_id: this.currentUser.userId,
      to_user_id: fromUserId,
      content: '',
      extra: '',
      timestamp: String(Date.now())
    })
  },

  rejectFriend(fromUserId) {
    wsClient.send({
      type: MessageType.FRIEND_REJECT,
      from_user_id: this.currentUser.userId,
      to_user_id: fromUserId,
      content: '',
      extra: '',
      timestamp: String(Date.now())
    })
  },

  // ==================== 群聊操作 ====================

  createGroup(groupName) {
    this.errorMessage = ''
    this.successMessage = ''
    wsClient.send({
      type: MessageType.GROUP_CREATE,
      from_user_id: this.currentUser.userId,
      to_user_id: '',
      content: '',
      extra: JSON.stringify({ group_name: groupName }),
      timestamp: String(Date.now())
    })
  },

  joinGroup(groupId, message) {
    this.errorMessage = ''
    this.successMessage = ''
    wsClient.send({
      type: MessageType.GROUP_JOIN,
      from_user_id: this.currentUser.userId,
      to_user_id: groupId,
      content: message || '',
      extra: '',
      timestamp: String(Date.now())
    })
  },

  agreeGroupJoin(groupId, fromUserId) {
    wsClient.send({
      type: MessageType.GROUP_AGREE,
      from_user_id: this.currentUser.userId,
      to_user_id: fromUserId,
      content: '',
      extra: JSON.stringify({ group_id: groupId }),
      timestamp: String(Date.now())
    })
  },

  rejectGroupJoin(groupId, fromUserId) {
    wsClient.send({
      type: MessageType.GROUP_REJECT,
      from_user_id: this.currentUser.userId,
      to_user_id: fromUserId,
      content: '',
      extra: JSON.stringify({ group_id: groupId }),
      timestamp: String(Date.now())
    })
  },

  loadGroupRequests(groupId) {
    wsClient.send({
      type: MessageType.GROUP_REQUEST_LIST,
      from_user_id: this.currentUser.userId,
      to_user_id: groupId || '',
      content: '',
      extra: '',
      timestamp: String(Date.now())
    })
  },

  // ==================== AI 设置 ====================

  updateChatAISettings(chatKey, settings) {
    this.chatAISettings[chatKey] = { ...settings }
    // 只有当chatKey是当前聊天时才更新currentAISettings
    const currentChatKey = `${this.currentChat.type}:${this.currentChat.id}`
    if (chatKey === currentChatKey) {
      this.currentAISettings = { ...settings }
    }
    localStorage.setItem('ai-settings-map', JSON.stringify(this.chatAISettings))
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

  // ==================== 工具方法 ====================

  _fmtTime(ts) {
    if (!ts) return ''
    let d
    if (typeof ts === 'number') {
      d = new Date(ts)
    } else {
      d = new Date(String(ts))
    }
    if (isNaN(d.getTime())) return ''
    const now = new Date()
    if (d.toDateString() === now.toDateString()) {
      return `${d.getHours().toString().padStart(2, '0')}:${d.getMinutes().toString().padStart(2, '0')}`
    }
    const yesterday = new Date(now)
    yesterday.setDate(yesterday.getDate() - 1)
    if (d.toDateString() === yesterday.toDateString()) {
      return `昨天 ${d.getHours().toString().padStart(2, '0')}:${d.getMinutes().toString().padStart(2, '0')}`
    }
    const year = d.getFullYear()
    const curYear = now.getFullYear()
    if (year === curYear) {
      return `${d.getMonth() + 1}/${d.getDate()} ${d.getHours().toString().padStart(2, '0')}:${d.getMinutes().toString().padStart(2, '0')}`
    }
    return `${year}/${d.getMonth() + 1}/${d.getDate()}`
  },

  clearMessages() {
    this.errorMessage = ''
    this.successMessage = ''
  },

  // ==================== 初始化 ====================
  async init() {
    // 恢复用户信息
    const saved = localStorage.getItem('user-info')
    if (saved) {
      try {
        const u = JSON.parse(saved)
        if (u.userId) {
          this.currentUser = u
          this.isLoggedIn = true
          wsClient.setUserId(u.userId)
        }
      } catch (_) {}
    }

    // 恢复最近聊天
    const savedChats = localStorage.getItem('recent-chats')
    if (savedChats) {
      try { this.recentChats = JSON.parse(savedChats) || [] } catch (_) {}
    }

    // 恢复 AI 设置
    const savedAI = localStorage.getItem('ai-settings')
    if (savedAI) {
      try { this.currentAISettings = JSON.parse(savedAI) } catch (_) {}
    }
    const savedAIMap = localStorage.getItem('ai-settings-map')
    if (savedAIMap) {
      try { this.chatAISettings = JSON.parse(savedAIMap) } catch (_) {}
    }

    // 恢复上次页面
    if (this.isLoggedIn) {
      const savedScreen = localStorage.getItem('current-screen')
      if (savedScreen && savedScreen !== 'login') {
        this.currentScreen = savedScreen
      } else {
        this.currentScreen = 'chatlist'
      }
    }

    // 连接 WebSocket
    await this.connectWebSocket()
  }
})

store.init()

export default store
