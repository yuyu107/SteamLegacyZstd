# Steam VSZa compatibility patch

为 Windows 8.1 上的指定旧版 32 位 Steam 客户端添加 SteamPipe `VSZa`（Zstandard）数据块解压支持。

## 已验证范围

- Steam `steamclient.dll`：2024-11-08 构建，SHA-256：`d0e83c515f17ca57090c8c73664e5d61e37eae718dfa3a5cbb1e4b909548fc34`
- 系统：Windows 8.1 x86/x64
- 已验证游戏：Terraria（AppID 105600）

它不会替换或修改硬盘上的 `steamclient.dll`、`steamui.dll` 或 `steam.exe`。启动器只在本次 Steam 进程的内存中加入分支；退出 Steam 后该变化自动消失。

## 使用

从 Release 下载并解压全部文件，完全退出 Steam 后运行 `OldSteam_VSZa_Launcher.exe`。不要直接用原 `steam.exe` 启动需要此兼容功能的下载或更新。

## 构建

需要 Zig 0.16 或兼容版本，以及 GNU binutils（`as`、`ld`、`objcopy`）。另需将 Zstandard 的合并源码 `zstd.c`、`zstd.h`、`zstd_errors.h` 放到 `third_party/zstd`；此依赖未直接纳入仓库，来源和许可证见该目录说明。

准备依赖后，在仓库根目录运行：

```bat
build.cmd
```

生成的三个运行文件位于 `dist`：启动器、`steam_zstd.dll` 与 `vsza_hook.bin`。

## 工作方式

启动器等待旧 Steam 加载目标 `steamclient.dll`，在内存中将其压缩分发函数跳转到一个兼容分支。该分支仅识别 `VSZa` 包装；其余旧格式仍交还给原始 Steam 代码。`steam_zstd.dll` 用 Zstandard 解压帧，并检查 Steam 包装中的 CRC32 与输出长度。

## 许可与第三方组件

本项目代码采用 [MIT License](LICENSE)。

本项目使用 Zstandard；该依赖遵循其自身的 BSD 许可证，见 `third_party/zstd/LICENSE`。

Steam、SteamPipe 与游戏名称是各自权利人的商标或财产。本项目与 Valve 无关联，也不提供 Steam 或游戏文件。
