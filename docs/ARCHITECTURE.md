# AI智能聊天互动系统 - 架构文档

## 1. 系统概述

AI智能聊天互动系统是一个基于 WebSocket 的实时聊天应用，支持：
- 一对一私聊
- 群聊
- AI 智能对话
- 好友管理
- 群组管理

## 2. 技术栈

### 后端
- **语言**: C++11
- **构建系统**: Makefile
- **数据库**: MySQL 8.0
- **缓存**: Redis (可选)
- **HTTP 客户端**: libcurl
- **JSON 解析**: 自研 JsonUtil 工具类

### 前端
- **框架**: Vue 3
- **构建工具**: Vite
- **状态管理**: Vue Reactive
- **WebSocket**: 原生 WebSocket API

### 代理层
- **WebSocket 代理**: Node.js + ws 库
- **静态文件服务**: Node.js

## 3. 系统架构

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   前端      │────▶│  WebSocket  │────▶│   后端      │
│  (Vue 3)    │     │    代理     │     │   (C++)     │
│  Port:5173  │     │  Port:8081  │     │  Port:8080  │
└─────────────┘     └─────────────┘     └─────────────┘
                           │                   │
                           ▼                   ▼
                    ┌─────────────┐     ┌─────────────┐
                    │ 静态文件服务 │     │   MySQL     │
                    │  Port:8082  │     │  Port:3306  │
                    └─────────────┘     └─────────────┘
```

## 4. 核心模块

### 4.1 后端模块

#### ChatService (消息路由中心)
- 消息分发和路由
- 会话管理
- 用户认证
- 好友/群组操作处理

#### AiService (AI 服务)
- AI API 调用（支持流式 SSE 输出和非流式两种模式）
- 多轮对话上下文管理（从 chat_record 查询历史，Token 预算自动截断）
- 流式推送（type 53/54/55 消息，逐字推送增量内容）
- 请求队列管理（2个工作线程消费任务队列）
- 响应缓存（内存 + Redis + MySQL 三级缓存）
- AI 设置管理（聊天级别，支持昵称/语气/优先级）
- 回复分段（按 UTF-8 字符计数，自然语句边界切分）
- 流式失败自动回退非流式调用

#### FriendService (好友服务)
- 好友请求处理
- 好友关系管理

#### GroupService (群组服务)
- 群聊创建/管理
- 群成员管理
- 群消息处理

#### VerifyService (验证服务)
- 验证码生成和验证
- 用户注册验证

### 4.2 数据模型

#### 用户表 (user)
```sql
CREATE TABLE user (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id VARCHAR(64) NOT NULL UNIQUE,  -- 业务ID
    username VARCHAR(64) NOT NULL,
    nickname VARCHAR(64),
    password VARCHAR(255) NOT NULL,
    phone VARCHAR(20),
    avatar_path VARCHAR(255),
    ai_nickname VARCHAR(64) DEFAULT 'AI助手',
    ai_tone TINYINT DEFAULT 0,
    ai_priority TINYINT DEFAULT 0,
    create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    update_time DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
```

#### 群聊表 (group_chat)
```sql
CREATE TABLE group_chat (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    group_id VARCHAR(64) NOT NULL UNIQUE,  -- 业务ID
    group_name VARCHAR(64) NOT NULL,
    creator_id BIGINT UNSIGNED NOT NULL,  -- 数值主键
    avatar_path VARCHAR(255),
    announcement TEXT,
    create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    update_time DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
```

#### 聊天记录表 (chat_record)
```sql
CREATE TABLE chat_record (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    sender_id BIGINT UNSIGNED NOT NULL,  -- 数值主键
    receiver_id BIGINT UNSIGNED,         -- 数值主键
    group_id BIGINT UNSIGNED,            -- 数值主键
    content TEXT NOT NULL,
    msg_type TINYINT DEFAULT 0,
    is_ai TINYINT DEFAULT 0,
    is_recalled TINYINT DEFAULT 0,
    is_read TINYINT DEFAULT 0,
    send_time DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

#### AI 设置表 (chat_ai_settings)
```sql
CREATE TABLE chat_ai_settings (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    chat_key VARCHAR(64) NOT NULL UNIQUE,
    nickname VARCHAR(32) DEFAULT 'AI助手',
    tone TINYINT DEFAULT 0,
    priority TINYINT DEFAULT 1,
    updated_by VARCHAR(32) DEFAULT '',
    update_time DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
```

## 5. 消息协议

### 消息格式
```json
{
    "type": 1,
    "from_user_id": "user123",
    "to_user_id": "user456",
    "content": "Hello",
    "extra": "{}",
    "timestamp": "2024-01-01 12:00:00"
}
```

### 消息类型
| 类型 | 说明 |
|------|------|
| 0 | 心跳 |
| 1 | 登录 |
| 3 | 登出 |
| 23 | 好友列表 |
| 25 | 好友请求 |
| 34 | 群消息 |
| 36 | 群列表 |
| 40 | 私聊消息 |
| 41 | 聊天记录 |
| 50 | AI 请求 |
| 51 | AI @召唤 |
| 52 | AI 设置 |
| 53 | AI 流式输出开始 |
| 54 | AI 流式输出增量 |
| 55 | AI 流式输出结束 |

## 6. ID 体系

### 双 ID 模型
- **业务 ID**: 字符串类型，用于前端和协议层
  - user_id: 如 "user123"
  - group_id: 如 "G1a2b3c4d"
- **数值主键**: BIGINT 类型，用于数据库关系层
  - user.id
  - group_chat.id

### 转换规则
- 前端协议使用字符串业务 ID
- Service 层负责转换：字符串 ID → 数值主键
- DAO 层只处理数值主键
- 响应时转换：数值主键 → 字符串业务 ID

## 7. AI 设置

### ChatKey 格式
- 私聊: `single:{userId}`
- 群聊: `group:{groupId}`
- 独立 AI: `ai:ai`

### 设置共享机制
- 群聊设置变更广播给所有群成员
- 私聊设置变更广播给聊天双方
- 独立 AI 设置只通知修改者

## 8. 部署说明

### 环境要求
- MySQL 8.0+
- Redis (可选)
- Node.js 16+
- g++ 支持 C++11

### 启动步骤

1. **初始化数据库**
```bash
mysql -u root -p < db/mysql/init.sql
```

2. **编译后端**
```bash
make clean && make
```

3. **启动后端**
```bash
./ai_chat_server
```

4. **启动代理**
```bash
node proxy-server.js
```

5. **启动前端**
```bash
cd frontend && npm install && npm run dev
```

### 访问地址
- 前端: http://localhost:5173
- WebSocket: ws://localhost:8081
- 静态资源: http://localhost:8082

## 9. 已知限制

1. **ID 体系**: 当前仍存在部分字符串 ID 直接入库的情况
2. **JSON 解析**: 自研实现，复杂嵌套场景可能有边界问题
3. **并发处理**: AI 请求使用固定大小线程池，高并发下可能排队
4. **持久化**: Redis 缓存为可选，不影响核心功能

## 10. 后续优化方向

1. 完全统一 ID 体系
2. 引入专业 JSON 库 (如 nlohmann/json)
3. 实现完整的会话管理
4. 添加消息已读回执
5. 支持文件和图片消息
6. 实现消息漫游
