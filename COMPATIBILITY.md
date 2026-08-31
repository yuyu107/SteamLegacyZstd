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

已实测：Windows 7 和 Windows 8.1，32 位与 64 位系统。

`steam_zstd.dll` 依赖 Universal CRT；Windows 7 需要具备相应的 UCRT/Visual C++ 运行库。

## 功能范围

已验证：Terraria（AppID 105600）游戏下载与创意工坊内容下载。

本项目只修复 SteamPipe 新 VSZa/Zstandard 压缩块的下载与解压。它不解决游戏本体的系统要求、64 位要求、图形 API、Web 组件或其他运行库问题。
