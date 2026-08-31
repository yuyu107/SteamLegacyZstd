@echo off
setlocal EnableExtensions
set "ZIG=zig"
if not "%ZIG_PATH%"=="" set "ZIG=%ZIG_PATH%"

if not exist dist mkdir dist
%ZIG% cc -target x86-windows-gnu -O2 -ffreestanding -fno-builtin -fno-stack-protector -c src\launcher.c -o dist\launcher.o || goto :error
%ZIG% cc -target x86-windows-gnu -nostdlib -Wl,--subsystem,windows -Wl,--entry,WinMain@16 dist\launcher.o -lkernel32 -luser32 -ladvapi32 -o dist\OldSteam_VSZa_Launcher.exe || goto :error
%ZIG% cc -target x86-windows-gnu -O2 -shared -DZSTD_DISABLE_ASM -DZSTD_LEGACY_SUPPORT=0 -Ithird_party\zstd src\helper.c third_party\zstd\zstd.c src\steam_zstd.def -o dist\steam_zstd.dll || goto :error
as --32 src/vsza_hook.s -o dist\vsza_hook.o || goto :error
ld -m elf_i386 -Ttext 0x3805f940 -e _vsza_hook dist\vsza_hook.o -o dist\vsza_hook.elf || goto :error
objcopy -O binary --only-section=.text dist\vsza_hook.elf dist\vsza_hook.bin || goto :error
del /q dist\launcher.o dist\vsza_hook.o dist\vsza_hook.elf 2>nul
echo Build complete: dist
exit /b 0

:error
echo Build failed.
exit /b 1
