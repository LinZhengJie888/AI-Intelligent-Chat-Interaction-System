# AI智能聊天互动系统

## 一、项目简介

本项目采用标准前后端分离架构，按云服务器部署标准开发（验证演示时本地或局域网启动即可），核心实现实时聊天功能，支持群聊与好友一对一对话双模式，并集成AI聊天助手（API调用模式，不本地部署）。

**项目用途：**
- 个人学习实践
- C++后端开发工程师求职简历呈现

**核心特点：**
- 自研 Reactor 网络框架（Epoll + 线程池）
- 前后端分离，WebSocket 代理转发 + HTTP 静态服务
- 多厂商 AI API 兼容接入
- 微信浅色风格 UI

**项目规模：** 约 15,500 行代码（C++ 11,289 行 + Vue/JS 4,089 行）

---

## 二、项目结构

```
AI智能聊天互动系统/
├── backend/                    # C++ 后端
│   ├── include/                # 头文件
│   │   ├── reactor/            # 自研 Reactor 网络框架
│   │   │   ├── TcpServer.h     # TCP 服务器
│   │   │   ├── EventLoop.h     # 事件循环
│   │   │   ├── Connection.h    # 连接管理
│   │   │   ├── Buffer.h        # 缓冲区（4字节长度头+JSON）
│   │   │   ├── Channel.h       # 通道
│   │   │   ├── Epoll.h         # Epoll 封装
│   │   │   ├── Socket.h        # Socket 封装
│   │   │   ├── Acceptor.h      # 连接接受器
│   │   │   ├── ThreadPool.h    # 线程池
│   │   │   └── Timestamp.h     # 时间戳
│   │   ├── model/              # 数据模型
│   │   │   ├── User.h          # 用户模型
│   │   │   ├── UserDAO.h       # 用户数据访问
│   │   │   ├── ChatRecordDAO.h # 聊天记录DAO
│   │   │   ├── ChatAISettingDAO.h # AI设置DAO
│   │   │   └── GroupChatDAO.h  # 群聊DAO
│   │   ├── module/             # 业务模块
│   │   │   ├── ChatService.h   # 消息路由中心
│   │   │   ├── Config.h        # 配置管理
│   │   │   ├── Database.h      # 数据库连接
│   │   │   ├── ai_api/         # AI API 调用模块
│   │   │   │   └── AiService.h
│   │   │   ├── verify/         # 动态验证码模块
│   │   │   │   └── VerifyService.h
│   │   │   ├── friend/         # 好友与群组模块
│   │   │   │   ├── FriendService.h
│   │   │   │   └── GroupService.h
│   │   │   └── redis/          # Redis 客户端
│   │   │       └── RedisClient.h
│   │   └── common/             # 公共工具
│   │       ├── Util.h          # 工具函数（MD5等）
│   │       └── JsonUtil.h      # JSON解析工具
│   ├── src/                    # 源文件（与 include 对应）
│   │   ├── main.cpp            # 后端入口
│   │   ├── reactor/            # Reactor 框架实现
│   │   ├── model/              # 数据模型实现
│   │   ├── module/             # 业务模块实现
│   │   └── common/             # 工具实现
│   └── static/                 # 静态资源（头像等）
│       └── avatars/            # 用户/群聊头像存储
│
├── frontend/                   # Vue3 前端
│   ├── index.html              # 入口 HTML
│   ├── package.json            # 依赖配置
│   ├── vite.config.js          # Vite 配置（含 WebSocket 代理）
│   └── src/
│       ├── main.js             # Vue 应用入口
│       ├── App.vue             # 根组件（页面切换）
│       ├── api/
│       │   └── websocket.js    # WebSocket 通信封装（4字节长度头+JSON）
│       ├── store/
│       │   └── index.js        # 全局状态管理（约1200行）
│       ├── components/
│       │   ├── Sidebar.vue     # 左侧导航栏
│       │   ├── BottomNav.vue   # 底部导航栏（移动端）
│       │   ├── AIPet.vue       # AI 浮窗助手
│       │   └── ChatAISettingsModal.vue # AI设置弹窗
│       └── screens/
│           ├── LoginScreen.vue       # 登录/注册页
│           ├── ChatListScreen.vue    # 聊天列表页
│           ├── SingleChatScreen.vue  # 单聊页（支持图片发送、删除好友）
│           ├── GroupChatScreen.vue   # 群聊页（支持群管理）
│           ├── AIChatScreen.vue      # AI 对话页
│           ├── ContactsScreen.vue    # 通讯录页（好友请求审核）
│           ├── GroupsScreen.vue      # 群聊列表页（加群申请管理）
│           ├── ProfileScreen.vue     # 个人资料页（头像上传）
│           └── AISettingsScreen.vue  # AI设置页
│
├── db/                         # 数据库
│   └── mysql/
│       └── init.sql            # MySQL 初始化脚本
│
├── docs/                       # 文档
│   ├── README.md               # 本文件
│   ├── PRD.md                  # 产品需求文档
│   ├── ARCHITECTURE.md         # 架构设计文档
│   └── FRONTEND_DESIGN.md      # 前端设计方案
│
├── config.ini                  # 运行配置文件
├── makefile                    # 后端编译脚本
├── proxy-server.js             # WebSocket 代理 + HTTP 静态服务器
└── package.json                # 代理服务器依赖
```

---

## 三、技术栈

### 后端

| 技术 | 说明 |
|------|------|
| C++11/14 | 核心语言 |
| 自研 Reactor 框架 | Epoll + 线程池，无第三方依赖 |
| MySQL 5.7+ | 主数据库，存储用户、消息、好友关系等 |
| Redis 6.0+ | 缓存层，缓存高频数据和验证码 |
| libcurl | HTTP 请求，用于调用 AI API |
| OpenSSL | MD5 密码加密 |

### 前端

| 技术 | 说明 |
|------|------|
| Vue 3 | Composition API + `<script setup>` |
| Vite 8 | 构建工具，支持热更新 |
| WebSocket | 通过代理与后端 TCP 服务器通信 |
| 微信浅色风格 | 绿色主色调，白色卡片，灰色背景 |

### 通信架构

```
浏览器前端 ──WebSocket(8081)──► proxy-server.js ──TCP(8080)──► C++后端
                                    │
                                    └── HTTP(8082) ──► 静态文件（头像等）
```

- **前端**：发送/接收 4字节大端序长度头 + JSON
- **代理**：WebSocket → TCP 透明转发，不解析内容
- **HTTP 服务**：提供静态文件访问（头像等），端口 8082
- **后端**：Buffer 类 (sep=1) 解析报文

### 报文格式

```
┌──────────────┬───────────────────┐
│ 4字节长度头    │ JSON 字符串        │
│ (大端序)      │ (UTF-8)           │
└──────────────┴───────────────────┘
```

### 消息字段

```json
{
  "type": 1,
  "from_user_id": "user001",
  "to_user_id": "user002",
  "content": "消息内容",
  "extra": "{}",
  "timestamp": "1234567890"
}
```

---

## 四、核心功能

### 4.1 用户系统

| 功能 | 说明 |
|------|------|
| 注册 | 用户名 + 用户ID + 密码 + 手机号（选填） |
| 登录 | 用户ID + 密码 + 动态验证码 |
| 验证码 | 6位数字+字母，图像形式，5分钟有效，1分钟冷却 |
| 密码加密 | MD5 加密存储 |
| 头像上传 | 支持 jpg/png，Base64 传输，服务器存储 |

### 4.2 聊天功能

| 功能 | 说明 |
|------|------|
| 单聊 | 一对一私聊，消息实时推送 |
| 群聊 | 多人群聊，支持群成员管理 |
| AI 对话 | 按键调用或 @AI 召唤 |
| 图片发送 | 支持发送图片消息 |
| 消息存储 | MySQL 持久化，Redis 缓存热点 |
| 聊天记录 | 页面刷新后自动加载历史记录 |

### 4.3 好友系统

| 功能 | 说明 |
|------|------|
| 添加好友 | 输入用户ID，附验证消息 |
| 请求审核 | 对方同意/拒绝，拒绝有冷却期 |
| 好友列表 | 显示好友头像、用户名 |
| 好友请求通知 | 实时推送好友请求 |
| 删除好友 | 支持删除好友，对方收到通知 |

### 4.4 群聊系统

| 功能 | 说明 |
|------|------|
| 创建群聊 | 设置群名，生成群聊ID |
| 加群申请 | 通过群聊ID申请，群主审核 |
| 群成员列表 | 显示成员头像、用户名、角色标签 |
| 群名修改 | 群主/管理员可修改 |
| 踢出成员 | 群主可踢出群成员 |
| 退群 | 群成员可退出群聊 |
| 解散群聊 | 群主可解散群聊 |

### 4.5 AI 助手

| 功能 | 说明 |
|------|------|
| 调用方式 | 按键调用 / @AI 召唤 |
| 回复规范 | 单条≤50字，分条发送 |
| 个性化设置 | 昵称、语气、响应优先级（按聊天独立设置） |
| 多厂商兼容 | 配置 API 地址和密钥即可切换 |
| 聊天记录 | 用户提问和AI回复都保存到数据库 |

---

## 五、消息类型定义

后端 `ChatService.h` 中定义的消息类型枚举（前后端完全对齐）：

```cpp
enum class MessageType {
    // 用户相关
    LOGIN = 1,              // 登录
    REGISTER = 2,           // 注册
    LOGOUT = 3,             // 登出

    // 验证码
    GET_CAPTCHA = 10,       // 获取验证码

    // 好友
    FRIEND_ADD = 20,        // 添加好友
    FRIEND_AGREE = 21,      // 同意好友
    FRIEND_REJECT = 22,     // 拒绝好友
    FRIEND_LIST = 23,       // 好友列表
    FRIEND_DELETE = 24,     // 删除好友
    FRIEND_REQUEST_LIST = 25, // 好友请求列表
    FRIEND_SET_REMARK = 26, // 设置好友备注

    // 群聊
    GROUP_CREATE = 30,      // 创建群聊
    GROUP_JOIN = 31,        // 加群申请
    GROUP_AGREE = 32,       // 同意加群
    GROUP_REJECT = 33,      // 拒绝加群
    GROUP_MESSAGE = 34,     // 群聊消息
    GROUP_MEMBERS = 35,     // 群成员列表
    GROUP_LIST = 36,        // 群列表
    GROUP_MODIFY_NAME = 37, // 修改群名
    GROUP_LEAVE = 39,       // 退出群聊

    // 聊天
    CHAT_PRIVATE = 40,      // 私聊消息
    CHAT_HISTORY = 41,      // 聊天记录
    MESSAGE_RECALL = 42,    // 消息撤回
    MESSAGE_READ = 43,      // 消息已读

    // AI
    AI_REQUEST = 50,        // AI 请求
    AI_AT = 51,             // AI @召唤
    AI_SETTING = 52,        // AI 设置

    // 其他
    UPDATE_USERNAME = 62,   // 修改用户名
    GROUP_KICK = 63,        // 踢出群成员
    GROUP_REQUEST_LIST = 64,// 加群请求列表
    GROUP_DISSOLVE = 65,    // 解散群聊

    // 头像
    UPLOAD_AVATAR = 70,     // 上传用户头像
    UPLOAD_GROUP_AVATAR = 71, // 上传群聊头像

    // 响应
    RESPONSE_OK = 100,      // 成功响应
    RESPONSE_ERROR = 101    // 错误响应
};
```

---

## 六、配置文件说明

`config.ini` 位于项目根目录：

```ini
[database]
host = 127.0.0.1       # 数据库地址
port = 3306            # 数据库端口
user = root            # 数据库用户
password = 123456      # 数据库密码
dbname = ai_chat_system # 数据库名

[ai]
api_url = https://...   # AI API 地址
api_key = xxx           # AI API 密钥
model = mimo-v2.5-pro   # 模型名称
default_nickname = AI助手
default_tone = 0        # 默认语气
default_priority = 0    # 默认优先级

[server]
port = 8080             # TCP 服务器端口
thread_pool_size = 4    # 线程池大小
avatar_storage_path = ./backend/static/avatars

[redis]
host = 127.0.0.1       # Redis 地址
port = 6379            # Redis 端口
```

---

## 七、如何启动

### 7.1 环境要求

| 依赖 | 版本 | 说明 |
|------|------|------|
| g++ | 支持 C++11 | 后端编译 |
| MySQL | 5.7+ | 主数据库 |
| Redis | 6.0+ | 缓存 |
| Node.js | 18+ | 前端运行、代理服务器 |
| libcurl | - | HTTP 请求 |
| libmysqlclient | - | MySQL 客户端 |
| libhiredis | - | Redis 客户端 |
| OpenSSL | - | MD5 加密 |

Ubuntu/Debian 安装依赖：

```bash
sudo apt update
sudo apt install g++ make libmysqlclient-dev libcurl4-openssl-dev libhiredis-dev libssl-dev redis-server
```

### 7.2 初始化数据库

```bash
# 启动 MySQL 服务
sudo systemctl start mysql

# 创建数据库并执行初始化脚本
mysql -u root -p < db/mysql/init.sql
```

### 7.3 启动 Redis

```bash
sudo systemctl start redis
```

### 7.4 编译并启动后端

```bash
# 编译
make clean && make

# 启动（默认监听 8080 端口）
./ai_chat_server
```

启动成功会输出：
```
Starting AI Chat Server on 127.0.0.1:8080...
Server started successfully!
Waiting for connections...
```

### 7.5 启动代理服务器

```bash
# 安装依赖（首次）
npm install

# 启动代理（包含 WebSocket 代理和 HTTP 静态服务）
node proxy-server.js
```

启动成功会输出：
```
HTTP static server running on http://localhost:8082
Serving static files from: /path/to/backend/static
WebSocket proxy server running on ws://localhost:8081
Forwarding to 127.0.0.1:8080
```

### 7.6 启动前端

```bash
cd frontend

# 安装依赖（首次）
npm install

# 启动开发服务器
npm run dev
```

启动成功会输出：
```
VITE v8.x.x  ready in xxx ms

➜  Local:   http://localhost:5173/
```

### 7.7 访问应用

在浏览器打开 `http://localhost:5173/`，进入登录/注册页面。

局域网内其他设备可通过 `http://<你的IP>:5173/` 访问。

---

## 八、服务端口说明

| 端口 | 服务 | 协议 | 说明 |
|------|------|------|------|
| 8080 | C++ 后端 | TCP | 主业务服务器，自定义协议 |
| 8081 | WebSocket 代理 | WS | 前端连接入口，转发到 8080 |
| 8082 | HTTP 静态服务 | HTTP | 头像等静态文件访问 |
| 5173 | 前端开发服务器 | HTTP | Vite 热更新 |
| 3306 | MySQL | TCP | 数据库 |
| 6379 | Redis | TCP | 缓存 |

---

## 九、快速启动脚本

可以创建一个启动脚本 `start.sh`：

```bash
#!/bin/bash

echo "========== 启动 AI 智能聊天互动系统 =========="

# 1. 检查并启动 MySQL
echo "1. 检查 MySQL..."
if ! systemctl is-active --quiet mysql; then
    sudo systemctl start mysql
fi
echo "   MySQL 已运行"

# 2. 检查并启动 Redis
echo "2. 检查 Redis..."
if ! systemctl is-active --quiet redis; then
    sudo systemctl start redis
fi
echo "   Redis 已运行"

# 3. 编译后端（如果需要）
echo "3. 检查后端..."
if [ ! -f "./ai_chat_server" ]; then
    echo "   编译后端..."
    make clean && make
fi
echo "   后端已就绪"

# 4. 启动后端
echo "4. 启动后端服务器..."
./ai_chat_server > /tmp/backend.log 2>&1 &
BACKEND_PID=$!
sleep 1

# 5. 启动代理（含 HTTP 静态服务）
echo "5. 启动代理服务器..."
node proxy-server.js > /tmp/proxy.log 2>&1 &
PROXY_PID=$!
sleep 1

# 6. 启动前端
echo "6. 启动前端..."
cd frontend
npm run dev > /tmp/frontend.log 2>&1 &
FRONTEND_PID=$!
sleep 2

echo ""
echo "========== 系统已启动 =========="
echo "后端:      http://localhost:8080 (TCP)"
echo "WebSocket: ws://localhost:8081"
echo "HTTP静态:  http://localhost:8082"
echo "前端:      http://localhost:5173"
echo ""
echo "日志文件："
echo "  后端: /tmp/backend.log"
echo "  代理: /tmp/proxy.log"
echo "  前端: /tmp/frontend.log"
echo ""
echo "按 Ctrl+C 停止所有服务"

# 等待中断
trap "kill $BACKEND_PID $PROXY_PID $FRONTEND_PID 2>/dev/null; exit" INT TERM
wait
```

---

## 十、数据库表结构

### 核心表

| 表名 | 说明 | 核心字段 |
|------|------|----------|
| `user` | 用户表 | id, user_id, username, password, nickname, phone, avatar_path |
| `group_chat` | 群聊表 | id, group_id, group_name, creator_id, avatar_path |
| `group_member` | 群成员表 | id, group_id, user_id, role, join_time |
| `group_request` | 加群请求表 | id, group_id, from_user_id, request_msg, status |
| `chat_record` | 聊天记录 | id, sender_id, receiver_id, group_id, content, send_time, is_ai |
| `chat_ai_settings` | AI设置表 | id, chat_key, nickname, tone, priority, updated_by |
| `friend_request` | 好友请求 | id, from_user_id, to_user_id, request_msg, status, cooling_time |
| `friend_relation` | 好友关系 | id, user_id, friend_id, remark, create_time |
| `verification_code` | 验证码 | id, phone, code, expire_time, is_used |
| `ai_cache` | AI回复缓存 | id, question, response, create_time |
| `ai_log` | AI调用日志 | id, request_id, user_id, question, response, success, response_time |

---

## 十一、Redis 缓存设计

| Key 格式 | 说明 |
|----------|------|
| `user:user_id:<id>` | 用户信息缓存 |
| `ai:hot_question:<md5>` | AI 高频回复缓存 |
| `chat:pair:<a>:<b>` | 私聊最近消息 |
| `chat:group:<id>` | 群聊最近消息 |

---

## 十二、常见问题

### Q: 编译报错找不到头文件
A: 检查是否安装了 `libmysqlclient-dev`、`libcurl4-openssl-dev`、`libhiredis-dev`

### Q: 连接数据库失败
A: 检查 `config.ini` 中的数据库配置，确保 MySQL 服务已启动，密码正确

### Q: 前端无法连接后端
A: 确保 `proxy-server.js` 已启动（端口 8081），后端已启动（端口 8080）

### Q: 验证码不显示
A: 当前使用本地 SVG 生成验证码，无需后端支持

### Q: 头像上传后刷新看不到
A: 确保 `proxy-server.js` 已启动（包含 HTTP 静态服务，端口 8082），头像文件存储在 `backend/static/avatars/`

### Q: 好友请求列表为空
A: 确保后端已重新编译（`make clean && make`），好友请求使用字符串 user_id 存储

### Q: 群聊中看不到群成员列表
A: 确保后端已重新编译，群成员查询需要正确的 group_id 映射

### Q: AI回复显示乱码
A: 后端已修复 Unicode 代理对解码，确保重新编译后端

---

## 十三、相关文档

- [产品需求文档 (PRD.md)](./PRD.md)
- [架构设计文档 (ARCHITECTURE.md)](./ARCHITECTURE.md)
- [前端设计方案 (FRONTEND_DESIGN.md)](./FRONTEND_DESIGN.md)
