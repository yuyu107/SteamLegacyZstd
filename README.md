# SteamLegacyZstd

[简体中文](README.md) | [English](README_EN.md)

为 Windows 7 / 8.1 上的指定旧版 Steam 客户端添加 SteamPipe `VSZa`（Zstandard）数据块解压支持。该目标版本是 Steam 结束对 Windows 7 / 8.x 支持前，按常规更新渠道通常会保留或安装到的最后一代兼容客户端，并非任意选择的历史版本。

> [!IMPORTANT]
> 截至 v1.0.2 发布时，Windows 7 / 8.x 用户仍可直接从 Steam 官网下载安装程序，并正常安装、更新到本补丁所支持的客户端版本 `1730853000`，无需另外寻找来历不明的旧版 Steam 安装包。

目标 Steam 程序本体为 32 位，但可在 32 位和 64 位 Windows 7 / 8.1 上使用。

## 已验证范围

- Steam 客户端版本：`1730853000`
- `steamclient.dll`：2024-11-08 构建
- `steamclient.dll` SHA-256：`d0e83c515f17ca57090c8c73664e5d61e37eae718dfa3a5cbb1e4b909548fc34`
- 系统：Windows 7、Windows 8.1（32 位与 64 位系统均支持；32 位系统尚未实机验证）
- 已验证内容：Terraria（AppID 105600）游戏下载；Wallpaper Engine 创意工坊内容下载

不支持当前新版 64 位 Steam 客户端、`steamclient64.dll`，以及其他更早、更晚或 Beta 分支的 Steam。详细范围见 [COMPATIBILITY.md](COMPATIBILITY.md)。

## 重要限制

本项目只解决旧 Steam 无法解压新压缩格式内容的问题，**不会让旧系统或旧版 Steam 获得运行所有游戏的能力**。部分游戏仍可能因为仅提供 64 位程序、要求 Windows 10/11、依赖新版 Web 组件、图形 API 或其他运行库而无法启动；这些限制与本补丁无关。

## 使用方法

从 [Releases](../../releases) 下载并完整解压全部文件，完全退出 Steam 后选择合适的入口：

- 普通用户：运行 `OldSteam_VSZa_Launcher.exe`
- Steam 黑屏、界面卡住或加载缓慢：运行 `Launch_with_gpu_disabled.cmd`
- 原 Steam 本来就必须使用 `-no-cef-sandbox`：运行 `Launch_with_no_cef_sandbox.cmd`
- 同时需要以上两种参数：运行 `Launch_with_gpu_disabled_and_no_cef_sandbox.cmd`

`-cef-disable-gpu` 只禁用 Steam UI（CEF）的 GPU 加速，不影响游戏使用显卡。`-no-cef-sandbox` 会关闭 Steam UI 的 CEF 浏览器沙盒、降低隔离保护，普通系统不要使用。

启动器会把自身收到的其他附加命令行参数原样转交给 Steam。

## Steam 安装目录如何识别

启动器不扫描磁盘，也不读取 Steam 账号、密码或游戏文件。它只读取 Steam 在当前用户注册表中保存的安装目录：

```text
HKEY_CURRENT_USER\\Software\\Valve\\Steam\\SteamPath
```

随后使用该路径启动其中的 `steam.exe`。如果此注册表项不存在或路径无效，启动器会显示找不到 Steam 安装目录的提示。

## 工作方式与安全软件提示

启动器等待旧 Steam 加载目标 `steamclient.dll`，在内存中将其压缩分发函数跳转到兼容分支。该分支仅识别 `VSZa` 包装；其余旧格式仍交还给原始 Steam 代码。`steam_zstd.dll` 使用 Zstandard 解压数据，并检查 Steam 包装中的 CRC32 与输出长度。

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
