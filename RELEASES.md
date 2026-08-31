# 发布校验

## v1.0.0

运行包：`Steam_old_VSZa_compat_v1.0.0.zip`

SHA-256：

```text
7bbeb9398cb59a68a60ce78bca096cead4059b374a4e6d70b5c1bb6a9613b5b2
```

已验证范围：

- 2024-11-08 构建的 32 位 `steamclient.dll`
- Windows 7 和 Windows 8.1，32 位与 64 位系统
- Terraria（AppID 105600）游戏下载和创意工坊内容下载

## 系统与游戏限制

启动器与 DLL 均为 32 位构建，因此可在 32 位 Windows 上运行；前提是该系统本身仍能运行目标旧版 Steam。

`steam_zstd.dll` 依赖 Universal CRT。Windows 7 需要具备相应的 UCRT/Visual C++ 运行库，才能加载该 DLL。

本补丁仅恢复新压缩格式内容的下载与解压。部分游戏仍可能因自身仅支持 64 位、要求较新 Windows 版本、依赖新版组件或图形 API 而无法启动。
