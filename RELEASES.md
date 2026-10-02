# 发布校验

## v1.1.0（待发布）

## 更新内容

- 同一启动器支持两种状态：Steam 未运行时自动启动后注入；Steam 已运行时直接注入。
- 修复注册表 SteamPath 与实际进程路径格式不同导致漏识别的问题；运行中注入不再依赖注册表路径。
- 启动阶段支持在等待期限内识别同安装位置的接替进程。
- 增加重复注入检测；注入失败不会主动关闭 Steam。
- 按 Windows 系统默认 UI 语言显示简体中文、繁体中文，其它语言仍为英文；错误阶段和成功提示同步翻译。
- 验证目标 DLL 的 SHA-256 和内存入口后再写入；兼容范围仍为指定 2024 年 11 月的 32 位 Steam DLL。

## 使用方法

完整解压，以管理员身份运行 `OldSteam_VSZa_Launcher.exe`。Steam 已打开时，首次注入前请暂停下载，成功后恢复下载。Steam 完全退出或重启后，需要再次运行。

保留禁用 GPU、禁用 CEF 沙盒及组合启动脚本。启动参数只在自动启动时生效，不会重启已打开的 Steam。

## 验证与限制

运行中注入已由用户完成 Terraria 下载，并通过 Steam 文件校验；源码通过严格交叉编译和 PE/包完整性检查。自动启动交接与各语言、系统组合尚未全面验证。

`steam_zstd.dll` 与 `vsza_hook.bin` 沿用 v1.0.2，未修改解压算法。不支持 2023 年 12 月客户端、新版 64 位客户端或任意历史 DLL。

下载末尾若提示“内容不可用”，且日志为 `No Download Source` / `No connection to content servers`，可保留进度、稍等再重试。若有 `Failed unpacking chunk`，请继续反馈日志。

## 下载校验

`SteamLegacyZstd_v1.1.0.zip` SHA-256：

```text
e2e7a29a1a7778795aae7a15bed661e1c2a993e723126151350caf00f50b40e1
```


## v1.0.1

本版仅更新运行包内说明；程序文件与 v1.0.0 相同。

运行包：`Steam_old_VSZa_compat_v1.0.1.zip`

SHA-256：

```text
0a1aacd2bac16bc52901d679f082bc3ede4d9f39b0835e12c855ddfd0f30da43
```

已验证范围：

- 2024-11-08 构建的 32 位 `steamclient.dll`
- Windows 7 和 Windows 8.1，32 位与 64 位系统
- Terraria（AppID 105600）游戏下载；Wallpaper Engine 创意工坊内容下载

## 系统与游戏限制

启动器与 DLL 均为 32 位构建，因此可在 32 位 Windows 上运行；前提是该系统本身仍能运行目标旧版 Steam。

`steam_zstd.dll` 依赖 Universal CRT。Windows 7 需要具备相应的 UCRT/Visual C++ 运行库，才能加载该 DLL。

本补丁仅恢复新压缩格式内容的下载与解压。部分游戏仍可能因自身仅支持 64 位、要求较新 Windows 版本、依赖新版组件或图形 API 而无法启动。

## v1.0.0

首个正式运行包。