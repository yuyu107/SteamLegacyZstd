# 兼容性范围

## 仅支持的 Steam 版本

本项目面向 Windows 7 / Windows 8.x 在 Steam 结束支持前，按常规更新渠道通常会保留或安装到的**最后一代兼容 Steam 客户端程序**。它不是任意挑选的旧版 Steam；目标版本为：

| 项目 | 值 |
| --- | --- |
| `steamclient.dll` 文件日期 | 2024-11-08 |
| Steam 客户端版本 | `1730853000` |
| `steamclient.dll` SHA-256 | `d0e83c515f17ca57090c8c73664e5d61e37eae718dfa3a5cbb1e4b909548fc34` |

该 Steam 程序本体为 32 位，但这**不等于只能在 32 位 Windows 上使用**：

- 32 位 Windows：安装上述指定旧版 Steam 后可以使用；
- 64 位 Windows：同样可以使用，只要安装的是上述指定旧版 Steam；
- 不支持当前的 64 位 Steam 客户端，也不支持 `steamclient64.dll`。

请不要将它用于更早版本、其他 2024 版本、更晚版本或 Steam Beta 客户端。这些版本的内部函数位置和数据结构可能不同；即使启动器能够打开 Steam，也不代表该版本受支持。

## 系统范围

已实测：Windows 7 和 Windows 8.1 的 64 位系统。目标 Steam 客户端也可运行于 32 位 Windows。2026-11-11 在 **Windows 8.1 32 位**实测通过恢复 `steam_client_legacy_win32`（`1730853000`）成功启动、登录并开始下载 Terraria；**随后 Zstd 补丁也已在该 Windows 8.1 32 位系统上实测可用**；Windows 7 32 位的本次恢复流程尚未实测。

注意：Windows 8.1 32 位已实测；Windows 7 32 位可能出现类似问题但尚未验证。从较旧的 Steam 安装程序安装时可能先收到 `steam_client_win32`（`1769731672`），继而提示“您当前的操作系统版本不再支持 Steam”。一次成功的恢复方式是：退出 Steam，备份并移走 **Steam 根目录**的 `tier0_s.dll` 和 `tier0_s64.dll`，为 Steam 快捷方式添加 `-no-cef-sandbox`，再启动触发完整性修复；实测随后下载了 `steam_client_legacy_win32`（`1730853000`）。这并非所有系统必然适用的解决办法；操作前请备份，不要删除游戏文件。详情见 [README 的 32 位系统故障排除](README.md#32-位-windows-安装--更新后提示系统不支持)。

`steam_zstd.dll` 依赖 Universal CRT；Windows 7 需要具备相应的 UCRT/Visual C++ 运行库。

## 功能范围

已验证：Terraria（AppID 105600）游戏下载；Wallpaper Engine 创意工坊内容下载。

本项目只修复 SteamPipe 新 VSZa/Zstandard 压缩块的下载与解压。它不解决游戏本体的系统要求、64 位要求、图形 API、Web 组件或其他运行库问题。

## v1.1.0 启动与语言

同一入口支持自动启动和运行中注入。运行中注入已有本会话用户完成 Terraria 下载并通过校验。自动启动交接与各语言/系统组合尚未全面验证，不扩大 Steam DLL 支持范围。

简体/繁体中文提示按系统默认 UI 语言选择，其它语言使用英文。
