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

32 位 Windows 8.1 可能在首次安装 / 更新 Steam 后遇到“操作系统不再支持 Steam”提示；Windows 7 32 位是否出现相同问题仍待验证。恢复步骤及验证记录请见 [32 位 Windows Steam 安装 / 更新故障排除](docs/STEAM_32BIT_RECOVERY.md)。

`steam_zstd.dll` 依赖 Universal CRT；Windows 7 需要具备相应的 UCRT/Visual C++ 运行库。

## 功能范围

已验证：Terraria（AppID 105600）游戏下载；Wallpaper Engine 创意工坊内容下载。

本项目只修复 SteamPipe 新 VSZa/Zstandard 压缩块的下载与解压。它不解决游戏本体的系统要求、64 位要求、图形 API、Web 组件或其他运行库问题。

## v1.1.0 启动与语言

同一入口支持自动启动和运行中注入。运行中注入已有本会话用户完成 Terraria 下载并通过校验。自动启动交接与各语言/系统组合尚未全面验证，不扩大 Steam DLL 支持范围。

简体/繁体中文提示按系统默认 UI 语言选择，其它语言使用英文。
