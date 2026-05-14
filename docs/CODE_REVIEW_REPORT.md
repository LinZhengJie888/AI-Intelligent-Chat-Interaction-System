# 当前分支代码 Review 报告

## 1. 结论概览

当前 `main` 分支和上一次看到的分支差异较大：前端已经从早期静态原型演进为接入 WebSocket 代理、具备登录/聊天/群聊/AI 设置等完整页面流的可运行形态；但后端的数据模型、表结构、服务初始化方式和协议实现仍存在明显断层。

如果只看展示效果，这个分支已经比之前完整很多；但如果从“可持续开发、可稳定演示、可继续扩展”的角度看，当前项目最核心的问题已经不是页面完成度，而是 **后端对 ID 模型、数据库结构、协议解析和运行时行为没有形成统一口径**。这会直接导致功能不稳定、环境依赖强、上线风险高。

当前最显著的问题优先级建议如下：

1. **数据库 schema 与业务代码严重漂移**，并且部分漂移通过运行时改表/删表硬修复。
2. **字符串业务 ID 与数据库数值主键混用**，群聊、好友、聊天记录链路都受影响。
3. **消息协议与 JSON 解析实现脆弱**，复杂字段依赖手写字符串解析。
4. **DAO 与服务层仍有明显安全/稳定性问题**，包括 SQL 拼接、`system()` 调用、分离线程。
5. **文档、启动脚本、真实实现存在偏差**，会放大维护和演示成本。

---

## 2. 当前分支的积极变化

相比之前误看的分支，当前分支已经有一些明确进展：

### 2.1 前端已经不是纯静态样板

`frontend/src/store/index.js:66` 开始的逻辑说明当前前端已经接入实际 WebSocket 流程，包含：

- 登录、注册响应处理
- 好友列表、群聊列表、好友请求列表拉取
- 私聊消息、群聊消息、聊天记录处理
- AI 请求与 AI_AT 调用
- 本地 `localStorage` 会话恢复

`frontend/src/App.vue:23` 也已经增加连接状态提示，说明前端开始围绕真实后端连接做交互，而不是只做视觉展示。

### 2.2 代理层已经补上浏览器接入桥梁

`proxy-server.js:62` 使用 `ws` 建立浏览器 WebSocket 入口，再转发到后端 TCP 服务；`proxy-server.js:77` 以后还做了长度头拆包。这个设计让前端可以直接复用浏览器环境，绕过浏览器无法直接使用原生 TCP 的限制，方向本身是合理的。

### 2.3 AI 功能已从“单独入口”走向“聊天上下文能力”

从 `frontend/src/store/index.js:199` 到 `frontend/src/store/index.js:310` 可以看到 AI 回复已经作为私聊/群聊消息流的一部分处理，且前端存在 `chatAISettings` / `currentAISettings` 概念，这比早期分支的能力更完整。

这些变化说明项目已经进入“真实联调阶段”，所以接下来最需要做的不是继续堆界面，而是先把后端底座统一。

---

## 3. 显著问题

## 3.1 数据库 schema 与业务实现已经出现系统性漂移

这是当前分支最严重的问题。

### 现象 1：初始化脚本与服务代码对同一张表的字段定义不一致

`db/mysql/init.sql:36-66` 定义的群相关表是：

- `group_chat.creator_id` 为 `BIGINT UNSIGNED`
- `group_member.group_id` / `user_id` 为 `BIGINT UNSIGNED`
- `group_request.group_id` / `from_user_id` 为 `BIGINT UNSIGNED`

但 `backend/src/module/friend/GroupService.cpp:136-151` 实际插入的是：

- `group_chat.creator_id` 写入字符串 `creator_id`
- `group_member.group_id` / `user_id` 也写入字符串业务 ID

这不是简单的“类型没对齐”，而是 **同一条业务链路在设计层面同时存在两套 ID 体系**。

### 现象 2：服务初始化时直接删表、改表来“修” schema

`backend/src/module/friend/GroupService.cpp:112-115` 调用 `fixTableSchemas()`，而该函数内部包含：

- 修改 `chat_record.group_id` 列类型
- 删除外键 `fk_chat_record_group`
- `DROP TABLE IF EXISTS group_request`
- `DROP TABLE IF EXISTS group_member`
- `DROP TABLE IF EXISTS group_record`
- `DROP TABLE IF EXISTS group_chat`
- 然后重新建表

这意味着：

- 服务启动行为不再只是“初始化依赖”，而是会主动重塑核心业务表
- 一旦线上或演示库里已有真实数据，启动本身就可能破坏数据完整性
- SQL 初始化脚本将不再是系统真实结构的唯一来源，环境可复现性很差

这是当前代码里最不应该继续保留的实现方式之一。

### 现象 3：迁移脚本与初始化脚本方向也不一致

`db/mysql/migrate_fix_ids.sql:13-39` 又把 `friend_relation`、`friend_request` 的 `user_id` 字段从数值型改成 `VARCHAR(32)`，并且直接 `TRUNCATE` 旧数据。

也就是说当前仓库至少同时存在三种 schema 来源：

1. `db/mysql/init.sql`
2. `db/mysql/migrate_fix_ids.sql`
3. `GroupService::fixTableSchemas()` 运行时修表

这会导致任何新环境都无法仅通过一套标准步骤稳定复现出“正确结构”。

### 影响

- 新环境初始化结果不可预测
- 业务联调经常出现“表能建但功能不通”
- 群聊/好友/历史记录功能容易表现为部分可用、部分异常
- 后续所有开发都会先被环境问题拖慢

### 优化方向

- 立即确定唯一可信 schema 来源，只保留一套正式数据库定义
- 禁止服务启动时删表/改表修复结构
- 把所有 schema 变更都收敛到显式迁移脚本中
- 在群聊、好友、聊天记录三条链路上统一 ID 策略后再重建 DAO/Service 实现

---

## 3.2 业务字符串 ID 与数据库数值主键混用，导致链路长期不稳定

项目当前明显同时使用：

- 面向前端/业务的 `user_id`、`group_id` 字符串标识
- 面向数据库关系的自增 `id`

问题不在于“双 ID 模型”本身，而在于当前代码没有清晰边界。

### 典型表现 1：查数字 ID，再回写字符串 ID

`backend/src/module/friend/GroupService.cpp:125-139` 先用 `getUserIdNum()` 查到数值 ID，但真正插入 `group_chat.creator_id` 时又写回 `creator_id` 字符串。

这说明代码作者知道数据库关系层需要数字 ID，但最终实现仍然被业务字符串 ID 拉回去了。

### 典型表现 2：聊天记录查询仍依赖数值 group_id

`backend/src/model/ChatRecordDAO.cpp:200-206` 的 `findByGroup()` 仍然按 `uint64_t group_id` 查询。

而 `backend/src/module/ChatService.cpp` 的群聊历史路径里，会先把业务 `target_id` 转数字：若转失败，就退化为 `findByGroup(0)` 返回空结果。这类逻辑会直接让“群存在但查不到历史”成为常态问题，而不是边界问题。

### 典型表现 3：前端全程使用字符串 ID

`frontend/src/store/index.js:119`、`frontend/src/store/index.js:175`、`frontend/src/store/index.js:259` 等位置都说明前端消息和状态管理完全依赖字符串 `userId` / `groupId`。

这意味着：当前系统的真实接口口径已经偏向字符串业务 ID，但后端存储层没有彻底跟上。

### 影响

- 群聊消息落库和查询链路容易断裂
- 好友、群成员、加群申请等关系查询容易出现“部分成功”
- 需要到处写 `getUserIdNum()` / `getGroupNumId()` 之类胶水代码
- DAO 无法稳定表达真实业务含义

### 优化方向

建议尽快二选一，不要继续折中：

- **方案 A：数据库关系层全部统一使用数值主键，自定义字符串 ID 仅作为业务展示/外部接口字段**
- **方案 B：数据库关系层也全面切换为字符串业务 ID，并重写所有关联表和 DAO**

结合当前已有 `user.id`、外键、聊天记录表设计，**更建议走方案 A**：

- 前端和协议继续传字符串业务 ID
- 进入 Service 后统一完成一次字符串 ID → 数值主键映射
- DAO 和关系表全部只处理数值主键
- 出口响应再转回字符串业务 ID

这比现在这种“有时转、有时不转”的混合态要稳定得多。

---

## 3.3 验证码表结构存在直接运行风险

`backend/src/module/verify/VerifyService.cpp:32-43` 初始化验证码表时使用的是：

- `token VARCHAR(64) NOT NULL UNIQUE`
- `create_time DATETIME`

但 `db/mysql/init.sql:146-157` 定义的是：

- `captcha_token VARCHAR(64)`
- `send_time DATETIME`

而当前服务其他读写路径仍在围绕 `captcha_token` 查询/插入。

这意味着：

- 如果数据库由 `init.sql` 建立，`VerifyService` 运行时表结构认知不一致
- 如果数据库由 `VerifyService::init()` 建立，又会偏离正式 schema
- 登录/注册验证码链路可能出现“表存在但字段不匹配”的直接故障

这是一个已经落到具体字段名上的功能性 bug，不是抽象设计问题。

### 优化方向

- 立刻统一 `verification_code` 的正式字段名与时间字段名
- 删除 `VerifyService` 内部自建另一版表结构的逻辑
- 验证码 DAO / Service 只对一份 schema 负责
- 补一条最小集成测试：生成验证码 → 刷新 → 校验 → 标记已使用

---

## 3.4 协议解析与 JSON 处理方式过于脆弱

`backend/src/module/ChatService.cpp:29-100` 与 `backend/src/module/ai_api/AiService.cpp:104-220` 都在手写 `getJsonValue()` 解析 JSON 字符串。

这类实现即使补了部分转义处理，也仍然存在天然问题：

- 对嵌套对象/数组的支持不完整
- 容易被特殊字符、转义组合、字段顺序影响
- 维护成本高，出现问题难定位
- 同一套解析逻辑在多个模块复制，行为可能逐步分叉

更关键的是，当前协议里 `extra` 本身就经常承载嵌套 JSON。

例如 `frontend/src/store/index.js:207-210`、`frontend/src/store/index.js:254-256` 会把 `extra` 当作 JSON 字符串再次解析；这说明消息体已经进入“嵌套结构普遍存在”的阶段，继续靠字符串查找解析会越来越脆。

### 影响

- 一旦 AI 设置、群消息扩展字段变复杂，解析 bug 会增多
- 后端协议升级成本高
- 前后端联调时很难判断是业务问题还是解析问题

### 优化方向

- 引入稳定 JSON 库，统一消息解析和序列化
- 把 `Message` 解析收敛为单一入口，不允许业务模块自己字符串拆 JSON
- 为 `extra` 建立明确的数据结构，不再让不同模块自行猜字段

这是当前项目中性价比很高的一项重构：改动不一定最大，但能显著降低后续复杂度。

---

## 3.5 DAO 层仍存在明显 SQL 注入与数据污染风险

### UserDAO

`backend/src/model/UserDAO.cpp:26-33`、`backend/src/model/UserDAO.cpp:63-69`、`backend/src/model/UserDAO.cpp:196-197` 直接使用 `snprintf` 拼接用户输入字段构造 SQL。

涉及字段包括：

- `user_id`
- `username`
- `nickname`
- `password`
- `phone`
- `avatar_path`
- `ai_nickname`

这属于标准注入风险入口。

### ChatRecordDAO

`backend/src/model/ChatRecordDAO.cpp:27` 虽然在 `insert()` 中开始对 `content` 做 `escapeString()`，这是一个进步；但 `update()` 里 `content` 仍然直接拼接：

`backend/src/model/ChatRecordDAO.cpp:98-105`

说明当前修复并没有形成统一约束，只是局部补丁。

### GroupService / 辅助查找函数

`backend/src/module/friend/GroupService.cpp:58-63` 和 `backend/src/module/ai_api/AiService.cpp:59-86` 的辅助查询同样继续直接拼接字符串 ID。

### 影响

- 注册、改资料、聊天内容、群名等多入口都可能带入非法 SQL 字符
- 问题分散在 DAO 和 Service，不容易一次性收敛
- 后续如果要补审计/风控，也缺少可信边界

### 优化方向

- 至少统一到 `Database::escapeString()` 一层
- 更好的方式是收敛到参数化语句或封装型 DAO 接口
- 不要再让 Service 层随手拼 SQL
- 先处理高频输入链路：登录注册、好友/群操作、消息发送、AI 设置保存

---

## 3.6 运行时存在不必要的危险调用和资源管理问题

### `system()` 调用

- `backend/src/module/ChatService.cpp` 的头像目录逻辑使用 `system("mkdir -p ...")`
- `backend/src/module/ai_api/AiService.cpp` 存在 `system(curl ...)` 形式的回退调用

这些调用会带来：

- 环境依赖增强
- 错误处理困难
- 命令拼接安全风险
- 跨平台与可测试性差

### 分离线程处理 AI 请求

`backend/src/module/ai_api/AiService.cpp` 中通过 `std::thread(...).detach()` 异步处理 AI 请求。

这类实现短期可跑，但长期问题明显：

- 请求多时线程数不可控
- 服务关闭时缺少统一回收
- 与数据库、缓存、连接对象生命周期边界不清晰

### 信号处理与资源释放方式粗糙

`backend/src/main.cpp` 中全局裸指针 + 信号处理器里直接 `delete` 的做法仍然存在。这会继续放大关闭阶段的不确定性。

### 优化方向

- 文件系统操作改为 C++ 标准库或明确封装
- AI HTTP 调用只保留一种正式实现，不保留 `system(curl ...)` 兜底
- AI 异步处理改为受控任务队列/线程池
- 服务生命周期统一到 RAII 或明确的 stop/shutdown 流程

---

## 3.7 前端状态管理已接入真实业务，但会话与数据一致性仍偏弱

当前前端比之前完整，但 `frontend/src/store/index.js` 也暴露出几个后续问题：

### 现象 1：本地登录态过度依赖 `localStorage`

`frontend/src/store/index.js:111-128`、以及初始化恢复逻辑中会直接从 `localStorage` 读取 `user-info` 恢复登录态。

这会导致：

- 页面刷新后可能出现“本地已登录、服务端未恢复连接”的短暂不一致
- 头像等数据存在本地覆盖服务端字段的情况
- 登录状态更像前端缓存状态，而不是完整会话状态

### 现象 2：消息去重与合并逻辑偏脆

`frontend/src/store/index.js:283-299` 当前用时间戳做本地消息去重。若服务端消息时间精度、顺序、补发策略变化，容易出现重复或漏合并。

### 现象 3：连接地址写死

`frontend/src/store/index.js:71` 写死连接 `ws://localhost:8081`；`frontend/src/store/index.js:25` 拼头像地址时也默认 `:8082`。

这会让部署、联调、跨机器访问都比较受限。

### 优化方向

- 登录态应以连接恢复/服务端校验结果为准，而不只是本地缓存
- 消息去重尽量引入稳定 message id
- WebSocket / 静态资源地址提取到统一配置
- 前端 store 逐步拆分为连接、会话、聊天、联系人等模块，降低单文件复杂度

当前前端不是最急的风险点，但在后端口径统一后，需要尽快做这一轮收敛。

---

## 3.8 启动脚本与文档存在明显漂移

### 根目录脚本疑似被 HTML 转义污染

`package.json:7-9` 中脚本内容是：

- `cd frontend &amp;&amp; npm install &amp;&amp; npm run dev`
- `npm run dev:proxy &amp; npm run dev:frontend`

如果文件中保存的就是 `&amp;` 字面量，那么脚本将无法按预期执行。

这类问题说明项目的“可启动性”还没有被稳定验证。

### 架构文档仍带有理想化描述

`docs/ARCHITECTURE.md` 虽然比更早版本更新了一些前端信息，但仍然保留较多“预留”“支持”“可实现”的规划性描述；与当前后端真实实现相比，文档对 schema 漂移、代理层、真实消息链路、AI 线程模型等关键事实没有准确落地。

### 影响

- 新人接手会被文档误导
- 演示环境更容易出现“文档说能跑，实际上启动失败”
- Review、答辩、简历展示时很难讲清真实技术结构

### 优化方向

- 先修正根目录脚本，保证最小启动路径真实可用
- 把架构文档从“目标设计说明”改成“当前实现说明 + 已知偏差”
- 报告、设计、数据库脚本之间保持同一术语体系

---

## 4. 优先级排序建议

### P0：先解决，不然项目继续开发会反复踩坑

1. **统一 ID 体系与正式 schema**
2. **移除运行时删表/改表逻辑**
3. **修复验证码表字段名冲突**
4. **修复聊天记录/群聊历史链路中的 ID 不一致问题**

### P1：紧接着做，解决稳定性与安全问题

1. **替换手写 JSON 解析**
2. **收敛 SQL 构造方式，先堵高频注入入口**
3. **移除 `system()` 调用，规范 AI 调用链**
4. **把 AI 异步改成可控执行模型**

### P2：在底座稳定后做，提高可维护性与演示质量

1. **整理前端 store 和连接配置**
2. **完善消息唯一标识与去重策略**
3. **修正文档与启动脚本**
4. **补最小联调测试与初始化说明**

---

## 5. 后续优化方向

## 5.1 先把“数据口径”作为唯一主线收敛

当前项目的最大问题不是少功能，而是同一个功能在不同层有不同数据解释方式。后续优化必须围绕“统一口径”展开：

- 用户、群聊、好友、聊天记录统一 ID 解释方式
- 前端协议字段和后端服务字段统一
- DAO、Service、SQL 文件对同一张表结构保持一致

只有这一层统一后，其他优化才不会持续返工。

## 5.2 以 Service 为边界，重新划清职责

当前 `ChatService`、`GroupService`、`AiService` 都承担了过多职责：

- 协议解析
- 业务校验
- ID 转换
- SQL 访问
- JSON 拼装
- 异步调度
- 部分初始化/修表职责

后续建议：

- `ChatService` 只负责协议分发与会话路由
- `Friend/Group/AI/Verify` 只负责领域业务
- DAO 只负责数据持久化
- schema 迁移只由 SQL migration 管理

这样才能把 bug 范围真正压缩到模块边界内。

## 5.3 为“能稳定演示”建立最小验证闭环

当前项目很适合先建立一条最小可验证链路：

1. 初始化数据库
2. 启动后端
3. 启动代理
4. 启动前端
5. 注册/登录
6. 添加好友
7. 私聊发消息
8. 创建群聊/发群消息
9. 触发 AI 回复
10. 刷新页面后验证历史记录

围绕这条链路补最小测试或验收脚本，能最快暴露 schema、协议和状态恢复问题。

---

## 6. 总结

当前 `main` 分支的真实状态可以概括为：

- **前端接入度明显提升，项目展示层已接近“完整产品原型”**；
- **后端底层一致性问题仍然很重，尤其是 schema、ID 与运行时行为三者之间没有统一口径**。

因此，下一阶段最正确的优化方向不是继续补界面，而是：

**先把数据库结构、ID 模型、消息解析和服务初始化方式统一，再继续做功能增强。**

如果这一步不先做，后面每加一个功能，都只是在当前不稳定底座上继续叠复杂度。