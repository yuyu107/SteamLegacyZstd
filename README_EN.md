# SteamLegacyZstd

[简体中文](README.md) | [English](README_EN.md)

Adds SteamPipe `VSZa` (Zstandard) chunk decompression support to the last supported legacy Steam client for Windows 7 and Windows 8.1. The targeted build is the client that these systems normally retained or received through the regular update channel when Steam ended support for Windows 7/8.x; it is not an arbitrarily selected historical build.

> [!IMPORTANT]
> Windows 7/8.x users can obtain the installer from Steam's official website instead of untrusted legacy-client mirrors. However, **the first installation does not always choose the legacy update channel**: a Windows 8.1 32-bit test on 2026-11-11 initially received `steam_client_win32` (`1769731672`) and failed with an unsupported-operating-system error. See the 32-bit troubleshooting section below.

The targeted Steam client is a 32-bit application, which is normal. It can run on both 32-bit and 64-bit editions of Windows 7/8.1.

## Verified scope

- Steam client version: `1730853000`
- `steamclient.dll` build date: 2024-11-08
- `steamclient.dll` SHA-256: `d0e83c515f17ca57090c8c73664e5d61e37eae718dfa3a5cbb1e4b909548fc34`
- Operating systems: Windows 7 and Windows 8.1 (the target 32-bit Steam client is intended for x86/x64 Windows; recovery, sign-in, and the start of a Terraria download were observed on Windows 8.1 x86; **this did not independently validate Zstd injection on x86**)
- Verified downloads: Terraria (AppID 105600) game content and Wallpaper Engine Workshop content

The current 64-bit Steam client, `steamclient64.dll`, Beta clients, and any earlier or later Steam builds are not supported. See [COMPATIBILITY.md](COMPATIBILITY.md) for the exact compatibility scope.

## Important limitation

This project only fixes the legacy client's inability to decompress content that uses the newer format. It **does not make every game compatible with an old Steam client or an old version of Windows**. A game may still fail to launch because it is 64-bit only, requires Windows 10/11, or depends on newer web components, graphics APIs, or runtimes.

## Unsupported-OS error after installing/updating on 32-bit Windows

**Observed on Windows 8.1 x86 (2026-11-11):** An older Steam installer/updater first fetched `steam_client_win32` (version `1769731672`), then showed “Your current operating system version is no longer supported by Steam.” Steam would not open. This is an **installation/update-channel issue, not a Zstd decompression failure**.

The following recovery worked in that one test; it is **not guaranteed** for every Windows build:

1. Fully exit Steam, including tray/background `steam.exe` processes. Back up important Steam installation data. **Do not remove `steamapps` or game files.**
2. In the **Steam installation root only**, move `tier0_s.dll` and `tier0_s64.dll` **outside the Steam directory** as backups (or remove them after backing them up). Do not touch similarly named files inside games.
3. Append `-no-cef-sandbox` to a shortcut targeting `steam.exe` and launch Steam through it. Example: `"C:\Program Files\Steam\steam.exe" -no-cef-sandbox`.
4. In the observed test, detecting the missing DLLs triggered verification/repair and the updater requested `steam_client_legacy_win32` (version `1730853000`). Steam then opened.
5. Check **Help → About Steam** for client version `1730853000`. Only apply this project's Zstd patch after verifying that `steamclient.dll` matches the required SHA-256. **Never force injection on an unmatched build.**

`-no-cef-sandbox` disables CEF process sandboxing and reduces isolation; use it only when necessary. It is **not** a dedicated legacy-channel selector. The logs demonstrate a legacy-channel download after the missing-file repair, but do not prove which step alone caused the channel switch.

If it still fails, preserve `Steam/logs/bootstrap_log.txt`, the error screenshot, and the About Steam version details. Avoid repeatedly deleting files or downloading historical clients from untrusted sites.

## Usage

Download the package from [Releases](../../releases), extract every file, then run the appropriate launcher as administrator. It attaches if Steam is already running, or starts Steam before patching if it is closed:

- Normal use: run `OldSteam_VSZa_Launcher.exe`
- Steam shows a black screen, freezes, or loads its UI very slowly: run `Launch_with_gpu_disabled.cmd`
- Your existing Steam installation already requires `-no-cef-sandbox`: run `Launch_with_no_cef_sandbox.cmd`
- Both workarounds are required: run `Launch_with_gpu_disabled_and_no_cef_sandbox.cmd`

`-cef-disable-gpu` only disables GPU acceleration for the Steam UI (CEF); it does not disable GPU acceleration in games. `-no-cef-sandbox` disables the CEF sandbox and reduces process isolation, so do not use it on systems that do not require it.

Pause downloads before first attaching to a running Steam process, and avoid first injection while a game is running. Resume downloads after success. Repeated runs recognize the same installed patch. Run the launcher again after Steam fully exits or restarts.

Launch arguments only apply when starting Steam. An already-running client is not restarted to change its arguments. Additional arguments are passed through unchanged when Steam is started.

### Automatic UI language

v1.1.0 uses the Windows system default UI language (`GetSystemDefaultUILanguage`): Simplified Chinese for China/Singapore, Traditional Chinese for Taiwan/Hong Kong/Macau, and English for other languages. Success messages, errors and failure stages use Unicode dialogs. Region and keyboard settings do not control this selection.

### Occasional content-unavailable errors near completion

If `logs/content_log.txt` reports `No Download Source` or `No connection to content servers`, keep the downloaded data and retry after a short wait. A user test completed and verified the remaining chunks after refreshing sources. `Failed unpacking chunk` still warrants decompression investigation; not all content-unavailable errors are connection failures.

## How the Steam installation is located

The launcher enumerates running `steam.exe` processes, queries the actual executable path, and checks the loaded `steamclient.dll` file SHA-256 and memory entry before patching. It does not read account credentials or game content. It refuses to guess when multiple Steam processes are present.

If Steam is closed, it reads the current user's registered installation path:

```text
HKEY_CURRENT_USER\Software\Valve\Steam\SteamPath
```

It then starts `steam.exe` there. If the initial process exits during startup, the launcher looks for a replacement from the same installation within its 60-second wait period. If registration is missing or invalid, open Steam normally and run the launcher again.

## How it works and antivirus notice

The launcher waits for the targeted `steamclient.dll` to load, then redirects its compression dispatcher to a compatibility branch in process memory. Target threads are briefly paused and resumed while the entry is written. An injection failure does not intentionally terminate Steam. That branch handles only the `VSZa` wrapper; all older formats continue through Steam's original code. `steam_zstd.dll` decompresses the Zstandard frame and validates the wrapped CRC32 and output length.

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
