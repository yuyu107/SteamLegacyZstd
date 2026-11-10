# 32 位 Windows：Steam 更新后提示操作系统不受支持

[返回项目主页](../README.md) | [English](STEAM_32BIT_RECOVERY_EN.md)

> 适用于遇到相同症状时的故障排除说明；并非 Steam 官方修复方式。**Windows 8.1 32 位已实测成功，Windows 7 32 位可能类似，但目前未验证。**

## 出现了什么问题？

2026-11-11，在 Windows 8.1 **32 位**上，从较旧的 Steam 安装程序更新时，更新器首先请求普通 32 位更新清单 `steam_client_win32`（版本 `1769731672`）。下载和安装完成后，Steam 弹出：

> 您当前的操作系统版本不再支持 Steam。

此时 `bootstrap_log.txt` 可能只记录校验完成和退出，没有解释弹窗原因。这是 **Steam 安装 / 更新及客户端启动问题**，不是本项目的 Zstd 解压错误。

## 已实测的恢复步骤

操作前请先备份 Steam 安装目录中的重要数据。**不要删除 `steamapps`、游戏文件，或游戏目录内的同名 DLL。**

1. 完全退出 Steam，包括通知区域和后台残留的 `steam.exe`。
2. 打开 **Steam 安装目录根目录**，找到 `tier0_s.dll` 和 `tier0_s64.dll`。建议先将这两个文件移到 Steam 目录**之外**备份，使原位置缺失；如已备份，也可删除原文件。
3. 右键 Steam 快捷方式 → **属性**，在“目标”的 `steam.exe` 路径后添加一个空格和 `-no-cef-sandbox`，例如：

   ```text
   "C:\Program Files\Steam\steam.exe" -no-cef-sandbox
   ```

4. 通过修改后的快捷方式启动 Steam，等待它执行安装验证 / 更新。
5. 若成功打开 Steam，进入 **帮助 → 关于 Steam**，确认版本为 `1730853000`，再检查本项目要求的 `steamclient.dll` SHA-256 是否匹配：`d0e83c515f17ca57090c8c73664e5d61e37eae718dfa3a5cbb1e4b909548fc34`。**版本 / 哈希不符时不要强制应用 Zstd 补丁。**

本次恢复后，用户还在同一 Windows 8.1 **32 位**系统上实测了 **SteamLegacyZstd 补丁可用**。这项验证仅针对 Zstd 补丁及对应客户端，并不意味着所有游戏均兼容。

## 从日志确认恢复过程

在此次测试中，删除上述 DLL 并重新启动后，日志记录：

```text
BVerifyInstalledFiles: tier0_s.dll is -1 bytes, expected 341144
BVerifyInstalledFiles: tier0_s64.dll is -1 bytes, expected 429720
Downloading manifest: https://client-update.steamstatic.com/steam_client_legacy_win32
Downloaded new manifest: /steam_client_legacy_win32 version 1730853000, installed version 1769731672
```

更新完成后，Steam 启动成功，登录正常，并可以开始下载 Terraria。这表明此次**实际重新取得了旧版分支**，而不是仅隐藏错误弹窗。

但本次证据**无法分别证明**单独删除哪个 DLL，或者单独添加 `-no-cef-sandbox`，就必然会选择旧版分支。`-no-cef-sandbox` 用于禁用 CEF 沙盒以处理兼容性，**会降低隔离保护**，不是专门的更新分支选择开关；没有需要时不建议使用。

## Windows 7 32 位和其他系统

Windows 7 **32 位**可能出现类似的首次更新错误。如提示相同错误，可以在备份后参考上述操作，但**尚未在 Windows 7 32 位验证本流程**，不保证所有系统或后续 Steam 更新均有效。

如未成功，请保留 `Steam\logs\bootstrap_log.txt`、错误截图和 Steam 客户端版本；不要反复清理 Steam 文件或从来历不明的站点下载历史安装包。

其他兼容性限制请参见 [COMPATIBILITY.md](../COMPATIBILITY.md)。
