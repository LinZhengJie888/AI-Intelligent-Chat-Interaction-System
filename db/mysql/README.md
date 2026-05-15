# AI智能聊天互动系统 - MySQL数据库说明

## 目录说明

本目录包含AI智能聊天互动系统的MySQL数据库初始化脚本：

| 文件 | 说明 |
|------|------|
| `init.sql` | 完整的数据库初始化脚本（含表结构、扩展表、外键约束、初始数据） |
| `migrate_fix_ids.sql.deprecated` | **已废弃** - 旧版迁移脚本，请勿使用 |

> ⚠️ **注意**：`migrate_fix_ids.sql` 已废弃，该脚本会将 ID 字段改为 VARCHAR 类型，与当前设计冲突。

## 数据库表结构

### 核心表（8张）

| 表名 | 说明 |
|------|------|
| `user` | 用户信息表（用户ID、密码、AI配置、头像等） |
| `group_chat` | 群聊信息表（群ID、群名、公告等） |
| `group_member` | 群成员表（成员角色、禁言状态等） |
| `friend_relation` | 好友关系表 |
| `friend_request` | 好友请求表（含冷却期控制） |
| `group_request` | 加群请求表 |
| `chat_record` | 聊天记录表（文字/表情/图片、AI回复标记） |
| `verification_code` | 验证码表（含图像验证码token） |

### 扩展表（2张）

| 表名 | 说明 |
|------|------|
| `group_message_read` | 群消息已读表（逐人记录已读时间） |
| `attachment` | 附件表（消息附件元数据） |

### AI 相关表（2张，由 AiService 自动创建）

| 表名 | 说明 |
|------|------|
| `ai_cache` | AI 回复缓存表 |
| `ai_log` | AI 调用日志表 |
| `chat_ai_settings` | 聊天 AI 设置表（按 chatKey 存储） |

## ID 体系说明

### 双 ID 模型

| 类型 | 字段 | 说明 |
|------|------|------|
| 业务 ID | `user_id` (VARCHAR) | 前端使用，如 "user123" |
| 数值主键 | `id` (BIGINT) | 数据库关系层使用 |

### 转换规则

- 前端协议使用字符串业务 ID
- Service 层负责转换：字符串 ID → 数值主键
- DAO 层只处理数值主键
- 响应时转换：数值主键 → 字符串业务 ID

## 初始数据

`init.sql` 会自动插入以下初始数据：

### AI 系统用户

```sql
INSERT INTO user (user_id, username, nickname, password, phone, ai_nickname, ai_tone, ai_priority)
VALUES ('ai', 'AI系统', 'AI助手', '', '', 'AI助手', 0, 0);
```

此用户用于 AI 消息的 sender_id，确保外键约束有效。

## 快速开始

### 1. 创建数据库并导入

```bash
# 一键创建数据库并导入初始化脚本
mysql -u root -p < init.sql
```

或者分步执行：

```bash
# 创建数据库
mysql -u root -p -e "CREATE DATABASE IF NOT EXISTS ai_chat_system DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"

# 导入初始化脚本
mysql -u root -p ai_chat_system < init.sql
```

### 2. 验证导入

```bash
# 查看数据库中的所有表
mysql -u root -p ai_chat_system -e "SHOW TABLES;"

# 查看 AI 系统用户
mysql -u root -p ai_chat_system -e "SELECT * FROM user WHERE user_id='ai';"
```

## verification_code 表字段说明

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | BIGINT AUTO_INCREMENT | 主键 |
| `phone` | VARCHAR(20) NOT NULL | 手机号 |
| `code` | VARCHAR(16) NOT NULL | 验证码 |
| `captcha_token` | VARCHAR(64) | 验证码 token |
| `expire_time` | DATETIME NOT NULL | 过期时间 |
| `send_time` | DATETIME DEFAULT CURRENT_TIMESTAMP | 发送时间 |
| `is_used` | TINYINT DEFAULT 0 | 是否已使用 |

> ⚠️ **重要**：VerifyService 中的建表逻辑已与此表结构保持一致，请勿修改。

## 外键约束说明

`init.sql` 包含外键约束以保证数据完整性。如果在特定部署环境中不需要外键，可以：

**方式一：临时禁用外键检查**
```bash
mysql -u root -p ai_chat_system -e "SET FOREIGN_KEY_CHECKS=0; SOURCE init.sql; SET FOREIGN_KEY_CHECKS=1;"
```

**方式二：移除外键部分**
编辑 `init.sql`，删除最后 `ADD CONSTRAINT` 相关的代码段。

## 字符集说明

- 使用 `utf8mb4` 字符集，支持完整的Unicode和emoji表情
- 排序规则 `utf8mb4_unicode_ci`

## 系统要求

- MySQL 8.0 或更高版本
- InnoDB 存储引擎
- 支持 utf8mb4 字符集

## 优化记录

### 2024-01 优化

1. **移除运行时删表逻辑**：GroupService 不再在启动时删除群聊相关表
2. **统一 verification_code 表字段**：`token` → `captcha_token`，`create_time` → `send_time`
3. **添加 AI 系统用户**：确保 AI 消息的 sender_id 有效
4. **废弃 migrate_fix_ids.sql**：该脚本方向错误，已标记废弃
5. **统一 SQL 转义**：所有 DAO 和 Service 层使用 `escapeString()` 防止 SQL 注入

## 回滚操作

如需删除数据库：

```bash
# 删除 ai_chat_system 数据库（会永久删除所有数据）
mysql -u root -p -e "DROP DATABASE IF EXISTS ai_chat_system;"
```

如需清空表数据但保留结构：

```bash
# 清空所有表数据
mysql -u root -p ai_chat_system -e "
SET FOREIGN_KEY_CHECKS=0;
TRUNCATE TABLE chat_record;
TRUNCATE TABLE group_message_read;
TRUNCATE TABLE attachment;
TRUNCATE TABLE friend_relation;
TRUNCATE TABLE friend_request;
TRUNCATE TABLE group_request;
TRUNCATE TABLE group_member;
TRUNCATE TABLE group_chat;
TRUNCATE TABLE verification_code;
TRUNCATE TABLE ai_cache;
TRUNCATE TABLE ai_log;
TRUNCATE TABLE chat_ai_settings;
-- 保留 user 表中的 AI 系统用户
DELETE FROM user WHERE user_id != 'ai';
SET FOREIGN_KEY_CHECKS=1;
"
```

## 常见问题

### Q1: 导入时报外键错误
A: 确保按顺序导入，或临时禁用外键检查。

### Q2: AI 消息保存失败
A: 检查 user 表中是否存在 user_id='ai' 的记录。

### Q3: 验证码功能异常
A: 确认 verification_code 表字段是否与 VerifyService 一致（captcha_token、send_time）。

### Q4: 群聊历史记录为空
A: 确认 group_chat 表中的 id 字段是否正确，ChatService 使用此字段查询。
