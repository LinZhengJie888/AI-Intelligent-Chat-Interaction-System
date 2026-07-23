/**
 * WebSocket 通信封装
 *
 * 通过 proxy-server.js (ws://host:8081) 与后端 Reactor TCP 服务器通信。
 * 协议格式：[4字节大端序长度头] + [JSON字符串]
 * 与后端 Buffer 类 (sep=1) 完全一致。
 */

const MessageType = {
  LOGIN: 1,
  REGISTER: 2,
  LOGOUT: 3,

  GET_CAPTCHA: 10,

  FRIEND_ADD: 20,
  FRIEND_AGREE: 21,
  FRIEND_REJECT: 22,
  FRIEND_LIST: 23,
  FRIEND_DELETE: 24,
  FRIEND_REQUEST_LIST: 25,
  FRIEND_SET_REMARK: 26,

  GROUP_CREATE: 30,
  GROUP_JOIN: 31,
  GROUP_AGREE: 32,
  GROUP_REJECT: 33,
  GROUP_MESSAGE: 34,
  GROUP_MEMBERS: 35,
  GROUP_LIST: 36,
  GROUP_MODIFY_NAME: 37,
  GROUP_LEAVE: 39,

  CHAT_PRIVATE: 40,
  CHAT_HISTORY: 41,
  MESSAGE_RECALL: 42,
  MESSAGE_READ: 43,

  AI_REQUEST: 50,
  AI_AT: 51,
  AI_SETTING: 52,
  AI_STREAM_START: 53,
  AI_STREAM_CHUNK: 54,
  AI_STREAM_END: 55,

  UPDATE_USERNAME: 62,
  GROUP_KICK: 63,
  GROUP_REQUEST_LIST: 64,
  GROUP_DISSOLVE: 65,

  UPLOAD_AVATAR: 70,
  UPLOAD_GROUP_AVATAR: 71,

  HEARTBEAT: 0,
  RESPONSE_OK: 100,
  RESPONSE_ERROR: 101
}

class WebSocketClient {
  constructor() {
    this.ws = null
    this.url = ''
    this.isConnected = false
    this.reconnectAttempts = 0
    this.maxReconnectAttempts = 5
    this.reconnectDelay = 3000
    this.handlers = new Map()
    this.pendingMessages = []
    this.userId = null
    this.receiveBuffer = new Uint8Array(0)
    this.heartbeatTimer = null
    this.heartbeatInterval = 5000
  }

  connect(url) {
    return new Promise((resolve, reject) => {
      this.url = url
      this.maxReconnectAttempts = 5
      this.ws = new WebSocket(url)
      this.ws.binaryType = 'arraybuffer'

      this.ws.onopen = () => {
        console.log('[WS] 连接成功:', url)
        this.isConnected = true
        this.reconnectAttempts = 0
        this.receiveBuffer = new Uint8Array(0)

        this._startHeartbeat()

        while (this.pendingMessages.length > 0) {
          const msg = this.pendingMessages.shift()
          this._sendRaw(msg)
        }
        resolve(true)
      }

      this.ws.onmessage = (event) => {
        this._onMessage(event.data)
      }

      this.ws.onclose = () => {
        console.log('[WS] 连接关闭')
        this.isConnected = false
        this._stopHeartbeat()
        this._reconnect()
      }

      this.ws.onerror = (err) => {
        console.error('[WS] 连接错误:', err)
        reject(err)
      }
    })
  }

  disconnect() {
    this.maxReconnectAttempts = 0
    this._stopHeartbeat()
    if (this.ws) {
      this.ws.close()
      this.ws = null
    }
    this.isConnected = false
    this.userId = null
  }

  _startHeartbeat() {
    this._stopHeartbeat()
    this.heartbeatTimer = setInterval(() => {
      if (this.isConnected) {
        this.send({ type: MessageType.HEARTBEAT, content: 'ping' })
      }
    }, this.heartbeatInterval)
  }

  _stopHeartbeat() {
    if (this.heartbeatTimer) {
      clearInterval(this.heartbeatTimer)
      this.heartbeatTimer = null
    }
  }

  _reconnect() {
    if (this.reconnectAttempts >= this.maxReconnectAttempts) {
      this._emit('reconnect_failed')
      return
    }
    this.reconnectAttempts++
    setTimeout(() => {
      if (!this.isConnected && this.url) {
        this.connect(this.url).catch(() => {})
      }
    }, this.reconnectDelay)
  }

  /**
   * 收到 WebSocket 数据
   * 代理转发的是原始 TCP 数据，包含 4字节长度头 + JSON
   */
  _onMessage(data) {
    const chunk = new Uint8Array(data)
    const merged = new Uint8Array(this.receiveBuffer.length + chunk.length)
    merged.set(this.receiveBuffer)
    merged.set(chunk, this.receiveBuffer.length)
    this.receiveBuffer = merged

    // 循环解析所有完整报文
    while (this.receiveBuffer.length >= 4) {
      const msgLen = new DataView(this.receiveBuffer.buffer).getUint32(0, false) // big-endian

      if (this.receiveBuffer.length < 4 + msgLen) break

      const jsonBytes = this.receiveBuffer.slice(4, 4 + msgLen)
      const jsonStr = new TextDecoder().decode(jsonBytes)
      this.receiveBuffer = this.receiveBuffer.slice(4 + msgLen)

      try {
        const msg = JSON.parse(jsonStr)
        if (msg.type !== MessageType.HEARTBEAT) {
          this._dispatch(msg)
        }
      } catch (e) {
        console.error('[WS] JSON 解析失败:', e, jsonStr)
      }
    }
  }

  /**
   * 发送消息（自动加 4字节长度头）
   */
  send(message) {
    const jsonStr = JSON.stringify(message)
    const jsonBytes = new TextEncoder().encode(jsonStr)

    const packet = new Uint8Array(4 + jsonBytes.length)
    const view = new DataView(packet.buffer)
    view.setUint32(0, jsonBytes.length, false) // big-endian
    packet.set(jsonBytes, 4)

    if (!this.isConnected) {
      this.pendingMessages.push(packet)
      return false
    }
    return this._sendRaw(packet)
  }

  _sendRaw(packet) {
    try {
      this.ws.send(packet)
      return true
    } catch (e) {
      console.error('[WS] 发送失败:', e)
      return false
    }
  }

  /**
   * 分发消息给注册的处理器
   */
  _dispatch(msg) {
    const type = msg.type
    const list = this.handlers.get(type)
    if (list) list.forEach(fn => fn(msg))
    const all = this.handlers.get('*')
    if (all) all.forEach(fn => fn(msg))
  }

  on(type, fn) {
    if (!this.handlers.has(type)) this.handlers.set(type, [])
    this.handlers.get(type).push(fn)
  }

  off(type, fn) {
    const list = this.handlers.get(type)
    if (list) {
      const i = list.indexOf(fn)
      if (i > -1) list.splice(i, 1)
    }
  }

  _emit(event, data) {
    const list = this.handlers.get(event)
    if (list) list.forEach(fn => fn(data))
  }

  setUserId(userId) {
    this.userId = userId
  }
}

const wsClient = new WebSocketClient()

export { wsClient, MessageType }
export default wsClient
