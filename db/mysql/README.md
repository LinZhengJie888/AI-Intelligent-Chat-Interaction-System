# AI智能聊天互动系统 - MySQL数据库说明

## 目录说明

本目录包含AI智能聊天互动系统的MySQL数据库初始化脚本：

| 文件 | 说明 |
|------|------|
| `init.sql` | 完整的数据库初始化脚本（含表结构、扩展表、外键约束） |

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

## 快速开始

### 1. 创建数据库
```bash
// 创建数据库，使用utf8mb4字符集支持完整Unicode和emoji
mysql -u root -p -e "CREATE DATABASE IF NOT EXISTS ai_chat_system DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
```

### 2. 导入初始化脚本
```bash
cd /home/lin/桌面/AI智能聊天互动系统/db/mysql
mysql -u root -p ai_chat_system < init.sql
```

### 3. 验证导入
```bash
// 查看数据库中的所有表
mysql -u root -p ai_chat_system -e "SHOW TABLES;"
```

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

- MySQL 5.7 或更高版本
- InnoDB 存储引擎

## 回滚操作

如需删除数据库：
```bash
// 删除ai_chat_system数据库（回滚操作，会永久删除所有数据）
mysql -u root -p -e "DROP DATABASE IF EXISTS ai_chat_system;"
```
