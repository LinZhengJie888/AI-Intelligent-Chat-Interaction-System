# 代码 Review 报告

## 1. 总体结论

当前项目已经搭起了“C++ 后端 + Vue 前端 + MySQL/Redis/AI 接口”的基本骨架，前端静态界面完整度还可以，后端也已经覆盖登录、好友、群聊、AI 调用等主要模块。

但从“可运行 Demo”走向“可持续开发/可实际联调”的角度看，项目目前最显著的问题不是某个单点 bug，而是：**设计文档、数据库模型、后端实现、前端实现之间存在明显脱节**。这会直接导致后续联调成本高、问题难排查、功能越加越乱。

## 2. 当前显著问题

### 2.1 前端目前基本还是静态原型，未真正接入业务

- 登录页提交后直接把本地状态改成已登录，没有任何接口调用，见 `web/src/screens/LoginScreen.vue:7` 和 `web/src/store/index.js:24`
- 聊天列表、单聊页都使用本地写死数据，未从后端拉取，见 `web/src/screens/ChatListScreen.vue:4` 和 `web/src/screens/SingleChatScreen.vue:7`
- 全局“登录态”仅靠 `localStorage` 保存 screen 名称恢复，刷新后就默认视为已登录，见 `web/src/store/index.js:29`
- `App.vue` 用 `v-if` 手动切屏，整个前端没有路由、没有 API 层、没有会话状态模型，见 `web/src/App.vue:23`

**影响：** 当前前端更像视觉稿/交互稿，不是真正可联调客户端。后续一旦接 WebSocket、登录态、好友列表、群消息，改动面会很大。

### 2.2 后端数据模型不一致，字符串 ID 和数字 ID 混用严重

这是当前最关键的问题之一。

- 初始化脚本把 `group_member.user_id`、`group_request.from_user_id` 等字段定义为 `BIGINT` 外键，见 `db/mysql/init.sql:57`、`db/mysql/init.sql:109`
- 但 `GroupService` 里大量 SQL 直接把这些字段当字符串 `user_id` / `group_id` 来用，见 `backend/src/module/friend/GroupService.cpp:156`、`backend/src/module/friend/GroupService.cpp:174`、`backend/src/module/friend/GroupService.cpp:590`
- `createGroupChatTable()` 又把 `creator_id` 定义成 `VARCHAR(32)`，和初始化脚本中的 `BIGINT UNSIGNED` 冲突，见 `backend/src/module/friend/GroupService.cpp:775`
- `getGroupMembersStruct()` 用 `gm.user_id = u.user_id` 做 join，而初始化脚本中 `gm.user_id` 是数值型外键，`u.user_id` 是业务字符串 ID，见 `backend/src/module/friend/GroupService.cpp:356`
- `ChatService::handleChatHistory()` 私聊历史查询直接对业务字符串 ID 做 `strtoull`，而私聊发送逻辑又先把业务 ID 转成数据库主键 ID，前后口径不一致，见 `backend/src/module/ChatService.cpp:900` 和 `backend/src/module/ChatService.cpp:949`

**影响：** 群聊、历史消息、成员查询、权限判断等功能很容易出现“能编译、但一跑就查不到数据/关联错数据”的问题。

### 2.3 SQL 拼接过多，存在明显注入风险

多个核心路径直接用 `snprintf` 拼接用户输入进入 SQL：

- 用户写入与查询：`backend/src/model/UserDAO.cpp:26`、`backend/src/model/UserDAO.cpp:196`
- 聊天记录写入：`backend/src/model/ChatRecordDAO.cpp:28`、`backend/src/model/ChatRecordDAO.cpp:98`
- 群聊相关：`backend/src/module/friend/GroupService.cpp:172`、`backend/src/module/friend/GroupService.cpp:245`
- AI 设置更新：`backend/src/module/ai_api/AiService.cpp:523`
- 路由层辅助查询：`backend/src/module/ChatService.cpp:131`

虽然 `Database` 提供了 `escapeString()`，见 `backend/src/module/Database.cpp:134`，但只有极少数地方真正使用，例如 AI 日志写入 `backend/src/module/ai_api/AiService.cpp:733`。

**影响：** 登录、注册、群聊、聊天、昵称等只要带引号或恶意内容，就可能破坏 SQL 语句甚至形成注入漏洞。

### 2.4 JSON 协议解析是手写字符串处理，健壮性差

- `ChatService` 自己写了 `getJsonValue()`，靠字符串查找解析 JSON，见 `backend/src/module/ChatService.cpp:29`
- `AiService` 也复制了一套类似逻辑，见 `backend/src/module/ai_api/AiService.cpp:79`
- 对嵌套对象、数组、转义字符、异常输入的处理都比较脆弱
- 一旦客户端字段顺序变化、嵌套更复杂、文本中包含边界字符，解析就容易出错

**影响：** 协议很难扩展，也很难定位线上“偶现解析失败”的问题。

### 2.5 验证码表字段名和实现不一致，模块存在运行时失败风险

- 初始化脚本使用字段 `captcha_token`，见 `db/mysql/init.sql:150`
- `VerifyService::init()` 动态建表时使用字段 `token`，见 `backend/src/module/verify/VerifyService.cpp:37`
- `saveCaptchaToDB()` 插入 `captcha_token`，见 `backend/src/module/verify/VerifyService.cpp:323`
- `loadCaptchaFromDB()` 和 `updateCaptchaStatus()` 却查询/更新 `token` 字段，见 `backend/src/module/verify/VerifyService.cpp:337` 和 `backend/src/module/verify/VerifyService.cpp:381`

**影响：** 在不同建库路径下，验证码功能很可能直接失效，且问题会表现为“生成成功但验证失败”。

### 2.6 AI 模块实现可用性一般，协议与安全边界也不稳定

- 默认模型仍是 `gpt-3.5-turbo`，见 `backend/src/module/ai_api/AiService.cpp:166`，和当前主流模型已脱节
- `httpPost()` 在非 `USE_CURL` 分支中使用 `system(curl ...)`，见 `backend/src/module/ai_api/AiService.cpp:952`，存在命令拼接风险和可维护性问题
- `sendAIResponse()` 用 `ai_nickname` 当 `from_user_id` 发消息，见 `backend/src/module/ai_api/AiService.cpp:857`，这会让“发送者身份”和“用户业务 ID”混淆
- AI 回复落库时把 `target_id` 当作私聊 `receiver_id`，但群聊场景仍然写 `group_id = 0`，见 `backend/src/module/ai_api/AiService.cpp:797`

**影响：** AI 功能看起来已接入，但在消息归属、历史记录、身份表达和兼容性上都不稳。

### 2.7 后端生命周期和资源管理偏脆弱

- `main.cpp` 使用全局裸指针 `server`、`db`，见 `backend/src/main.cpp:16`
- 信号处理函数里直接 `delete` 对象和输出日志，见 `backend/src/main.cpp:19`，这不是很稳妥的信号处理方式
- `AiService::processRequest()` 每次请求直接 `detach` 一个线程，见 `backend/src/module/ai_api/AiService.cpp:251`

**影响：** 请求量上来后，线程失控、资源释放顺序问题、退出时状态异常等都可能出现。

### 2.8 文档与代码现状不一致，误导后续开发

- 架构文档中写了不少前端/头像/缓存/异常切换能力，但代码里很多尚未真正实现，见 `docs/ARCHITECTURE.md:37` 起
- 文档里说前端是 `HTML5 + CSS3 + JavaScript`，但实际是 Vue + Vite，见 `docs/ARCHITECTURE.md:39` 与 `web/package.json:1`
- 文档目录结构、模块目录和当前仓库结构也已有偏差，见 `docs/ARCHITECTURE.md:73`

**影响：** 新加入的人会依据错误文档判断系统边界，导致误解和重复返工。

## 3. 优先级最高的优化方向

### P0：先统一数据模型和协议口径

建议优先做这一件事，否则后续所有功能都会在错误地基上继续叠加。

建议统一规则：

- 对外协议：统一使用业务字符串 ID（如 `user_id`、`group_id`）
- 对内数据库关系：统一使用数值主键 `id`
- 在 Service 层做清晰转换，不要在 DAO 和路由层到处隐式转换
- 重新梳理 `user / group_chat / group_member / group_request / chat_record` 的字段语义
- 对照 `db/mysql/init.sql`、DAO、Service 全量校准一遍

这是最值得先投入的一轮修正。

### P1：补一层真正的 API / 消息协议边界

后端方面：

- 用成熟 JSON 库替换手写解析
- 明确请求/响应结构，统一错误码和 data 格式
- 把协议解析、业务处理、DAO 访问分层

前端方面：

- 增加 API/WebSocket 调用层
- 把“页面展示状态”和“服务端数据状态”拆开
- 至少把登录、会话列表、消息发送/接收先接通一条主链路

### P1：全面消除 SQL 直接拼接

建议统一处理：

- 所有用户输入都先 escape，最低限度先把现有漏洞补上
- 更推荐逐步换成预处理语句/参数化查询
- 先覆盖登录、注册、聊天发送、群管理、AI 设置这些入口

这是安全问题，也是稳定性问题。

### P1：把前端从“静态演示”升级成“可联调客户端”

建议最小化改造路径：

1. 增加路由或至少增加 screen 常量枚举，避免裸字符串切页
2. 增加 `api` 与 `socket` 模块
3. 把 store 改成明确的数据状态：用户信息、联系人、会话、当前聊天
4. 先打通登录 -> 拉会话列表 -> 进入单聊/群聊 -> 发消息 -> 收消息

不要一开始追求功能全，而是先做通一条闭环。

### P2：整理 AI 模块职责

建议把 AI 模块拆成三块：

- AI Provider 调用层：只负责请求外部模型
- AI 业务层：负责缓存、用户偏好、重试、日志
- 消息投递层：负责把 AI 回复转成聊天消息并落库

同时建议：

- 去掉 `system(curl ...)` 回退路径
- 明确 AI 消息发送者身份，不要用昵称充当 `from_user_id`
- 重新设计 AI 消息在私聊/群聊中的落库字段

### P2：降低并发与生命周期风险

建议：

- 把裸指针改成 RAII 管理
- 信号处理只做“设置退出标记”，不要在 handler 里做复杂析构
- AI 异步处理改为线程池/任务队列，而不是每请求一个 detach 线程

## 4. 建议的分阶段优化路线

### 第一阶段：修地基

目标：让系统“数据一致、能联调、能排错”。

- 统一 ID 体系
- 统一建表脚本和运行时建表逻辑
- 替换手写 JSON 解析
- 修复 SQL 拼接风险
- 修正聊天记录与群聊相关查询逻辑

### 第二阶段：打通主链路

目标：形成真实可用 demo。

- 前端接登录接口
- 前端接会话列表/好友列表
- 单聊和群聊接入真实消息收发
- 历史消息读取与展示接通
- AI 回复可在真实聊天流中展示

### 第三阶段：补工程能力

目标：让项目可持续迭代。

- 增加最基本的接口测试和 DAO 测试
- 增加日志分级与关键链路 tracing
- 整理配置项，避免散落在代码里
- 更新文档，使其与实现一致

## 5. 本次 review 的额外观察

- 前端构建可通过：`npm run build --prefix ./web`
- 后端 `make` 当前可通过，但这只能说明能编译，不代表运行时逻辑一致
- `makefile` 已经引用了 `verify`、`friend`、`ai_api`、`redis` 模块，见 `makefile:23`
- 当前最需要担心的是“模块都在，但口径不一致”，不是“模块不够多”

## 6. 最后建议

如果接下来只做一件事，我建议先做一轮 **“数据库模型 + Service 层 ID 体系 + 聊天协议” 对齐重构**。这一步完成后，再推进前端联调、AI 对接和功能扩展，投入产出比最高。
