# 当前分支整改施行方案

## 1. 目标

本方案基于 `docs/CODE_REVIEW_REPORT.md`，目标不是继续叠加功能，而是在尽量少打断现有演示能力的前提下，先完成底座收敛，使项目达到以下状态：

1. 数据库结构只有一套正式口径。
2. 前后端对用户、群聊、聊天记录的 ID 解释一致。
3. 消息解析、DAO 写入、AI 调用不再依赖高风险的临时实现。
4. 项目具备一条可重复执行的最小联调路径。

本次整改建议按 **P0 → P1 → P2** 三阶段实施，优先修复会直接导致功能异常和数据破坏的问题，再处理安全、稳定性和可维护性问题。

---

## 2. 总体实施原则

### 2.1 先统一口径，再修局部问题

当前很多问题看起来分散在不同文件里，但根因是同一个：

- schema 不统一
- ID 模型不统一
- 协议与存储层边界不统一

因此整改顺序必须是：

1. **统一 schema 与 ID 体系**
2. **重写关键业务链路中的映射和查询**
3. **再替换 JSON / SQL / AI 等实现细节**

如果反过来先做局部优化，例如单独修 SQL 拼接、单独优化 AI 线程，后续在 ID 或 schema 重构时还会再次返工。

### 2.2 保留前端协议字符串 ID，不改前端交互口径

当前前端已经稳定使用字符串业务 ID，例如：

- `frontend/src/store/index.js:119`
- `frontend/src/store/index.js:175`
- `frontend/src/store/index.js:259`

这部分已经形成现有 UI 与消息流的事实标准，因此整改时建议：

- **前端协议继续传 `user_id` / `group_id` 字符串**
- **后端 Service 层统一转换为数据库数值主键**
- **DAO 层只处理数值主键**
- **响应给前端时再转回字符串业务 ID**

这样改动范围最可控，也最符合当前 `db/mysql/init.sql` 已有的主键/外键结构。

### 2.3 禁止运行时修表/删表

从本次整改开始，禁止再通过服务启动去重塑数据库结构。所有结构调整都必须通过 SQL 脚本完成，并且以 `db/mysql/init.sql` + 明确 migration 为唯一来源。

---

## 3. 阶段划分

## 第一阶段：P0 底座收敛

### 阶段目标

解决当前最危险的四类问题：

1. schema 漂移
2. ID 混用
3. 验证码表不一致
4. 群聊/历史记录链路不稳定

完成这一阶段后，项目至少应达到：

- 新数据库可按统一脚本初始化
- 后端启动不会删表改表
- 注册/登录/好友/群聊/历史记录链路可在同一套 schema 下跑通

---

### 3.1 统一正式 schema

#### 目标

以 `db/mysql/init.sql` 作为唯一正式 schema 定义，后续所有业务代码都向它对齐。

#### 核心修改

**文件：**

- `db/mysql/init.sql`
- `db/mysql/migrate_fix_ids.sql`
- `backend/src/module/friend/GroupService.cpp`
- `backend/src/module/verify/VerifyService.cpp`

#### 具体动作

1. **确认群聊、好友、聊天记录关系表全部以数值主键关联**
   - 保留 `user.id`、`group_chat.id` 作为关系层引用字段
   - `user.user_id`、`group_chat.group_id` 仅作为业务唯一标识

2. **删除或废弃 `migrate_fix_ids.sql` 中将关系字段改成 VARCHAR 的方案**
   - 该脚本当前方向与正式 schema 冲突
   - 如果要保留，需重写为“历史数据修复脚本”，不能继续作为常规初始化步骤

3. **移除 `GroupService::fixTableSchemas()` 的 destructive 行为**
   - 不允许继续 `DROP TABLE IF EXISTS group_request/group_member/group_chat...`
   - 不允许在运行时修改 `chat_record.group_id` 类型

4. **统一验证码表字段**
   - 建议正式保留 `captcha_token`
   - 建议正式保留 `send_time`
   - `VerifyService` 所有建表、插入、查询、更新逻辑都向 `init.sql` 对齐

#### 建议结果

- `db/mysql/init.sql` 成为唯一可信的“从零初始化”入口
- migration 只负责补历史数据或字段迁移，不再定义另一套 schema
- 服务启动只检查依赖，不做结构重塑

---

### 3.2 统一 ID 模型

#### 目标

明确边界：

- 协议层：字符串 ID
- Service 层：负责映射
- DAO 层：只接受数值主键

#### 核心修改

**文件：**

- `backend/src/module/ChatService.cpp`
- `backend/src/module/friend/GroupService.cpp`
- `backend/src/module/ai_api/AiService.cpp`
- `backend/src/model/ChatRecordDAO.cpp`
- `backend/src/model/UserDAO.cpp`
- 相关 Friend / Group / Chat DAO 文件

#### 具体动作

1. **补统一 ID 解析入口**
   - 建议增加一层统一工具函数或 service helper，例如：
     - `resolveUserPrimaryKey(user_id)`
     - `resolveGroupPrimaryKey(group_id)`
   - 不要在各文件散落重复的 `getUserIdNum()` / `getGroupNumId()`

2. **规范 Service 与 DAO 边界**
   - `ChatService`、`GroupService`、`AiService` 接收字符串 ID
   - 进入持久化前统一查主键
   - DAO 入参统一改成数值 ID

3. **群聊链路全面改为“先映射再落库”**
   - `group_chat.creator_id` 写数值用户主键
   - `group_member.group_id` / `user_id` 写数值主键
   - `group_request.group_id` / `from_user_id` 写数值主键

4. **聊天记录链路对齐**
   - 私聊：`sender_id` / `receiver_id` 使用数值主键
   - 群聊：`group_id` 使用 `group_chat.id`
   - 对外返回历史记录时再把发送者、群聊业务 ID 补回响应

#### 重点代码修改点

- `backend/src/module/friend/GroupService.cpp:136-151`
  - 当前把字符串 `creator_id` / `group_id` 直接写入关系表
  - 需要改为写数值主键

- `backend/src/model/ChatRecordDAO.cpp:200-206`
  - 当前 `findByGroup()` 已经是数值查询，这是正确方向
  - 要做的是让调用方稳定拿到正确的数值 group id，而不是再出现 `findByGroup(0)`

- `backend/src/module/ChatService.cpp:459-463`
  - 登录等逻辑仍在手工从 `extra` 取字段，后续会进入统一 JSON 解析
  - 当前阶段先保证所有需要查库的 `from_user_id` / `to_user_id` / `group_id` 在入库前统一映射

---

### 3.3 修复群聊历史记录与群关系链路

#### 目标

让“创建群 → 加群 → 发群消息 → 拉群历史”走通，并且在同一套 ID 逻辑下工作。

#### 核心修改

**文件：**

- `backend/src/module/ChatService.cpp`
- `backend/src/module/friend/GroupService.cpp`
- `backend/src/model/ChatRecordDAO.cpp`
- 群成员/群请求相关 DAO

#### 具体动作

1. **修正群聊历史查询入口**
   - 不允许找不到 group 数值主键时回退到 `findByGroup(0)`
   - 应明确返回“群不存在”或“群 ID 非法”错误

2. **统一群成员关系查询**
   - 群成员查询、加群审批、群消息发送都要基于同一套数值主键
   - 业务层再映射出前端需要的字符串 `group_id` / `user_id`

3. **清理 group 表的重复/临时表设计**
   - 如果 `group_record` 只是群消息冗余表，需要重新确认是否必要
   - 避免 `chat_record` 和 `group_record` 同时承担群消息事实来源

#### 验证标准

- 创建群成功
- 创建者自动成为群主
- 用户申请加群成功
- 群主审批成功
- 群消息能入库
- 群历史记录能查回，且不再依赖异常兜底逻辑

---

### 3.4 修复验证码链路

#### 目标

让验证码功能只依赖一份表结构，并稳定支持生成/刷新/校验。

#### 核心修改

**文件：**

- `backend/src/module/verify/VerifyService.cpp`
- 相关 Verify DAO / helper（如果后续拆分）
- `db/mysql/init.sql`

#### 具体动作

1. 删除 `VerifyService::init()` 中与正式 schema 不一致的建表字段定义。
2. 所有 SQL 改为统一使用：
   - `captcha_token`
   - `send_time`
   - `expire_time`
   - `is_used`
3. 清理内存缓存与数据库字段名不一致的问题。
4. 对登录链路中的验证码校验参数做一次端到端确认：
   - `captcha`
   - `captcha_token`

#### 验证标准

- 获取验证码成功
- 刷新验证码成功
- 正确验证码通过
- 过期或已使用验证码失败

---

## 第二阶段：P1 稳定性与安全收敛

### 阶段目标

在底座统一后，集中处理协议解析、SQL 安全、AI 调用和进程生命周期问题。

---

### 4.1 替换手写 JSON 解析

#### 目标

让消息解析和 AI 响应解析不再依赖字符串查找。

#### 核心修改

**文件：**

- `backend/src/module/ChatService.cpp`
- `backend/src/module/ai_api/AiService.cpp`
- 可能新增一个 JSON 工具封装文件
- 构建脚本 / 第三方依赖配置

#### 具体动作

1. 引入一个轻量 JSON 库。
2. 将 `Message` 解析改为统一反序列化流程。
3. `extra` 统一先解析为对象，再按类型读取字段。
4. AI 返回解析同样改为 JSON 库读取，而不是字符串截取。

#### 重点代码修改点

- `backend/src/module/ChatService.cpp:29-100`
- `backend/src/module/ChatService.cpp:397-409`
- `backend/src/module/ai_api/AiService.cpp:104-220`

#### 预期收益

- 降低协议演进成本
- 降低嵌套字段解析 bug
- 避免多个模块维护多套 JSON 规则

---

### 4.2 收敛 SQL 构造方式

#### 目标

优先堵住高频入口的 SQL 注入风险，并建立统一写法。

#### 核心修改

**文件：**

- `backend/src/model/UserDAO.cpp`
- `backend/src/model/ChatRecordDAO.cpp`
- `backend/src/module/friend/GroupService.cpp`
- `backend/src/module/ai_api/AiService.cpp`
- 其他直接拼接 SQL 的 DAO/Service 文件

#### 具体动作

1. 第一轮至少统一到 `Database::escapeString()`。
2. 第二轮如果成本可接受，逐步改为参数化封装。
3. Service 层不再自己拼业务 SQL，尽量下沉到 DAO。

#### 优先修复入口

1. 注册/登录
2. 用户资料更新
3. 消息发送与更新
4. 群创建 / 群名修改 / 加群申请
5. AI 设置保存

#### 重点代码修改点

- `backend/src/model/UserDAO.cpp:26-33`
- `backend/src/model/UserDAO.cpp:63-69`
- `backend/src/model/UserDAO.cpp:196-197`
- `backend/src/model/ChatRecordDAO.cpp:98-105`
- `backend/src/module/friend/GroupService.cpp:58-63`
- `backend/src/module/ai_api/AiService.cpp:59-86`

---

### 4.3 清理 `system()` 调用与 AI 回退链路

#### 目标

消除命令拼接和环境依赖带来的额外风险。

#### 核心修改

**文件：**

- `backend/src/module/ChatService.cpp`
- `backend/src/module/ai_api/AiService.cpp`

#### 具体动作

1. 将头像目录创建改为标准库文件系统接口。
2. 删除 `system(curl ...)` 形式的 AI HTTP 回退。
3. 只保留一种正式 HTTP 调用实现。
4. 统一超时、重试、日志输出逻辑。

#### 预期结果

- 降低命令注入与环境差异风险
- AI 调用链路更容易调试和测试

---

### 4.4 收敛 AI 异步执行模型

#### 目标

把当前“一个请求一个 detached thread”改成可控模型。

#### 核心修改

**文件：**

- `backend/src/module/ai_api/AiService.cpp`
- 可能新增任务队列/工作线程封装

#### 具体动作

1. 用固定 worker 数量的任务队列代替 `std::thread(...).detach()`。
2. 明确请求入队、处理、超时、完成回调的状态。
3. 服务关闭时支持等待任务完成或安全丢弃。

#### 重点代码修改点

- `backend/src/module/ai_api/AiService.cpp:333-336`

#### 验证标准

- 连续多次 AI 请求不会无限膨胀线程数
- 服务关闭时不会留下不可控后台线程

---

### 4.5 收敛进程生命周期

#### 目标

避免信号处理器里直接 `delete` 资源。

#### 核心修改

**文件：**

- `backend/src/main.cpp`

#### 具体动作

1. 去掉全局裸指针直接释放模式。
2. 信号处理只设置停止标志，不做复杂释放。
3. 在主流程里统一 stop/shutdown。
4. 尽量改用 RAII 管理数据库与服务对象。

#### 重点代码修改点

- `backend/src/main.cpp:16-29`
- `backend/src/main.cpp:72-87`
- `backend/src/main.cpp:198-214`

---

## 第三阶段：P2 可维护性与演示体验完善

### 阶段目标

在核心链路稳定后，修正前端状态管理、启动脚本、文档和最小测试闭环。

---

### 5.1 前端 store 收敛

#### 目标

降低 `frontend/src/store/index.js` 的职责密度，修正本地会话与消息合并策略。

#### 核心修改

**文件：**

- `frontend/src/store/index.js`
- `frontend/src/api/websocket` 相关文件
- 如需要，可新增 `store/session.js`、`store/chat.js` 等模块

#### 具体动作

1. 将连接状态、会话状态、聊天状态、联系人状态拆分。
2. 登录恢复以“连接成功 + 服务端身份确认”为准。
3. 补稳定的消息唯一标识，不再仅依赖时间戳去重。
4. 将 WebSocket / 头像服务地址提取为配置。

#### 重点代码修改点

- `frontend/src/store/index.js:66-85`
- `frontend/src/store/index.js:107-139`
- `frontend/src/store/index.js:279-299`
- `frontend/src/store/index.js:25`
- `frontend/src/store/index.js:71`

---

### 5.2 修复启动脚本

#### 目标

保证最小开发启动命令可直接使用。

#### 核心修改

**文件：**

- `package.json`

#### 具体动作

1. 修复 `&amp;&amp;` / `&amp;` 被 HTML 转义的问题。
2. 明确推荐启动方式：
   - 后端
   - 代理
   - 前端
3. 如有必要，拆分为更明确的脚本：
   - `dev:backend`
   - `dev:proxy`
   - `dev:frontend`
   - `dev`

---

### 5.3 更新文档

#### 目标

让文档反映真实实现，而不是理想结构。

#### 核心修改

**文件：**

- `docs/ARCHITECTURE.md`
- `docs/README.md`
- 可补充 `docs/SETUP.md` 或直接更新 README（如果需要）

#### 具体动作

1. 架构文档改为“当前实现 + 已知限制”。
2. 写清：
   - 前端 → WebSocket 代理 → TCP 后端
   - 字符串业务 ID 与数据库主键的边界
   - AI 请求链路
   - 数据库初始化步骤
3. 加入最小联调说明。

---

### 5.4 建立最小验证闭环

#### 目标

确保每次整改后都能快速验证没有把主链路改坏。

#### 验证清单

1. 初始化数据库
2. 启动后端
3. 启动代理 `proxy-server.js`
4. 启动前端
5. 注册
6. 登录
7. 获取好友列表
8. 添加好友
9. 私聊发消息
10. 创建群聊
11. 申请加群 / 审批
12. 发群消息
13. 触发 AI 回复
14. 刷新页面后查看历史记录
15. 重新连接后状态恢复

#### 建议方式

- 短期：整理成手工验收 checklist
- 中期：把关键接口做成最小脚本化测试

---

## 6. 推荐实施顺序

### 第 1 周：先做 P0

1. 统一 `init.sql`
2. 废弃错误 migration 方向
3. 移除运行时删表修表
4. 统一验证码表字段
5. 修复群聊/历史记录 ID 映射

### 第 2 周：做 P1

1. 引入 JSON 库并替换消息解析
2. 收敛 DAO / Service SQL 写法
3. 去掉 `system()` 调用
4. 改造 AI 异步执行模型
5. 修复 main 生命周期管理

### 第 3 周：做 P2

1. 拆分前端 store
2. 修复根脚本
3. 更新架构文档与启动说明
4. 建立联调 checklist

如果时间有限，建议最少先完成：

- P0 全部
- P1 中的 JSON 替换 + SQL 收敛 + 去掉 `system()`

这是“最小但有效”的整改范围。

---

## 7. 风险与注意事项

### 7.1 最大风险：历史数据兼容

如果数据库里已经有旧数据，P0 阶段会面临历史表结构和旧数据兼容问题。整改前要先明确：

- 当前数据库是否只用于本地演示
- 是否需要保留既有测试数据

如果只是演示库，最简单做法是：

- 固化新 schema
- 清库重建
- 用标准化样例数据重新初始化

这比在错误 schema 上继续补兼容要更稳。

### 7.2 第二风险：前后端同时改动导致联调中断

因为前端已经有较完整交互，整改时不要同时大改协议字段名。建议策略是：

- 先保持前端协议不变
- 后端内部完成映射和修复
- 等底座稳定后，再逐步优化前端 store

### 7.3 第三风险：范围蔓延

本次整改很容易从“修底座”变成“大重构”。要控制范围：

- 不新增功能
- 不改 UI 结构
- 不做无关抽象
- 每一处修改都要能追溯到报告中的问题项

---

## 8. 最终交付标准

完成整改后，至少应满足以下标准：

1. `db/mysql/init.sql` 可独立初始化新环境。
2. 服务启动不再删表、改表。
3. 登录/好友/群聊/AI/历史记录在同一套 schema 下可跑通。
4. 后端消息解析不再依赖手写字符串查找。
5. 关键 DAO 写入路径不再直接拼接未转义用户输入。
6. AI 请求不再使用 `system(curl ...)` 与 detached thread。
7. 根目录脚本可真实启动项目。
8. 文档能准确描述当前实现。

---

## 9. 一句话结论

这次整改的核心不是“把代码写得更漂亮”，而是 **先把项目从多套口径并存的状态，收敛成一套可以稳定运行、稳定演示、稳定继续开发的工程底座**。

先收 schema 和 ID，再收协议、SQL 和 AI，最后再补前端与文档，这是当前分支成本最低、成功率最高的实施路径。