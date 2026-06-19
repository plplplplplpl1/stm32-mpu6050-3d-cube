# Claude Code 配置清单

> 生成日期: 2026-06-11 | Claude Code v2.1.173 | Node v24.16.0

---

## 目录

1. [版本与环境](#1-版本与环境)
2. [模型配置](#2-模型配置)
3. [全局配置 (~/.claude/settings.json)](#3-全局配置-claudesettingsjson)
4. [用户本地配置 (~/.claude/settings.local.json)](#4-用户本地配置-claudesettingslocaljson)
5. [项目级配置 (.claude/settings.local.json)](#5-项目级配置-claudesettingslocaljson)
6. [MCP 服务器](#6-mcp-服务器)
7. [Skills 技能列表](#7-skills-技能列表)
8. [自定义命令 (Commands)](#8-自定义命令-commands)
9. [自定义代理 (Agents)](#9-自定义代理-agents)
10. [Hooks 钩子系统](#10-hooks-钩子系统)
11. [Status Line 状态栏](#11-status-line-状态栏)
12. [插件系统 (Plugins)](#12-插件系统-plugins)
13. [记忆系统 (Memory)](#13-记忆系统-memory)
14. [CLAUDE.md 项目指令](#14-claudemd-项目指令)
15. [键盘快捷键 (Keybindings)](#15-键盘快捷键-keybindings)
16. [定时任务 (Scheduled Tasks)](#16-定时任务-scheduled-tasks)
17. [应用级配置 (~/.claude.json)](#17-应用级配置-claudejson)
18. [已安装的全局包](#18-已安装的全局包)

---

## 1. 版本与环境

| 项目 | 值 |
|------|-----|
| Claude Code CLI | `@anthropic-ai/claude-code@2.1.173` (全局安装) |
| Node.js | `v24.16.0` |
| npm | `11.13.0` |
| 安装方式 | `global` (npm install -g) |
| 操作系统 | Windows 11 Home China 10.0.26200 |
| Shell | bash (Git Bash) |
| 终端 | 标准终端 (非 IDE 插件) |
| 主题 | `dark` |

---

## 2. 模型配置

**API 提供商**: DeepSeek (通过 Anthropic 兼容 API)

| 环境变量 | 值 |
|----------|-----|
| `ANTHROPIC_BASE_URL` | `https://api.deepseek.com/anthropic` |
| `ANTHROPIC_MODEL` | `deepseek-v4-pro[1m]` |
| `ANTHROPIC_DEFAULT_OPUS_MODEL` | `deepseek-v4-pro[1m]` |
| `ANTHROPIC_DEFAULT_SONNET_MODEL` | `deepseek-v4-pro[1m]` |
| `ANTHROPIC_DEFAULT_HAIKU_MODEL` | `deepseek-v4-pro[1m]` |
| `CLAUDE_CODE_SUBAGENT_MODEL` | `deepseek-v4-pro[1m]` |
| `ANTHROPIC_AUTH_TOKEN` | `sk-c831...` (已配置) |

> **说明**: 所有模型路由到 DeepSeek v4-pro，上下文窗口 1M tokens。未区分 Opus/Sonnet/Haiku 模型。

---

## 3. 全局配置 (~/.claude/settings.json)

```json
{
  "env": { /* 见上方模型配置 */ },
  "model": "deepseek-v4-pro[1m]",
  "permissions": {
    "deny": [
      "Bash(rm -rf /)", "Bash(rm -rf /*)",
      "Bash(rm -rf ~)", "Bash(rm -rf ~/*)",
      "Bash(shutdown *)", "Bash(reboot *)",
      "Bash(format *)", "Bash(mkfs.*)",
      "Bash(dd if=*)", "Bash(> /dev/sd*)",
      "Bash(git push --force origin main)",
      "Bash(git push --force origin master)",
      "Bash(git push -f origin main)",
      "Bash(git push -f origin master)"
    ],
    "defaultMode": "bypassPermissions"
  },
  "skipDangerousModePermissionPrompt": true,
  "autoCompactEnabled": true,
  "autoCompactWindow": 850000,
  "theme": "dark",
  "enabledPlugins": {}
}
```

**关键配置说明**:

| 配置项 | 值 | 说明 |
|--------|-----|------|
| `permissions.defaultMode` | `bypassPermissions` | 默认跳过权限询问 |
| `skipDangerousModePermissionPrompt` | `true` | 跳过危险模式警告 |
| `autoCompactEnabled` | `true` | 自动压缩上下文 |
| `autoCompactWindow` | `850000` | 上下文超过 850k tokens 触发压缩 |
| `enabledPlugins` | `{}` | 未启用任何插件 |

> ⚠️ **安全警示**: `bypassPermissions` + `skipDangerousModePermissionPrompt` 意味着所有操作默认允许执行，仅在 deny 列表中的命令被阻止。

---

## 4. 用户本地配置 (~/.claude/settings.local.json)

**权限白名单** (`permissions.allow`): **149 条规则**，涵盖：

- **Git 操作**: `git add`, `git commit`, `git push`, `git checkout`, `git rm`, `git config`
- **Python 执行**: `python3 *`, `python *`, `py *`
- **文件读取**: `Read(/d/*)`, `Read(//c/tmp/**)`, `Read(//c/Program Files/**)`
- **文件搜索**: `Glob(/d/*)`
- **Shell 操作**: `curl`, `grep`, `cat`, `find`, `ls`, `perl`, `awk`, `sort`, `xxd`
- **Node.js 执行**: `node *`, `npx *`
- **包管理**: `pip install`, `npm install`, `uv sync`
- **PowerShell**: `git *`, `gh auth`, `winget install`
- **Web 访问**: `WebSearch`, `WebFetch` (多个域名白名单)
- **MCP 工具**: `mcp__playwright__*`, `mcp__jlcpcb__component_search`
- **其他工具**: `ffmpeg`, `magick/convert`, `tasklist`, `taskkill`

**额外目录** (`permissions.additionalDirectories`):
```json
["/d/", "D:/"]
```

**启用的 MCP 服务器** (`enabledMcpjsonServers`):
```json
["embedded-serial", "embedded-tools"]
```

---

## 5. 项目级配置 (.claude/settings.local.json)

路径: `.claude/settings.local.json` (项目根目录)

```json
{
  "permissions": {
    "allow": [
      "Bash(git add *)",
      "Bash(git commit *)",
      "Bash(git push *)",
      "Bash(python3 *)",
      "Bash(python Tools/gen_hyperbolic.py)",
      "Bash(python -c ' *)",
      "Bash(python Tools/gen_4d_cell.py)",
      "Bash(dir /b /s *.c)",
      "Bash(python \"Tools/gen_4d_cell.py\")",
      "Bash(attrib -r Start/startup_stm32f10x_md.s)",
      "Bash(attrib +r Start/startup_stm32f10x_md.s)",
      "Bash(git checkout *)"
    ]
  }
}
```

> 仅包含项目所需的 Git/Python 操作白名单，无其他配置项。

---

## 6. MCP 服务器

配置文件: `~/.claude/mcp.json`

| 服务器名称 | 命令 | 用途 | 状态 |
|-----------|------|------|------|
| `embedded-serial` | `embedded-serial-mcp` v0.0.7 | 嵌入式串口通信 | ✅ 已启用 |
| `embedded-tools` | `embedded-mcp-server` v1.0.0 | 嵌入式工具集 | ✅ 已启用 |
| `windows-control` | `terminator-mcp-agent` v0.24.28 | Windows 系统控制 | ❌ 未启用 |

---

## 7. Skills 技能列表

共 **37 个技能**。`~/.claude/skills/` 下 28 个内置 + 9 个符号链接到 `~/.agents/skills/`。

### 7.1 项目开发技能

| 技能名称 | 描述 |
|----------|------|
| `build-keil` | Keil MDK 命令行编译，解析工程文件、执行构建、定位固件产物 |
| `debug-gdb-openocd` | OpenOCD + GDB 在线调试、固件下载、崩溃现场检查 |
| `flash-openocd` | OpenOCD 烧录嵌入式固件 |
| `serial-monitor` | 串口识别、日志抓取、运行状态分析 |
| `memory-analysis` | 解析 .map/ELF 获取内存使用报告、符号大小排名 |
| `stm32-hal-coding` | STM32 HAL/LL 外设驱动生成 (GPIO/UART/I2C/SPI/ADC/PWM/DMA/Timer) |
| `stm32-hal-development` | CubeMX HAL 项目固件开发指导 |
| `implementing-firmware` | STM32/ESP-IDF/PlatformIO 固件实现，含需求标签和 MISRA-C lint |

### 7.2 质量保证技能

| 技能名称 | 描述 |
|----------|------|
| `adversarial-review` | 对抗性代码审查 — 找漏洞、安全问题、设计缺陷 |
| `code-simplifier` | 代码简化 — 去重、死代码、冗余抽象 (不改行为) |
| `systematic-debugging` | 4 阶段系统调试: 复现→假设→实验→修复+回归 |
| `test-driven-development` | TDD 红-绿-重构循环 |
| `verification-before-completion` | 完成前验证 — 运行测试、检查边界、验证输出 |
| `tracing-requirements` | 验证 SYS→PRD→SW 需求可追溯性链 |
| `checking-compliance` | CE/FCC/UL 合规要求生成 |

### 7.3 规划设计技能

| 技能名称 | 描述 |
|----------|------|
| `brainstorming` | 代码库探索、多方案设计、权衡评估 |
| `writing-plans` | 多阶段实施计划，含清晰阶段/文件列表/检查点 |
| `planning-with-files` | 基于文件的持久化任务规划 (task_plan.md / findings.md / progress.md) |
| `ralph-loop` | 防早停循环 — 复杂任务确保所有阶段完成 |

### 7.4 PCB/硬件技能

| 技能名称 | 描述 |
|----------|------|
| `auditing-pcb-design` | KiCad/Altium DRC/ERC 检查、Gerber/BOM 导出 |

### 7.5 Agent Stack 管理技能

| 技能名称 | 描述 |
|----------|------|
| `stack-bootstrap` | 初始化/重新同步 agent-stack 优化层 |
| `stack-doctor` | 检查 agent-stack 健康状态 (token 预算、SKILL.md 限制、hook 冲突) |
| `stack-graph-profile` | 检查/切换代码图谱后端和配置文件 |
| `stack-handoff` | 写/恢复会话交接文件 (跨会话工作持续性) |
| `stack-measure` | 测量 token 使用和节省量 |

### 7.6 Caveman 压缩系列 (符号链接到 ~/.agents/skills/)

| 技能名称 | 描述 |
|----------|------|
| `caveman` | 超压缩通信模式 — 省 ~75% token (lite/full/ultra/wenyan) |
| `caveman-commit` | 超压缩提交信息生成 (Conventional Commits) |
| `caveman-compress` | 压缩自然语言记忆文件 (CLAUDE.md/todos 等) |
| `caveman-help` | Caveman 模式快速参考 |
| `caveman-review` | 超压缩代码审查评论 |
| `caveman-stats` | 显示会话 token 使用和节省统计 |
| `cavecrew` | 洞穴人子代理调度决策指南 |

### 7.7 非技能型斜杠命令 (Commands)

| 命令 | 文件 | 描述 |
|------|------|------|
| `stack-audit` | `~/.claude/commands/stack-audit.md` | 运行 `agent-stack audit` 并汇总 token 报告 |
| `stack-handoff` | `~/.claude/commands/stack-handoff.md` | 运行 `agent-stack handoff write/resume` |
| `stack-measure` | `~/.claude/commands/stack-measure.md` | 运行 `agent-stack measure --since 7d` |

---

## 8. 自定义命令 (Commands)

路径: `~/.claude/commands/`

共 **3 个自定义命令**，均为 agent-stack 相关：

| 命令 | 功能 |
|------|------|
| `/stack-audit` | 运行 `npx @drmahdikazempour/agent-stack audit` 并汇总 token 报告 |
| `/stack-handoff` | 运行 `npx @drmahdikazempour/agent-stack handoff write/resume` 管理会话交接 |
| `/stack-measure` | 运行 `npx @drmahdikazempour/agent-stack measure --since 7d` 测量 token 使用 |

---

## 9. 自定义代理 (Agents)

路径: `~/.claude/agents/`

| 代理名称 | 描述 | 可用工具 |
|----------|------|----------|
| `stack-explorer` | 只读代码库探索代理，通过代码图谱查询，返回结论而非文件转储 | Read, Grep, Glob, Bash |

> 行为: 只读、结论导向、精简输出以节省主上下文预算。

---

## 10. Hooks 钩子系统

配置于 `~/.claude/settings.json` 的 `hooks` 字段:

### Stop Hook — token 使用记录

```json
{
  "hooks": {
    "Stop": [{
      "hooks": [{
        "type": "command",
        "command": "ccusage --json >> .agent-stack/usage.jsonl 2>/dev/null || true # agent-stack"
      }]
    }]
  }
}
```

| 属性 | 值 |
|------|-----|
| 触发时机 | 每次会话停止 (Stop) |
| 执行命令 | `ccusage --json` (记录 token 使用) |
| 输出目标 | `.agent-stack/usage.jsonl` (JSONL 追加) |
| 错误处理 | `|| true` — 失败不中断 |
| 来源 | `# agent-stack` 标识 |

> **注意**: 项目当前没有 `.agent-stack/` 目录，该 hook 会静默失败 (输出到 `/dev/null`)。

---

## 11. Status Line 状态栏

配置于 `~/.claude/settings.json`:

```json
{
  "statusLine": {
    "type": "command",
    "command": "/d/python/python.exe \"$HOME/.claude/statusline.py\"",
    "refreshInterval": 2
  }
}
```

| 属性 | 值 |
|------|-----|
| 类型 | `command` (自定义脚本) |
| Python 解释器 | `D:\python\python.exe` |
| 脚本路径 | `~/.claude/statusline.py` |
| 刷新间隔 | 2 秒 |

### Status Line 显示内容

通过 `statusline.py` 脚本实时渲染：

| 显示项 | 说明 |
|--------|------|
| **模型名称** | `# deepseek-v4-pro[1m]` |
| **上下文窗口** | 百分比 + 颜色编码 (>70% 红色, >40% 黄色, ≤40% 绿色) |
| **输入 tokens** | 累计输入 token 数 |
| **输出 tokens** | 累计输出 token 数 |
| **缓存命中** | 累计缓存读取 token 数 |
| **预估费用** | USD (按 DeepSeek 定价: 输入 $0.28/M, 输出 $0.42/M, 缓存 $0.028/M) |
| **会话时长** | 从首条消息起计时 |
| **当前路径** | 截短显示 (保留末 2 级) |
| **Git 分支** | 当前分支名 |
| **当前时间** | HH:MM:SS |

---

## 12. 插件系统 (Plugins)

### 12.1 插件市场

| 市场 | 来源 | 最后更新 |
|------|------|----------|
| `claude-plugins-official` | `anthropics/claude-plugins-official` (GitHub) | 2026-06-10 |

### 12.2 官方可用插件 (35 个)

**开发工具类**: `agent-sdk-dev`, `clangd-lsp`, `typescript-lsp`, `pyright-lsp`, `rust-analyzer-lsp`, `gopls-lsp`, `jdtls-lsp`, `kotlin-lsp`, `lua-lsp`, `ruby-lsp`, `swift-lsp`, `php-lsp`, `csharp-lsp`

**工作流类**: `code-review`, `code-simplifier`, `code-modernization`, `commit-commands`, `feature-dev`, `frontend-design`, `pr-review-toolkit`, `ralph-loop`, `hookify`

**项目管理类**: `claude-code-setup`, `claude-md-management`

**MCP 相关**: `mcp-server-dev`, `mcp-tunnels`

**专用领域**: `cwc-makers`, `math-olympiad`, `security-guidance`

**其他**: `example-plugin`, `explanatory-output-style`, `learning-output-style`, `playground`, `plugin-dev`, `session-report`, `skill-creator`

### 12.3 外部集成插件 (15 个)

`asana`, `context7`, `discord`, `fakechat`, `firebase`, `github`, `gitlab`, `greptile`, `imessage`, `laravel-boost`, `linear`, `playwright`, `serena`, `telegram`, `terraform`

### 12.4 当前启用状态

```json
"enabledPlugins": {}
```

> **目前未启用任何插件**。

---

## 13. 记忆系统 (Memory)

### 13.1 项目记忆

路径: `C:\Users\30374\.claude\projects\D--stm32--------10-1---I2C--MPU6050\memory\`

| 状态 | 说明 |
|------|------|
| **当前状态** | 空目录 — 无记忆文件，无 MEMORY.md 索引 |
| **格式** | Markdown 文件 + YAML frontmatter (`name`, `description`, `metadata`) |
| **类型** | `user` / `feedback` / `project` / `reference` |

### 13.2 其他项目的记忆

| 项目 slug | 说明 |
|-----------|------|
| `C--Users-30374` | 用户主目录项目 |
| `D---------` | D 盘项目 (slug 编码) |
| `D--stm32--------10-1---I2C--MPU6050` | 当前项目 (空) |

### 13.3 用户级记忆

路径: `C:\Users\30374\.claude\projects\C--Users-30374\memory\`

> 存在于其他项目目录中，但当前项目 memory 为空。

---

## 14. CLAUDE.md 项目指令

路径: `CLAUDE.md` (项目根目录)

**文件大小**: ~107 行 Markdown

### 核心内容速览

| 章节 | 关键信息 |
|------|----------|
| **项目概述** | STM32F103C8T6 + MPU6050 + SSD1306 OLED 实时 3D 姿态显示，支持 31 种多面体形状 |
| **构建系统** | Keil MDK uVision 5, ARMCC V5.06, -O1, C99, USE_STDPERIPH_DRIVER |
| **双 I2C 总线** | 硬件 I2C1 (PB6/PB7 → OLED), 软件 I2C MyI2C (PB10/PB11 → MPU6050) |
| **W25Q64 Flash** | 8MB NOR Flash, 软件 SPI (PB12-15), 分区布局定义 |
| **猫动画** | 28 帧 GIF 动画系统, W25Q64 实时读取 |
| **形状生成器** | Python 脚本生成 4D 多胞体/双曲镶嵌数据 |
| **OLED 刷新** | 水平寻址模式防撕裂方案 |
| **中文字库** | HZK16 行优先格式, SimSun 12pt |
| **多动画系统** | 可切换动画播放器 |
| **已知陷阱** | W25Q64 DO/DI 标签、软件 SPI 面包板、MPU6050 时钟源等 |

---

## 15. 键盘快捷键 (Keybindings)

| 状态 |
|------|
| **未配置** — `~/.claude/keybindings.json` 不存在 |

> 使用 Claude Code 默认键盘快捷键。可用 `/keybindings-help` 或 `keybindings-help` 技能自定义。

---

## 16. 定时任务 (Scheduled Tasks)

配置文件: `~/.claude/scheduled_tasks.json`

```json
{"tasks": []}
```

> **当前无活跃定时任务**。任务支持 durable (持久化到磁盘) 和 session-only (会话内) 模式。

---

## 17. 应用级配置 (~/.claude.json)

关键非历史配置项:

| 配置项 | 值 | 说明 |
|--------|-----|------|
| `installMethod` | `global` | npm 全局安装 |
| `numStartups` | `100` | 累计启动次数 |
| `firstStartTime` | `2026-04-28` | 首次使用日期 |
| `migrationVersion` | `13` | 配置迁移版本 |
| `lastOnboardingVersion` | `2.1.121` | 最后新手引导版本 |
| `lastReleaseNotesSeen` | `2.1.172` | 最后查看的发布说明 |
| `hasCompletedOnboarding` | `true` | 已完成新手引导 |
| `promptQueueUseCount` | `125` | 提示队列使用次数 |
| `btwUseCount` | `2` | 后台任务使用次数 |
| `hasSeenTasksHint` | `true` | 已看过任务提示 |

---

## 18. 已安装的全局包

| 包名 | 版本 | 用途 |
|------|------|------|
| `@anthropic-ai/claude-code` | 2.1.173 | Claude Code CLI 主体 |
| `@vividcodeai/embedded-serial-mcp` | 0.0.7 | 嵌入式串口 MCP 服务器 |
| `embedded-mcp-server` | 1.0.0 | 嵌入式工具 MCP 服务器 |
| `terminator-mcp-agent` | 0.24.28 | Windows 控制 MCP 代理 |
| `@xpack-dev-tools/arm-none-eabi-gcc` | 15.2.1-1.1.1 | ARM GCC 交叉编译器 |
| `tesseract.js` | 7.0.0 | OCR 引擎 |
| `xpm` | 0.23.2 | xPack 包管理器 |

---

## 附录：配置文件路径速查

| 文件 | 路径 | 层级 |
|------|------|------|
| 全局设置 | `~/.claude/settings.json` | 用户全局 |
| 本地设置 | `~/.claude/settings.local.json` | 用户本地覆盖 |
| 项目设置 | `.claude/settings.local.json` | 项目级 |
| MCP 配置 | `~/.claude/mcp.json` | 用户全局 |
| 应用状态 | `~/.claude.json` | 用户全局 |
| 定时任务 | `~/.claude/scheduled_tasks.json` | 用户全局 |
| 状态栏脚本 | `~/.claude/statusline.py` | 用户全局 |
| 技能目录 | `~/.claude/skills/` | 用户全局 |
| 命令目录 | `~/.claude/commands/` | 用户全局 |
| 代理目录 | `~/.claude/agents/` | 用户全局 |
| 插件目录 | `~/.claude/plugins/` | 用户全局 |
| Caveman 技能 | `~/.agents/skills/` | 用户全局 |
| 项目指令 | `CLAUDE.md` | 项目根目录 |
| 项目记忆 | `~/.claude/projects/<slug>/memory/` | 项目级 |
