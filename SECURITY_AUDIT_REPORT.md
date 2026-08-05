# 书剑3 MUD 源码安全审计报告

**审计日期**: 2026-08-04  
**审计范围**: 完整源码（FluffOS LPC 游戏逻辑 + Node.js 桥接服务 + Web 前端 + 配置文件）  
**风险等级说明**: 🔴 严重 / 🟠 高危 / 🟡 中危 / 🟢 低危

---

## 一、总览

本次审计共发现 **26 个安全问题**，其中：

| 等级 | 数量 | 说明 |
|------|------|------|
| 🔴 严重 | 5 | 可导致系统被完全控制或数据泄露 |
| 🟠 高危 | 7 | 可导致权限绕过或数据篡改 |
| 🟡 中危 | 8 | 存在安全隐患但利用条件较苛刻 |
| 🟢 低危 | 6 | 最佳实践建议或配置优化 |

另发现 **7 个功能性 Bug**。

---

## 二、严重风险（🔴）

### 2.1 硬编码后门超级用户 — `securityd.c`

**文件**: [securityd.c](file:///workspace/shujian/adm/daemons/securityd.c#L148)  
**风险**: 后门账户

在 `trusted_read` 和 `trusted_write` 映射中，硬编码了用户 `"hongba"` 拥有根目录 `/` 的完全读写权限：

```c
// 第148行
mapping trusted_read = ([
    "/": ({ "master", "hongba" }),
    ...
]);

// 第230行
mapping trusted_write = ([
    "/": ({ "master", "hongba" }),
    ...
]);
```

`"hongba"` 用户拥有与 `ROOT_UID` 几乎同等的权限，可读写系统任意文件。这是一个明显的后门账户。

**建议**: 立即从 `trusted_read` 和 `trusted_write` 中移除 `"hongba"`，如确需超级管理员，使用 `(admin)` 状态并通过 `wizlist` 文件管理。

---

### 2.2 权限检查被绕过 — `securityd.c` valid_read()

**文件**: [securityd.c](file:///workspace/shujian/adm/daemons/securityd.c#L159-L227)  
**风险**: 权限检查完全失效

`valid_read()` 函数在第 179 行有一个**无条件 `return 1;`**，导致后续所有读取权限检查代码（第 180-227 行）永远不会被执行：

```c
int valid_read(string file, mixed user, string func)
{
    // ...
    switch (func) {
        case "file_size":
        case "stat":
            return 1;
    }
    return 1;   // <-- 第179行：无条件返回1，后续代码永不执行！
    // Get the euid and status of the user.  ... 以下代码全部无效
```

这意味着任何用户都可以读取服务器上的任意文件，包括 `/adm/etc/wizlist`、`/log/` 下的敏感日志等。

**建议**: 删除第 179 行的 `return 1;` 语句，恢复正常的权限检查逻辑。

---

### 2.3 SQL 注入漏洞 — `tohtml.c` 和 `bbsd.c`

**文件**: [tohtml.c](file:///workspace/shujian/cmds/wiz/tohtml.c#L179-L208) 和 [bbsd.c](file:///workspace/shujian/adm/daemons/bbsd.c#L63-L109)  
**风险**: SQL 注入

`tohtml.c` 中存在大量 SQL 字符串拼接，直接将用户输入的数据插入 SQL 语句：

```c
// tohtml.c 第179-181行
if (BBS_D->add_Bbs_Up_Map(WEB_DB_NAME, "REPLACE INTO mud_info (subject, utime, content, type, site)
        VALUES ('"+CHINESE_MUD_NAME+"在线玩家详细资料"+"', '"+time()+"', 
        '"+title+"', 'title', '"+lower_case(INTERMUD_MUD_NAME)+"')", ...))
```

虽然 `CHINESE_MUD_NAME` 和 `INTERMUD_MUD_NAME` 来自配置文件，但 `title` 变量中包含玩家名字、昵称等用户可控数据，只做了简单的 `replace_string(title,"'","\"")` 处理，仍可能被绕过。

`bbsd.c` 的 `bbs_post()` 函数（第 327-356 行）直接拼接用户输入到 SQL 中，且注释掉的 `update_pool()` 函数（第 254-281 行）包含更多 SQL 注入点。

**建议**: 使用参数化查询（prepared statements）替代字符串拼接，对所有用户输入进行严格转义。

---

### 2.4 GitHub Token 泄露 — `.env` 文件

**文件**: [.env](file:///workspace/shujian/.env)  
**风险**: 凭证泄露

`.env` 文件包含有效的 GitHub Personal Access Token：

```
GITHUB_TOKEN=ghp_***REDACTED***
```

虽然该文件已加入 `.gitignore`，但 (1) 文件仍存在于磁盘上，(2) 代码已推送到 GitHub 仓库，如果 Token 曾经被提交过，则历史记录中仍可找回。

**建议**: 
1. 立即在 GitHub 上撤销该 Token
2. 使用 `git filter-branch` 或 BFG Repo-Cleaner 清理 Git 历史
3. 生成新 Token 并妥善保管

---

### 2.5 任意 SQL 执行命令 — `cmds/adm/sql.c`

**文件**: [sql.c](file:///workspace/shujian/cmds/adm/sql.c)  
**风险**: 数据库被完全控制

该命令允许管理员用户直接执行任意 SQL 语句，仅过滤了 `*` 字符（防止 `SELECT *`）：

```c
int main(object me, string sql)
{
    if(!sql) return notify_fail("failed\n");
    if(strsrch(sql,"*")!=-1) return notify_fail("大哥小心点，别搞当机了。\n");
    ret = dbquery(sql);
    // ...
}
```

攻击者可以执行 `DROP TABLE`、`DELETE FROM`、`UPDATE` 等破坏性操作。

**建议**: 
1. 限制该命令仅允许 `(admin)` 级别用户执行
2. 添加白名单机制，只允许 `SELECT` 语句
3. 使用只读数据库连接执行查询

---

## 三、高危风险（🟠）

### 3.1 CORS 配置允许任意来源 — `bridge.mjs`

**文件**: [bridge.mjs](file:///workspace/shujian/bridge.mjs#L126)  
**风险**: 跨站请求伪造

```javascript
res.setHeader('Access-Control-Allow-Origin', '*');
```

允许任意域名访问 API 接口，恶意网站可以伪造请求。

**建议**: 将 `*` 替换为具体的允许域名列表。

---

### 3.2 WebSocket 无来源验证 — `bridge.mjs`

**文件**: [bridge.mjs](file:///workspace/shujian/bridge.mjs#L164-L202)  
**风险**: 未授权访问

WebSocket 服务器 (`wss`) 接受任何来源的连接，没有验证 `Origin` 头，也没有任何认证机制。任何知道地址的人都可以连接并发送游戏指令。

**建议**: 
1. 在 `wss.on('connection')` 中验证 `Origin` 头
2. 添加简单的 Token 认证机制

---

### 3.3 注册流程绕过 — `logind.c`

**文件**: [logind.c](file:///workspace/shujian/adm/daemons/logind.c#L684-L705)  
**风险**: 注册限制被绕过

`enter_world()` 函数中硬编码了两次 `user->set("registered",3)`：

```c
// 第684行
user->set("registered",3);
// 第705行
user->set("registered",3);
```

这导致所有新创建的角色自动绕过注册验证流程，直接获得完整游戏权限。

**建议**: 删除这些硬编码的 `set("registered",3)` 调用，恢复正常的注册流程。

---

### 3.4 Guest 账户硬编码密码 — `logind.c`

**文件**: [logind.c](file:///workspace/shujian/adm/daemons/logind.c#L188-L194)  
**风险**: 未授权访问

```c
if (arg == "guest") {
    ob->set("password", "SJGUEST");
    write("您的中文名字：");
    input_to("get_name", ob);
    return;
}
```

Guest 账户密码硬编码为 `"SJGUEST"`，且不经过任何密码验证即可创建角色。

**建议**: 为 Guest 账户设置随机密码，或完全禁用 Guest 账户创建功能。

---

### 3.5 密码暴力破解防护不足 — `logind.c`

**文件**: [logind.c](file:///workspace/shujian/adm/daemons/logind.c#L249-L261)  
**风险**: 暴力破解

密码错误计数器仅基于 IP 地址：

```c
if (query_temp("step1/"+ip_number) >= 3) {
    // 30秒锁定
    call_out("delete_temp", 30, "step1/"+ip_number);
}
```

攻击者可以通过更换 IP 地址绕过限制，且锁定时间仅 30 秒。

**建议**: 
1. 增加账户级别的失败计数
2. 延长锁定时间至 15 分钟以上
3. 添加指数退避机制

---

### 3.6 敏感信息日志泄露 — 多处

**文件**: [logind.c](file:///workspace/shujian/adm/daemons/logind.c#L289) 等  
**风险**: 密码明文记录

登录失败时将密码明文写入日志：

```c
log_file("USAGE", sprintf("%s 来自 %15s 的某人企图使用：%16s 登录 %s\n",
    ctime(time())[4..18], ip_number, pass, capitalize(""+ob->query("id"))));
```

`pass` 是用户输入的密码明文。如果管理员误操作或日志文件权限不当，用户密码可能泄露。

**建议**: 记录日志时不要包含密码明文，或仅记录密码的哈希值。

---

### 3.7 前端 XSS 风险 — `index.html`

**文件**: [index.html](file:///workspace/shujian/www/index.html#L854-L855)  
**风险**: 跨站脚本攻击

```javascript
function appendOutput(text) {
    var div = document.createElement('div');
    div.innerHTML = parseAnsi(text);  // innerHTML 直接插入解析后的内容
```

`parseAnsi()` 函数虽然调用了 `escHtml()` 对部分内容进行转义，但 ANSI 解析逻辑可能产生未转义的 HTML。NPC 名字、房间名等来自游戏服务器的数据直接插入 DOM。

**建议**: 
1. 使用 `textContent` 替代 `innerHTML` 用于纯文本内容
2. 实现更严格的 HTML 转义
3. 添加 Content-Security-Policy 头

---

## 四、中危风险（🟡）

### 4.1 Wiz Call 命令权限过宽 — `cmds/wiz/call.c`

**文件**: [call.c](file:///workspace/shujian/cmds/wiz/call.c#L8-L97)  
**风险**: 权限提升

`call` 命令允许巫师（wizard）调用任意对象上的任意函数。虽然对 `(apprentice)` 级别有限制，但 `(wizard)` 及以上级别可以调用大部分函数，且 `(admin)` 可以通过 `-euid` 标志提升权限。

**建议**: 添加函数调用白名单，限制危险函数（如 `destruct`、`rm`、`shutdown`）的调用。

---

### 4.2 前端重连无频率限制 — `index.html`

**文件**: [index.html](file:///workspace/shujian/www/index.html#L949-L957)  
**风险**: 资源耗尽

```javascript
function scheduleReconnect() {
    if (reconnectTimer) return;
    reconnectTimer = setTimeout(function() {
        reconnectTimer = null;
        connectWS();
    }, reconnectDelay);
    reconnectDelay = Math.min(reconnectDelay * 2, 15000);
}
```

虽然使用了指数退避，但没有设置最大重试次数。如果服务器长期不可用，客户端会无限重连。

**建议**: 添加最大重试次数（如 10 次），超过后停止自动重连并提示用户手动重试。

---

### 4.3 密码复杂度要求不足 — `logind.c`

**文件**: [logind.c](file:///workspace/shujian/adm/daemons/logind.c#L431-L468)  
**风险**: 弱密码

密码验证仅要求：(1) 至少 5 个字符，(2) 包含数字和大写字母。没有检查：
- 特殊字符
- 常见弱密码
- 连续或重复字符

**建议**: 增强密码强度要求，添加常见弱密码黑名单。

---

### 4.4 配置文件中的敏感信息 — `login.h`

**文件**: [login.h](file:///workspace/shujian/include/login.h#L14-L15)  
**风险**: 信息泄露

```c
#define SJ_EMAIL               "chuneer@citiz.com"
#define SJ_HOMEPAGE             "http://www.sjmud.com"
```

硬编码了管理员邮箱和网站地址，可能被用于社会工程攻击。

**建议**: 将敏感信息移至配置文件或环境变量。

---

### 4.5 ANSI 转义序列过滤被禁用 — `config.ini`

**文件**: [config.ini](file:///workspace/shujian/config.ini#L242-L253)  
**风险**: 终端注入

```ini
; no ansi : 1
; strip before process input : 1
```

两个安全选项均被注释掉（分号开头），意味着 ANSI 转义序列和特殊字符不会被过滤，可能导致终端注入攻击。

**建议**: 取消注释，启用 `no ansi` 和 `strip before process input`。

---

### 4.6 无 TLS/SSL 加密 — `config.ini`

**文件**: [config.ini](file:///workspace/shujian/config.ini#L22)  
**风险**: 中间人攻击

```ini
; external_port_2_tls : cert=cert.crt key=cert.key
```

WebSocket 端口不支持 TLS 加密，所有数据（包括密码）以明文传输。

**建议**: 配置 TLS 证书并启用加密连接。

---

### 4.7 Cloudflare Worker 默认配置不安全 — `deploy/worker.js`

**文件**: [worker.js](file:///workspace/shujian/deploy/worker.js#L18)  
**风险**: 服务暴露

```javascript
const backend = new WebSocket(`ws://${env.MUD_HOST || 'localhost'}:${env.MUD_PORT || '8080'}/ws`);
```

如果 `MUD_HOST` 和 `MUD_PORT` 环境变量未设置，默认连接到 `localhost:8080`，可能导致服务不可用或连接到错误的后端。

**建议**: 移除默认值，未配置时返回明确的错误信息。

---

### 4.8 前端输入验证不足 — `index.html`

**文件**: [index.html](file:///workspace/shujian/www/index.html#L965-L973)  
**风险**: 注入攻击

`sendCommand()` 函数直接发送用户输入，没有进行长度限制或内容过滤：

```javascript
function sendCommand(cmd) {
    ws.send(JSON.stringify({ type: 'cmd', cmd: cmd }));
}
```

**建议**: 添加命令长度限制（如 500 字符），过滤控制字符。

---

## 五、低危风险（🟢）

### 5.1 依赖版本未锁定 — `package.json`

**文件**: [package.json](file:///workspace/shujian/package.json)  
**风险**: 供应链攻击

```json
"iconv-lite": "^0.7.3",
"ws": "^8.21.1"
```

使用 `^` 前缀允许自动安装兼容的最新版本，可能导致意外引入有漏洞的依赖。

**建议**: 使用 `package-lock.json` 锁定依赖版本，定期运行 `npm audit`。

---

### 5.2 调试信息可能泄露 — `bbsd.c`

**文件**: [bbsd.c](file:///workspace/shujian/adm/daemons/bbsd.c#L131-L132)  
**风险**: 信息泄露

```c
if(debug) me = find_player("linux");
if(me) tell_object(me, sprintf("write(%d): %s\n", fd, str));
```

调试模式下将 SQL 语句和数据库操作详情发送给特定玩家 `"linux"`。

**建议**: 正式环境中禁用调试模式，或将调试信息写入日志文件而非发送给玩家。

---

### 5.3 服务端保活机制可被探测 — `bridge.mjs`

**文件**: [bridge.mjs](file:///workspace/shujian/bridge.mjs#L81-L86)  
**风险**: 信息泄露

每 15 秒发送 Telnet NOP 和 WebSocket Ping，可以被用于服务存活探测。

**建议**: 可接受的风险，但建议将间隔随机化。

---

### 5.4 多个玩家使用同一 IP 限制不严格 — `logind.c`

**文件**: [logind.c](file:///workspace/shujian/adm/daemons/logind.c#L99-L105)  
**风险**: 资源滥用

```c
login_cnt += query_ip_number(item) == ip;
if (login_cnt > 2) {
    destruct(ob);
    return;
}
```

同一 IP 只允许 2 个登录连接，但可通过代理绕过。

**建议**: 结合其他指纹信息（User-Agent、浏览器指纹）进行限制。

---

### 5.5 错误消息信息泄露 — 多处

多处 `notify_fail()` 调用返回了详细的错误信息，可能被攻击者利用进行信息收集。

**建议**: 生产环境中使用通用的错误消息，详细信息记录到日志。

---

### 5.6 静态文件服务缺少安全头 — `bridge.mjs`

**文件**: [bridge.mjs](file:///workspace/shujian/bridge.mjs#L138-L140)  
**风险**: 点击劫持、MIME 嗅探

```javascript
res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
```

缺少 `X-Content-Type-Options`、`X-Frame-Options`、`Content-Security-Policy` 等安全头。

**建议**: 添加常见安全响应头。

---

## 六、功能性 Bug

### Bug 1: valid_read() 权限检查失效（严重）

**文件**: [securityd.c](file:///workspace/shujian/adm/daemons/securityd.c#L179)  
**描述**: `return 1;` 语句导致所有文件读取权限检查被跳过。

---

### Bug 2: registered 状态被重复强制覆盖

**文件**: [logind.c](file:///workspace/shujian/adm/daemons/logind.c#L684-L705)  
**描述**: `user->set("registered",3)` 在 `enter_world()` 中被调用两次，且在第 684 行无条件设置，覆盖了之前的注册状态。

---

### Bug 3: 注销的注册流程代码未清理

**文件**: [logind.c](file:///workspace/shujian/adm/daemons/logind.c#L222-L236)  
**描述**: 大量被注释掉的旧代码（如数据库连接、社区注册等）未清理，影响代码可维护性。

---

### Bug 4: tohtml.c 中 ANSI 清理函数效率低

**文件**: [tohtml.c](file:///workspace/shujian/cmds/wiz/tohtml.c#L352-L366)  
**描述**: `remove_ansi()` 函数遍历所有 ANSI 颜色代码并逐个替换，对于大文本效率极低，且可能遗漏某些 ANSI 序列。

---

### Bug 5: 前端 lookExits 存在竞态条件

**文件**: [index.html](file:///workspace/shujian/www/index.html#L462-L471)  
**描述**: `lookExits()` 函数在收到新房间信息时自动触发 `look` 命令，如果玩家快速移动，可能导致多个 `look` 命令同时进行，造成状态混乱。

---

### Bug 6: bbsd.c 中注释代码包含安全漏洞

**文件**: [bbsd.c](file:///workspace/shujian/adm/daemons/bbsd.c#L254-L281)  
**描述**: 被注释掉的 `update_pool()` 函数包含大量 SQL 注入漏洞。如果将来取消注释，将直接引入安全风险。

---

### Bug 7: 密码修改后旧密码仍可登录

**文件**: [logind.c](file:///workspace/shujian/adm/daemons/logind.c#L285)  
**描述**: 第 285 行的 `oldcrypt` 兼容模式允许使用旧加密方式的密码登录，即使密码已被更新为 MD5 格式。

---

## 七、修复优先级建议

| 优先级 | 问题 | 修复工作量 |
|--------|------|-----------|
| P0 | 2.2 valid_read() 权限绕过 | 小（删除一行） |
| P0 | 2.4 GitHub Token 泄露 | 小（撤销 Token） |
| P0 | 2.1 硬编码后门用户 hongba | 小（删除配置） |
| P1 | 2.3 SQL 注入 | 大（需重构 SQL 查询） |
| P1 | 3.3 注册流程绕过 | 中（需恢复注册逻辑） |
| P1 | 3.2 WebSocket 无认证 | 中（需添加认证机制） |
| P1 | 3.6 密码明文日志 | 小（修改日志语句） |
| P2 | 3.1 CORS 配置 | 小（修改配置） |
| P2 | 3.7 前端 XSS | 中（需重构渲染逻辑） |
| P2 | 4.1 Wiz Call 权限 | 中（需添加白名单） |
| P3 | 其他中低危问题 | 按需修复 |

---

## 八、总结

本次审计发现该源码存在 **5 个严重安全漏洞**和 **7 个高危漏洞**，其中最紧迫的问题是：

1. **`valid_read()` 权限检查完全失效** — 任何用户可读取任意文件
2. **硬编码后门账户 `hongba`** — 拥有系统完全读写权限
3. **SQL 注入** — 玩家可控数据直接拼接到 SQL 语句中
4. **GitHub Token 泄露** — 有效凭证可能已在 Git 历史中暴露
5. **注册流程被绕过** — 硬编码的 `registered=3` 跳过所有注册验证

建议按照第七节的优先级排序立即开始修复工作。对于 SQL 注入问题，建议进行全面的代码审查，因为 `bbsd.c` 中还存在大量已注释但包含同类漏洞的代码。

---

*报告结束*