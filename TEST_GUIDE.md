# 测试指南

## 前置准备

### 1. 启动MySQL服务
```bash
sudo systemctl start mysql
# 或
sudo service mysql start
```

### 2. 创建数据库（如果还没有）
```sql
CREATE DATABASE IF NOT EXISTS ai_chat_system DEFAULT CHARSET utf8mb4;
```

### 3. 启动服务器
```bash
cd /home/lin/桌面/AI智能聊天互动系统
./ai_chat_server
```

服务器启动后应显示：
```
ChatService initialized successfully
VerifyService initialized successfully
FriendService initialized successfully
GroupService initialized successfully
AiService initialized successfully
Server started successfully!
```

---

## 测试方法

### 方法一：自动完整测试（推荐）

```bash
cd /home/lin/桌面/AI智能聊天互动系统
python3 test_client.py --full
```

这会自动测试所有功能模块。

### 方法二：交互式菜单测试

```bash
python3 test_client.py
```

然后根据菜单选择要测试的功能。

### 方法三：单个功能测试

```bash
python3 test_client.py register    # 测试注册
python3 test_client.py captcha     # 测试验证码
python3 test_client.py login       # 测试登录
python3 test_client.py friend_add  # 测试添加好友
python3 test_client.py friend_list # 测试好友列表
python3 test_client.py group_create# 测试创建群聊
python3 test_client.py ai          # 测试AI对话
```

---

## 测试用例说明

### 1. 用户注册 (type=2)
- 发送：用户ID、用户名、密码
- 预期：返回成功，用户信息写入数据库

### 2. 获取验证码 (type=10)
- 发送：手机号
- 预期：返回captcha_token和captcha_image

### 3. 用户登录 (type=1)
- 发送：用户ID、密码、验证码
- 预期：验证成功返回用户信息，失败返回错误

### 4. 添加好友 (type=20)
- 发送：请求方用户ID、接收方用户ID、验证消息
- 预期：请求发送成功，对方收到通知

### 5. 同意好友 (type=21)
- 发送：请求方用户ID、接收方用户ID
- 预期：好友关系建立，双方可聊天

### 6. 创建群聊 (type=30)
- 发送：创建者用户ID、群名称
- 预期：返回群聊ID，创建者成为群主

### 7. 加群申请 (type=31)
- 发送：申请人用户ID、群聊ID、申请消息
- 预期：申请发送给群主

### 8. 私聊消息 (type=40)
- 发送：发送者ID、接收者ID、消息内容
- 预期：消息保存并推送给接收者

### 9. AI请求 (type=50)
- 发送：用户ID、问题内容
- 预期：收到AI回复（需要配置AI API）

---

## 数据库验证

测试后可以查询数据库验证数据是否正确：

```sql
-- 查看用户
SELECT * FROM ai_chat_system.user;

-- 查看好友关系
SELECT * FROM ai_chat_system.friend_relation;

-- 查看好友请求
SELECT * FROM ai_chat_system.friend_request;

-- 查看群聊
SELECT * FROM ai_chat_system.group_chat;

-- 查看群成员
SELECT * FROM ai_chat_system.group_member;

-- 查看验证码
SELECT * FROM ai_chat_system.verification_code;

-- 查看聊天记录
SELECT * FROM ai_chat_system.chat_record;
```

---

## 常见问题

### Q: 连接失败
A: 确保服务器已启动，端口8080未被占用

### Q: 数据库错误
A: 检查config.ini中的数据库配置是否正确

### Q: AI功能不工作
A: 需要在config.ini中配置AI API的URL和密钥

---

## 测试消息格式

所有消息采用JSON格式：

```json
{
    "type": 消息类型编号,
    "from_user_id": "发送方用户ID",
    "to_user_id": "接收方用户ID或群聊ID",
    "content": "消息内容",
    "extra": "额外参数(JSON字符串)",
    "timestamp": "时间戳"
}
```

响应格式：
```json
{
    "type": 响应类型,
    "code": 状态码(0成功, -1失败),
    "msg": "提示信息",
    "data": "数据内容",
    "timestamp": "时间戳"
}
```
