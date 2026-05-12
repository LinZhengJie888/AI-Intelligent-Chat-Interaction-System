# AI 智能聊天互动系统 - 前端

基于 Vue 3 + Vite 构建的现代化聊天应用前端，支持单聊、群聊和 AI 智能对话。

## 技术栈

- **框架**: Vue 3 (Composition API + `<script setup>`)
- **构建工具**: Vite 8.x
- **状态管理**: 响应式 Store (自定义)
- **通信**: WebSocket 实时通信
- **样式**: CSS 变量 + 微信风格设计系统

## 项目结构

```
frontend/
├── public/                  # 静态资源
├── src/
│   ├── api/                 # API 通信层
│   │   └── websocket.js     # WebSocket 客户端
│   ├── components/          # 通用组件
│   │   ├── AIPet.vue        # AI 宠物组件
│   │   ├── BottomNav.vue    # 底部导航栏
│   │   ├── HelloWorld.vue   # 示例组件
│   │   └── Sidebar.vue      # 侧边栏
│   ├── screens/             # 页面级组件
│   │   ├── AIChatScreen.vue      # AI 聊天界面
│   │   ├── ChatListScreen.vue    # 聊天列表
│   │   ├── ContactsScreen.vue    # 联系人列表
│   │   ├── GroupChatScreen.vue   # 群聊界面
│   │   ├── GroupsScreen.vue      # 群组列表
│   │   ├── LoginScreen.vue       # 登录界面
│   │   ├── ProfileScreen.vue     # 个人资料
│   │   └── SingleChatScreen.vue  # 单聊界面
│   ├── store/               # 状态管理
│   │   └── index.js         # 响应式 Store
│   ├── App.vue              # 根组件
│   └── main.js              # 应用入口
├── index.html               # HTML 入口
├── package.json             # 依赖配置
├── vite.config.js           # Vite 配置
└── README.md                # 项目文档
```

## 安装和运行

### 前置要求

- Node.js >= 18.x
- npm >= 9.x

### 安装依赖

```bash
cd frontend
npm install
```

### 开发模式

```bash
npm run dev
```

启动后访问: http://localhost:5173

### 生产构建

```bash
npm run build
```

### 预览构建结果

```bash
npm run preview
```

## 功能说明

### 核心功能

1. **用户认证**
   - 登录/登出
   - 用户信息管理

2. **聊天功能**
   - 单聊: 一对一实时聊天
   - 群聊: 多人群组聊天
   - AI 聊天: 与 AI 助手对话

3. **联系人管理**
   - 联系人列表
   - 群组管理

4. **AI 特色功能**
   - AI 宠物互动
   - 智能回复建议

### 界面组件

- **Sidebar**: 左侧导航，显示用户信息和功能入口
- **BottomNav**: 底部标签栏，快速切换主要功能
- **AIPet**: 浮动 AI 宠物，提供快捷交互
- **TopBar**: 顶部标题栏，包含返回和操作按钮

## 开发指南

### 添加新页面

1. 在 `src/screens/` 创建新的 `.vue` 文件
2. 在 `App.vue` 中导入并添加条件渲染
3. 在 `store/index.js` 中添加对应的屏幕状态

### 状态管理

使用自定义的响应式 Store (`src/store/index.js`)：

```javascript
import store from './store'

// 访问状态
console.log(store.isLoggedIn)
console.log(store.currentScreen)

// 更新状态
store.setLoggedIn(true)
store.setCurrentScreen('chatlist')
```

### WebSocket 通信

通过 `src/api/websocket.js` 进行实时通信：

```javascript
import { connectWebSocket, sendMessage } from './api/websocket'

// 建立连接
connectWebSocket()

// 发送消息
sendMessage({
  type: 'chat',
  content: 'Hello!',
  to: 'user123'
})
```

## 配置说明

### Vite 配置 (vite.config.js)

- **开发服务器端口**: 5173
- **主机监听**: 0.0.0.0 (允许外部访问)
- **代理配置**:
  - `/ws` -> `ws://localhost:8081` (WebSocket 服务)
  - `/static` -> `http://localhost:8080` (静态资源)

### 环境变量

创建 `.env` 文件配置环境变量：

```env
VITE_API_BASE_URL=http://localhost:8080
VITE_WS_URL=ws://localhost:8081
```

## 设计系统

### CSS 变量

项目使用微信风格的设计系统，主要变量：

```css
--wx-green: #07C160;        /* 主色调 */
--wx-bg: #EDEDED;           /* 背景色 */
--wx-text: #111111;         /* 主文本 */
--wx-text-secondary: #999;  /* 次要文本 */
--wx-border: #E6E6E6;       /* 边框色 */
```

### 响应式设计

- 移动端优先设计
- 断点: 768px
- 消息气泡最大宽度: 80% (移动端 90%)

## 相关链接

- [Vue 3 文档](https://v3.vuejs.org/)
- [Vite 文档](https://vitejs.dev/)
- [项目架构文档](../docs/ARCHITECTURE.md)
- [测试指南](../TEST_GUIDE.md)

## 常见问题

### 1. WebSocket 连接失败

确保后端 WebSocket 服务运行在 8081 端口，或修改 `vite.config.js` 中的代理配置。

### 2. 样式问题

检查浏览器兼容性，项目使用 CSS 变量和现代 CSS 特性。

### 3. 构建失败

清理 `node_modules` 并重新安装：

```bash
rm -rf node_modules package-lock.json
npm install
```
