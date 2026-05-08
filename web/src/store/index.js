import { reactive } from 'vue'

const store = reactive({
  currentScreen: 'login',
  chatTitle: '张三',
  groupTitle: '产品讨论组',
  isLoggedIn: false,

  switchScreen(id) {
    this.currentScreen = id
    localStorage.setItem('wx-current-screen', id)
  },

  openChat(type, name) {
    if (type === 'group') {
      this.groupTitle = name
      this.switchScreen('group-chat')
    } else {
      this.chatTitle = name
      this.switchScreen('single-chat')
    }
  },

  login() {
    this.isLoggedIn = true
    this.switchScreen('chatlist')
  },

  init() {
    const saved = localStorage.getItem('wx-current-screen')
    if (saved && saved !== 'login') {
      this.currentScreen = saved
      this.isLoggedIn = true
    }
  }
})

store.init()

export default store
