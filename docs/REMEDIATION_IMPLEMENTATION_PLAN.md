# 代码整改实施方案

## 1. 文档目标

本方案基于 `docs/CODE_REVIEW_REPORT.md`，目标不是继续泛泛而谈“需要优化”，而是把现有问题拆成：

- 先做什么
- 为什么先做
- 具体改哪些模块
- 核心代码如何调整
- 每一阶段完成后的验收标准是什么

整体原则：**先修地基，再打通链路，最后补工程能力**。

---

## 2. 总体实施策略

### 2.1 实施总原则

1. **先统一口径，再新增功能**  
   当前最大问题是数据模型、协议、代码实现不一致，所以短期内不建议继续叠加头像上传、复杂群管理、更多 AI 能力。

2. **优先改“主干链路”**  
   优先保证：登录 -> 会话列表 -> 单聊/群聊 -> AI 回复 这条主链路真实可用。

3. **优先做可验证改动**  
   每一阶段都要有明确验收结果，而不是“代码看起来更规范了”。

4. **以最小重构换取最大稳定性**  
   不是全盘推翻重写，而是在保留现有模块边界的前提下，先消除错误口径和危险实现。

---

## 3. 分阶段实施路线

---

### 第一阶段：统一模型与协议地基（P0）

### 3.1 阶段目标

让系统具备以下前提：

- 数据表字段语义一致
- 业务 ID 和数据库主键 ID 的职责清晰
- 单聊、群聊、群成员、群申请、聊天记录的查询口径一致
- 前后端通信协议可以稳定扩展

这是整个整改的第一优先级。

### 3.2 本阶段核心优化方向

#### 方向一：统一 ID 体系

建议采用以下统一规则：

- **对外协议层**：使用字符串业务 ID
  - 用户：`user_id`
  - 群：`group_id`
- **数据库关系层**：使用数值主键
  - 用户主键：`user.id`
  - 群主键：`group_chat.id`
- **Service 层职责**：负责“业务 ID -> 数值 ID”的转换
- **DAO 层职责**：只处理确定字段，不再隐式猜测 ID 类型

#### 方向二：统一表结构来源

当前同时存在：

- 初始化脚本：`db/mysql/init.sql`
- 运行时自动建表：`backend/src/module/verify/VerifyService.cpp:30`
- 运行时自动建表：`backend/src/module/friend/GroupService.cpp:74`
- 运行时自动建表：`backend/src/module/ai_api/AiService.cpp:183`

建议规则：

- **业务主表统一以 `db/mysql/init.sql` 为唯一准源**
- 运行时只允许补充“缓存表/日志表”这类辅助表
- 不要再让 `GroupService` 和 `VerifyService` 动态创建核心业务表结构

#### 方向三：替换手写 JSON 解析

当前 `ChatService` 和 `AiService` 都有手写 `getJsonValue()`。

建议：

- 引入成熟 JSON 库，例如 `nlohmann/json`
- 在消息入口处完成解析和基础校验
- 统一请求体/响应体结构

### 3.3 本阶段核心代码修改点

#### 3.3.1 数据库与模型口径统一

**重点文件：**

- `db/mysql/init.sql`
- `backend/src/module/friend/GroupService.cpp`
- `backend/include/module/friend/GroupService.h`
- `backend/src/module/ChatService.cpp`
- `backend/src/model/ChatRecordDAO.cpp`
- `backend/src/model/UserDAO.cpp`

**修改内容：**

1. `group_member.user_id` 统一按 `BIGINT` 外键使用
2. `group_request.from_user_id` 统一按 `BIGINT` 外键使用
3. `group_chat.creator_id` 统一按 `BIGINT` 外键使用
4. `chat_record.sender_id / receiver_id / group_id` 全部严格按主键口径使用
5. `GroupService` 中所有字符串直查 SQL 改为：
   - 先通过业务 ID 找到数值 ID
   - 再基于数值 ID 查询/写入关系表

**示例改造思路：**

当前错误模式：

- `backend/src/module/friend/GroupService.cpp:590`
- `backend/src/module/friend/GroupService.cpp:611`

这里直接把 `user_id`、`group_id` 当作字符串写进 `group_member/group_chat` 关系查询。

建议改为：

- 先解析用户业务 ID -> `user.id`
- 先解析群业务 ID -> `group_chat.id`
- 再查询 `group_member(group_id, user_id)`

也就是说，`GroupService` 里应新增一组内部辅助函数，例如：

- `getUserPkByUserId()`
- `getGroupPkByGroupId()`
- `mustFindUserPk()`
- `mustFindGroupPk()`

这些函数可以放在：

- `backend/src/module/friend/GroupService.cpp`
- 或后续抽成独立 mapper/helper 文件

#### 3.3.2 聊天记录读写统一

**重点文件：**

- `backend/src/module/ChatService.cpp:872`
- `backend/src/module/ChatService.cpp:937`
- `backend/src/model/ChatRecordDAO.cpp`

**修改内容：**

1. 私聊消息发送时继续使用数值主键落库，这个方向是对的
2. `handleChatHistory()` 不能再对字符串 `user_id` 直接 `strtoull`
3. 历史消息读取时应与发送路径完全一致：
   - 先把 `from_user_id` / `to_user_id` 转换成数值主键
   - 再查询 `chat_record`
4. 群聊历史要统一以 `group_chat.id` 查询，不要混用 `group_id` 业务串

#### 3.3.3 验证码表结构统一

**重点文件：**

- `db/mysql/init.sql:146`
- `backend/src/module/verify/VerifyService.cpp`

**修改内容：**

1. 明确字段统一为 `captcha_token`
2. 修正以下方法对字段名的使用：
   - `saveCaptchaToDB()`
   - `loadCaptchaFromDB()`
   - `updateCaptchaStatus()`
3. `VerifyService::init()` 不再重新定义另一套不同字段名的验证码表

**建议方案：**

- 保留“确保表存在”的能力，但 SQL 必须和 `init.sql` 一致
- 更推荐直接移除验证码主表自动建表逻辑，把建库职责交给初始化脚本

#### 3.3.4 JSON 协议重构

**重点文件：**

- `backend/src/module/ChatService.cpp`
- `backend/src/module/ai_api/AiService.cpp`
- `makefile`

**修改内容：**

1. 在 `makefile` 中加入 JSON 库依赖
2. 删除 `getJsonValue()` 这类手写解析函数
3. 在 `ChatService::parseMessage()` 中直接解析为结构化字段
4. 统一 `extra` 字段的对象结构，不再把 JSON 当作普通字符串嵌套处理
5. `buildResponse()` 也改为基于 JSON 对象序列化输出

**建议目标消息格式：**

```json
{
  "type": 40,
  "from_user_id": "u1001",
  "to_user_id": "u1002",
  "content": "hello",
  "extra": {
    "is_group": false,
    "ai_nickname": "AI助手"
  },
  "timestamp": "2026-05-14 20:00:00"
}
```

不要继续使用：

- `extra` 里再包一层 JSON 字符串
- 通过字符串扫描取字段

### 3.4 本阶段验收标准

完成后至少满足：

- 登录、加群、群成员查询、聊天记录查询不再出现 ID 口径混乱
- 验证码生成与校验在同一套表结构下可正常工作
- 任意消息协议字段顺序变化，不影响解析
- 可以为第二阶段联调提供稳定接口基础

---

### 第二阶段：修复安全边界与主链路联调（P1）

### 4.1 阶段目标

让系统从“代码骨架存在”变成“可真实联调的 Demo”。

### 4.2 本阶段核心优化方向

#### 方向一：全面清理 SQL 注入风险

当前最现实的处理策略分两步：

**第一步：快速止血**

- 所有拼接 SQL 的用户输入先统一走 `Database::escapeString()`
- 先覆盖高风险入口

**第二步：逐步参数化**

- 中长期把核心写路径改成预处理语句

#### 方向二：打通前端 API / Socket 层

当前前端主要问题不是样式，而是没有业务通信层。

建议新增：

- `web/src/api/`：HTTP 或握手相关接口
- `web/src/socket/`：WebSocket/TCP 消息适配层
- `web/src/store/`：重构为真实状态管理
- `web/src/constants/`：screen、message type、error code 常量

#### 方向三：把登录与聊天做成闭环

优先做：

1. 登录
2. 拉好友/会话列表
3. 打开单聊
4. 发送消息
5. 接收消息
6. 拉取历史消息
7. AI 回复回流到聊天窗口

### 4.3 本阶段核心代码修改点

#### 4.3.1 SQL 安全改造

**重点文件：**

- `backend/src/model/UserDAO.cpp`
- `backend/src/model/ChatRecordDAO.cpp`
- `backend/src/module/friend/GroupService.cpp`
- `backend/src/module/ChatService.cpp`
- `backend/src/module/ai_api/AiService.cpp`

**优先修改位置：**

- `backend/src/model/UserDAO.cpp:24`
- `backend/src/model/UserDAO.cpp:61`
- `backend/src/model/UserDAO.cpp:160`
- `backend/src/model/ChatRecordDAO.cpp:23`
- `backend/src/model/ChatRecordDAO.cpp:95`
- `backend/src/module/friend/GroupService.cpp:171`
- `backend/src/module/friend/GroupService.cpp:243`
- `backend/src/module/ai_api/AiService.cpp:523`

**实施方式：**

- 先给每个用户可控文本字段做 escape
- 再逐步把高频操作改为参数化接口

如果本轮不想一次性引入 prepared statement，可以先在 `Database` 层补一个简单封装，例如：

- `escapeAndQuote()`
- `executeFormat()`

但这只是过渡，不是终态。

#### 4.3.2 前端状态管理重构

**重点文件：**

- `web/src/store/index.js`
- `web/src/App.vue`
- `web/src/screens/LoginScreen.vue`
- `web/src/screens/ChatListScreen.vue`
- `web/src/screens/SingleChatScreen.vue`
- `web/src/screens/GroupChatScreen.vue`

**改造目标：**

当前 store：

- 只保存 `currentScreen`
- 只保存少量标题文本
- 登录态是假登录

建议改为：

```js
{
  session: {
    connected: false,
    currentUser: null,
    token: null
  },
  ui: {
    currentScreen: 'login',
    currentConversation: null
  },
  contacts: [],
  conversations: [],
  messages: {}
}
```

同时：

- `switchScreen()` 只负责 UI 切换
- `login()` 必须变成异步请求
- `openChat()` 不再只改标题，而是设置当前会话并拉取消息

#### 4.3.3 增加 API / 通信适配层

**建议新增目录：**

- `web/src/api/client.js`
- `web/src/api/auth.js`
- `web/src/api/chat.js`
- `web/src/socket/chatSocket.js`
- `web/src/constants/messageTypes.js`

**作用：**

- 页面组件不直接拼协议
- 页面组件不直接操作原始连接
- 协议适配和状态更新逻辑集中管理

### 4.4 本阶段验收标准

完成后至少满足：

- 登录必须经过真实后端校验
- 刷新页面后不会仅凭 `localStorage` 假定用户已登录
- 单聊可以拉历史、发消息、收消息
- 群聊可以拉历史、发消息、收消息
- AI 回复可以进入真实消息流
- 主要输入点不再存在裸 SQL 拼接风险

---

### 第三阶段：整理 AI 模块与运行时稳定性（P2）

### 5.1 阶段目标

让 AI 模块、线程模型、资源管理不再成为未来扩展时的隐患。

### 5.2 本阶段核心优化方向

#### 方向一：拆分 AI 模块职责

当前 `AiService` 同时负责：

- 配置管理
- 调用外部模型
- 缓存
- 日志
- 请求调度
- 消息回推
- 聊天记录落库

职责过重。

建议拆分为：

1. **AIProvider / Client 层**  
   只处理 HTTP 调用与响应解析

2. **AIApplicationService 层**  
   处理用户设置、缓存、重试、日志

3. **AIMessageDispatcher 层**  
   负责把 AI 结果转成聊天消息、广播、落库

#### 方向二：统一 AI 消息身份

当前 `sendAIResponse()` 把 `ai_nickname` 当作 `from_user_id` 使用，这会破坏用户/消息模型。

建议改成：

- `from_user_id` 仍然是明确的系统身份，例如固定 AI 发送者标识
- `extra` 字段记录 `is_ai: true`
- 展示昵称由前端根据消息标记和用户 AI 设置决定

也就是说：

- **身份字段负责身份**
- **展示字段负责显示**
- 不能混用

#### 方向三：替换危险的命令调用与线程模型

当前风险点：

- `backend/src/module/ai_api/AiService.cpp:952` 使用 `system(curl ...)`
- `backend/src/module/ai_api/AiService.cpp:251` 每次请求 `detach` 新线程
- `backend/src/main.cpp:16` 和 `backend/src/main.cpp:19` 用全局裸指针 + 信号内析构

建议：

1. 强制只保留 `libcurl` 路径
2. 删除 `system(curl ...)` 回退逻辑
3. 把 AI 请求调度改为固定大小线程池 / 任务队列
4. 把 `main.cpp` 中全局对象改成 RAII 生命周期管理
5. 信号处理只设置退出标记，由主循环完成安全关闭

### 5.3 本阶段核心代码修改点

**重点文件：**

- `backend/src/module/ai_api/AiService.cpp`
- `backend/include/module/ai_api/AiService.h`
- `backend/src/main.cpp`
- `backend/src/reactor/ThreadPool.cpp`
- `backend/include/reactor/ThreadPool.h`

**建议修改：**

- 给 AI 请求复用现有线程池，或者新增专用任务队列
- `AiService::processRequest()` 改为入队，不再 `detach`
- `sendAIResponse()` 重构为明确区分：
  - 私聊 AI 回复
  - 群聊 AI 回复
  - 消息落库类型
- `main.cpp` 改为局部对象管理，而不是手动 `new/delete`

### 5.4 本阶段验收标准

完成后至少满足：

- AI 模块不再依赖 `system(curl ...)`
- AI 请求处理不会无限制创建线程
- AI 回复身份模型清晰
- 程序退出流程安全，不依赖信号处理内析构对象

---

### 第四阶段：补测试、日志和文档（P2）

### 6.1 阶段目标

让系统具备基本可维护性，避免每次改动都靠手工试。

### 6.2 本阶段优化方向

#### 方向一：补关键路径测试

建议优先测试：

- 用户注册/登录
- 验证码生成与校验
- 单聊消息写入与历史查询
- 群成员判断与群消息发送
- AI 请求缓存与响应落库

#### 方向二：整理日志与错误码

建议：

- 区分 info / warn / error
- 统一返回码语义
- 给登录、消息收发、AI 调用增加 request_id 或 trace_id

#### 方向三：更新文档

至少同步更新：

- `docs/ARCHITECTURE.md`
- `docs/README.md`
- 接口/协议说明文档

文档必须以“当前实现”为准，不能再写成目标想象图。

### 6.3 本阶段验收标准

- 关键主链路至少有基本测试覆盖
- 主要错误能从日志快速定位
- 架构文档、目录说明、技术栈说明与代码一致

---

## 4. 推荐的任务拆解顺序

建议实际执行顺序如下：

### 任务组 A：数据与协议修复

1. 校准 `init.sql` 与运行时建表逻辑
2. 修复 `VerifyService` 字段不一致
3. 修复 `GroupService` 的 ID 体系混用
4. 修复 `ChatService` 历史消息读取逻辑
5. 引入 JSON 库并替换手写解析

### 任务组 B：安全与联调

6. 修复 UserDAO / ChatRecordDAO / GroupService 的 SQL 拼接风险
7. 重构前端 store
8. 增加前端 API / socket 层
9. 打通登录与消息主链路

### 任务组 C：AI 与运行时

10. 重构 AI 模块职责
11. 删除 `system(curl ...)`
12. 改造 AI 请求线程模型
13. 修复 main 生命周期管理

### 任务组 D：工程化补齐

14. 补测试
15. 补日志规范
16. 更新文档

---

## 5. 风险与注意事项

### 5.1 不建议一次性同时重写前后端

因为当前问题虽然多，但主因集中在“口径不一致”。
先修模型和协议，再做联调，比直接推翻重写成本低很多。

### 5.2 不建议一开始就追求完整功能覆盖

短期最有价值的是：

- 登录真联调
- 单聊真联调
- 群聊基本可用
- AI 能进入真实消息流

而不是先做头像上传、复杂群权限、缓存高级策略。

### 5.3 要避免“文档先行、代码滞后”再次发生

这次整改里，任何文档更新都应放到对应代码完成之后，不要再提前描述未实现能力。

---

## 6. 最终建议

如果按收益排序，我建议你下一步按下面顺序推进：

1. **先做第一阶段：统一数据模型、修复群聊/聊天记录/验证码口径**
2. **再做第二阶段：补 API/Socket 层，打通前端主链路**
3. **然后做第三阶段：收敛 AI 模块和线程模型**
4. **最后补测试和文档**

其中最核心的一轮代码修改，会集中在这些文件：

- `db/mysql/init.sql`
- `backend/src/module/friend/GroupService.cpp`
- `backend/src/module/ChatService.cpp`
- `backend/src/module/verify/VerifyService.cpp`
- `backend/src/model/UserDAO.cpp`
- `backend/src/model/ChatRecordDAO.cpp`
- `backend/src/module/ai_api/AiService.cpp`
- `web/src/store/index.js`
- `web/src/screens/LoginScreen.vue`
- `web/src/screens/ChatListScreen.vue`
- `web/src/screens/SingleChatScreen.vue`
- `web/src/App.vue`

这批文件基本就是本项目第一轮整改的主战场。
