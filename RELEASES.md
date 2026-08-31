# 发布校验

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
- Terraria（AppID 105600）游戏下载和创意工坊内容下载

## 系统与游戏限制

启动器与 DLL 均为 32 位构建，因此可在 32 位 Windows 上运行；前提是该系统本身仍能运行目标旧版 Steam。

`steam_zstd.dll` 依赖 Universal CRT。Windows 7 需要具备相应的 UCRT/Visual C++ 运行库，才能加载该 DLL。

本补丁仅恢复新压缩格式内容的下载与解压。部分游戏仍可能因自身仅支持 64 位、要求较新 Windows 版本、依赖新版组件或图形 API 而无法启动。

## v1.0.0

首个正式运行包。