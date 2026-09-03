# SteamLegacyZstd

[简体中文](README.md) | [English](README_EN.md)

Adds SteamPipe `VSZa` (Zstandard) chunk decompression support to the last supported legacy Steam client for Windows 7 and Windows 8.1. The targeted build is the client that these systems normally retained or received through the regular update channel when Steam ended support for Windows 7/8.x; it is not an arbitrarily selected historical build.

> [!IMPORTANT]
> As of the v1.0.2 release, Windows 7/8.x users can still download the installer from the official Steam website and install or update normally to the supported client version `1730853000`. There is no need to obtain an old Steam installer from an untrusted source.

The targeted Steam client is a 32-bit application, which is normal. It can run on both 32-bit and 64-bit editions of Windows 7/8.1.

## Verified scope

- Steam client version: `1730853000`
- `steamclient.dll` build date: 2024-11-08
- `steamclient.dll` SHA-256: `d0e83c515f17ca57090c8c73664e5d61e37eae718dfa3a5cbb1e4b909548fc34`
- Operating systems: Windows 7 and Windows 8.1 (both x86 and x64 are supported; x86 Windows has not yet been tested on real hardware)
- Verified downloads: Terraria (AppID 105600) game content and Wallpaper Engine Workshop content

The current 64-bit Steam client, `steamclient64.dll`, Beta clients, and any earlier or later Steam builds are not supported. See [COMPATIBILITY.md](COMPATIBILITY.md) for the exact compatibility scope.

## Important limitation

This project only fixes the legacy client's inability to decompress content that uses the newer format. It **does not make every game compatible with an old Steam client or an old version of Windows**. A game may still fail to launch because it is 64-bit only, requires Windows 10/11, or depends on newer web components, graphics APIs, or runtimes.

## Usage

Download the package from [Releases](../../releases), extract every file, and fully exit Steam. Then choose the appropriate launcher:

- Normal use: run `OldSteam_VSZa_Launcher.exe`
- Steam shows a black screen, freezes, or loads its UI very slowly: run `Launch_with_gpu_disabled.cmd`
- Your existing Steam installation already requires `-no-cef-sandbox`: run `Launch_with_no_cef_sandbox.cmd`
- Both workarounds are required: run `Launch_with_gpu_disabled_and_no_cef_sandbox.cmd`

`-cef-disable-gpu` only disables GPU acceleration for the Steam UI (CEF); it does not disable GPU acceleration in games. `-no-cef-sandbox` disables the CEF sandbox and reduces process isolation, so do not use it on systems that do not require it.

Any other command-line arguments supplied to the launcher are passed through to Steam unchanged.

## How the Steam installation is located

The launcher does not scan disks or read Steam account credentials or game files. It only reads the Steam installation path stored for the current user in the Windows Registry:

```text
HKEY_CURRENT_USER\\Software\\Valve\\Steam\\SteamPath
```

It then starts `steam.exe` from that directory. If the registry value is missing or points to an invalid location, the launcher displays an error instead.

## How it works and antivirus notice

The launcher waits for the targeted `steamclient.dll` to load, then redirects its compression dispatcher to a compatibility branch in process memory. That branch handles only the `VSZa` wrapper; all older formats continue through Steam's original code. `steam_zstd.dll` decompresses the Zstandard frame and validates the wrapped CRC32 and output length.

No on-disk `steamclient.dll`, `steamui.dll`, or `steam.exe` file is replaced or modified. The patch disappears when Steam exits. Because the launcher performs cross-process memory operations, some security products may flag it heuristically. Download it only from this project's official Releases page and verify the published SHA-256 checksum.

## Building

Building requires Zig 0.16 or a compatible version and GNU binutils (`as`, `ld`, and `objcopy`). Place the Zstandard amalgamated sources (`zstd.c`, `zstd.h`, and `zstd_errors.h`) in `third_party/zstd`; see the files in that directory for source and licensing information.

From the repository root, run:

```bat
build.cmd
```

The launcher, `steam_zstd.dll`, and `vsza_hook.bin` are written to `dist`.

## License and third-party components

Project code is licensed under the [MIT License](LICENSE). Zstandard is distributed under its own BSD license; see `third_party/zstd/LICENSE`.

Steam, SteamPipe, and game names are trademarks or property of their respective owners. This project is not affiliated with Valve and does not distribute Steam or game files.
