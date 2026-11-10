# SteamLegacyZstd

[简体中文](README.md) | [English](README_EN.md)

为 Windows 7 / 8.1 上的指定旧版 Steam 客户端添加 SteamPipe `VSZa`（Zstandard）数据块解压支持。该目标版本是 Steam 结束对 Windows 7 / 8.x 支持前，按常规更新渠道通常会保留或安装到的最后一代兼容客户端，并非任意选择的历史版本。

> [!IMPORTANT]
> Windows 7 / 8.x 用户可以从 Steam 官网获取安装程序，无需寻找来历不明的旧版客户端。但**不能保证首次安装就会正确选择旧版分支**：2026-11-11 的 Windows 8.1 32 位实测曾先下载到不兼容的 `steam_client_win32`（`1769731672`），出现“您当前的操作系统版本不再支持 Steam”。如遇此问题，参见下方的 32 位系统故障排除。

目标 Steam 程序本体为 32 位，但可在 32 位和 64 位 Windows 7 / 8.1 上使用。

## 已验证范围

- Steam 客户端版本：`1730853000`
- `steamclient.dll`：2024-11-08 构建
- `steamclient.dll` SHA-256：`d0e83c515f17ca57090c8c73664e5d61e37eae718dfa3a5cbb1e4b909548fc34`
- 系统：Windows 7、Windows 8.1（目标为 32 位 Steam 客户端，适用于 32/64 位 Windows；本次已在 Windows 8.1 32 位实测客户端回退启动、登录和开始下载 Terraria，**不等于此次验证了 Zstd 注入**）
- 已验证内容：Terraria（AppID 105600）游戏下载；Wallpaper Engine 创意工坊内容下载

不支持当前新版 64 位 Steam 客户端、`steamclient64.dll`，以及其他更早、更晚或 Beta 分支的 Steam。详细范围见 [COMPATIBILITY.md](COMPATIBILITY.md)。

## 重要限制

本项目只解决旧 Steam 无法解压新压缩格式内容的问题，**不会让旧系统或旧版 Steam 获得运行所有游戏的能力**。部分游戏仍可能因为仅提供 64 位程序、要求 Windows 10/11、依赖新版 Web 组件、图形 API 或其他运行库而无法启动；这些限制与本补丁无关。

## 32 位 Windows 安装 / 更新后提示系统不支持

**已观察到的情况（Windows 8.1 32 位，2026-11-11）：** 从较旧的 Steam 安装程序启动后，更新器先请求 `steam_client_win32`（版本 `1769731672`），安装完成却提示“您当前的操作系统版本不再支持 Steam”，客户端无法打开。这是**客户端安装 / 更新问题，不是 Zstd 解压失败**。

该环境下曾通过以下方法恢复到本补丁支持的旧版 Steam（不是所有 Windows 版本都保证有效）：

1. **彻底退出 Steam**，包括托盘图标和残留的 `steam.exe` 进程。建议先备份 Steam 安装目录中的重要数据；**不要删除 `steamapps` 或游戏文件**。
2. 仅在 **Steam 安装目录根目录**找到 `tier0_s.dll` 与 `tier0_s64.dll`，将它们移到 Steam 目录**之外**备份（或在已有备份后删除）；不要操作同名的游戏目录文件。
3. 编辑 Steam 快捷方式，在 `steam.exe` 路径后添加 `-no-cef-sandbox`，然后使用该快捷方式启动 Steam。例如：`"C:\Program Files\Steam\steam.exe" -no-cef-sandbox`。
4. 在此次实测中，启动器检测到两个 DLL 缺失后重新验证、下载，改为请求 `steam_client_legacy_win32`（版本 `1730853000`），随后旧版 Steam 成功打开。
5. 在 Steam 的“帮助 → 关于 Steam”核对客户端版本为 `1730853000`。**确认目标 `steamclient.dll` 的 SHA-256 与本项目要求一致后**，才能使用本项目的 Zstd 补丁；版本不符时不要强行注入。

`-no-cef-sandbox` 会关闭 CEF 沙盒、降低隔离保护，只应在确有需要时使用。它**不是**选择旧版更新分支的专用参数；此次日志只能证明“文件缺失触发修复后选中了 legacy 分支”，不能据此认定单独删除某一个 DLL 或单独添加参数必然有效。

若仍然报错，请保留 `Steam/logs/bootstrap_log.txt`、错误截图和“关于 Steam”的版本信息反馈；不要反复删除文件或使用未知来源的历史安装包。

## 使用方法

从 [Releases](../../releases) 下载并完整解压全部文件，右键以管理员身份运行合适的入口。Steam 已打开时会直接注入；未打开时会自动启动后注入：

- 普通用户：运行 `OldSteam_VSZa_Launcher.exe`
- Steam 黑屏、界面卡住或加载缓慢：运行 `Launch_with_gpu_disabled.cmd`
- 原 Steam 本来就必须使用 `-no-cef-sandbox`：运行 `Launch_with_no_cef_sandbox.cmd`
- 同时需要以上两种参数：运行 `Launch_with_gpu_disabled_and_no_cef_sandbox.cmd`

`-cef-disable-gpu` 只禁用 Steam UI（CEF）的 GPU 加速，不影响游戏使用显卡。`-no-cef-sandbox` 会关闭 Steam UI 的 CEF 浏览器沙盒、降低隔离保护，普通系统不要使用。

首次对已运行的 Steam 注入前，请暂停下载，避免在游戏运行时首次注入。成功后可恢复下载。重复运行会提示当前进程已应用相同补丁；Steam 完全退出或重启后需要再次运行启动器。

启动参数只在自动启动 Steam 时生效；已打开的 Steam 不会被重启以修改参数。自动启动时，启动器会把自身收到的其他附加命令行参数原样转交给 Steam。

### 自动界面语言

v1.1.0 根据 Windows 系统默认 UI 语言（`GetSystemDefaultUILanguage`）显示提示：简体中文（中国大陆、新加坡）、繁体中文（台湾、香港、澳门）；其它语言保持英文。成功、错误和失败阶段均使用 Unicode 弹窗。地区和键盘设置不会影响此判断。

### 下载末尾偶发“内容不可用”

如果 `logs/content_log.txt` 显示 `No Download Source` 或 `No connection to content servers`，保留下载进度，稍等后重试；本次用户测试曾重新获取下载源后补齐剩余分块并通过校验。若出现 `Failed unpacking chunk`，则仍需排查解压，不要把所有“内容不可用”都视为连接问题。

## Steam 安装目录如何识别

启动器先枚举正在运行的 `steam.exe`，查询其实际路径，并验证已加载 `steamclient.dll` 对应文件的 SHA-256 和内存入口。不会读取 Steam 账号、密码或游戏内容。发现多个 `steam.exe` 时不会猜测目标。

Steam 未运行时，读取当前用户注册表中的安装目录：

```text
HKEY_CURRENT_USER\Software\Valve\Steam\SteamPath
```

随后启动该目录中的 `steam.exe`。启动阶段若初始进程退出，会在 60 秒等待期限内查找同一安装位置的接替进程。若注册表路径缺失或无效，可以先正常打开 Steam，再运行本启动器。

## 工作方式与安全软件提示

启动器等待旧 Steam 加载目标 `steamclient.dll`，在内存中将其压缩分发函数跳转到兼容分支。写入入口时会短暂暂停并恢复目标线程。注入失败不会主动终止 Steam。该分支仅识别 `VSZa` 包装；其余旧格式仍交还给原始 Steam 代码。`steam_zstd.dll` 使用 Zstandard 解压数据，并检查 Steam 包装中的 CRC32 与输出长度。

它不会替换或修改硬盘上的 `steamclient.dll`、`steamui.dll` 或 `steam.exe`；退出 Steam 后内存变化自动消失。由于程序需要使用跨进程内存操作，部分安全软件可能启发式报毒。请只从本项目官方 Release 下载并核对发布页提供的 SHA-256。

## 构建

需要 Zig 0.16 或兼容版本，以及 GNU binutils（`as`、`ld`、`objcopy`）。另需将 Zstandard 的合并源码 `zstd.c`、`zstd.h`、`zstd_errors.h` 放到 `third_party/zstd`；来源和许可证见该目录说明。

准备依赖后，在仓库根目录运行：

```bat
build.cmd
```

生成的三个运行文件位于 `dist`：启动器、`steam_zstd.dll` 与 `vsza_hook.bin`。

## 许可与第三方组件

本项目代码采用 [MIT License](LICENSE)。Zstandard 依赖遵循其自身的 BSD 许可证，见 `third_party/zstd/LICENSE`。

Steam、SteamPipe 与游戏名称是各自权利人的商标或财产。本项目与 Valve 无关联，也不提供 Steam 或游戏文件。
