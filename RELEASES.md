# 发布校验

## v1.0.0

运行包：`Steam_old_VSZa_compat_v1.0.0.zip`

SHA-256：

```text
7bbeb9398cb59a68a60ce78bca096cead4059b374a4e6d70b5c1bb6a9613b5b2
```

已验证范围：2024-11-08 构建的 32 位 `steamclient.dll` 与 Windows 8.1；Terraria（AppID 105600）下载完成。

## Windows 7

启动器本身仅使用 Windows 7 已有的系统接口。但 `steam_zstd.dll` 依赖 Universal CRT；Windows 7 需先具备相应的 UCRT/Visual C++ 运行库，才能加载该 DLL。未安装时启动器会提示 DLL 无法加载。

尚未在干净的 Windows 7 环境完成端到端下载验证，因此当前将其标为“可尝试、未正式验证”，而非承诺兼容。
