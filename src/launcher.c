#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>

#define STEAMCLIENT_ENTRY_RVA 0x00AF8110u
#define STEAMCLIENT_CAVE_RVA  0x0005F940u

static void fail(const char *message) {
    MessageBoxA(0, message, "Old Steam VSZa launcher", MB_OK | MB_ICONERROR);
}

static __declspec(noinline) void zero_bytes(void *target, DWORD count) {
    volatile BYTE *p = (volatile BYTE *)target;
    while (count--) *p++ = 0;
}

static void dirname_in_place(char *path) {
    char *p = path, *last = 0;
    while (*p) { if (*p == '\\' || *p == '/') last = p; ++p; }
    if (last) *last = 0;
}

static DWORD_PTR remote_module(DWORD pid, const char *wanted) {
    MODULEENTRY32 me;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    me.dwSize = sizeof(me);
    if (Module32First(snap, &me)) {
        do {
            if (lstrcmpiA(me.szModule, wanted) == 0) {
                DWORD_PTR result = (DWORD_PTR)me.modBaseAddr;
                CloseHandle(snap);
                return result;
            }
        } while (Module32Next(snap, &me));
    }
    CloseHandle(snap);
    return 0;
}

static int read_file(const char *path, BYTE *buffer, DWORD capacity, DWORD *size) {
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    DWORD got = 0;
    if (f == INVALID_HANDLE_VALUE) return 0;
    if (!ReadFile(f, buffer, capacity, &got, 0) || got == 0) {
        CloseHandle(f); return 0;
    }
    CloseHandle(f);
    *size = got;
    return 1;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command, int show) {
    char steam_dir[MAX_PATH], steam_exe[MAX_PATH], own_dir[MAX_PATH];
    char helper[MAX_PATH], hook_path[MAX_PATH], cmdline[MAX_PATH + 80];
    DWORD steam_dir_size = sizeof(steam_dir);
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    BYTE hook[0x6C0], original[9], jump[9] = {0xE9,0,0,0,0,0x90,0x90,0x90,0x90};
    const BYTE expected[9] = {0x55,0x8B,0xEC,0x81,0xEC,0x8C,0x02,0x00,0x00};
    DWORD hook_size, old_protect, written, remote_exit = 0;
    DWORD_PTR client_base, local_kernel, remote_kernel, remote_loadlibrary;
    SIZE_T path_len;
    LPVOID remote_path;
    HANDLE thread;
    int i;

    (void)instance; (void)previous; (void)command; (void)show;
    zero_bytes(&si, sizeof(si)); zero_bytes(&pi, sizeof(pi)); si.cb = sizeof(si);
    if (RegGetValueA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath",
                     RRF_RT_REG_SZ, 0, steam_dir, &steam_dir_size) != ERROR_SUCCESS) {
        fail("Steam installation path was not found in the registry."); return 1;
    }
    GetModuleFileNameA(0, own_dir, sizeof(own_dir)); dirname_in_place(own_dir);
    wsprintfA(steam_exe, "%s\\steam.exe", steam_dir);
    wsprintfA(helper, "%s\\steam_zstd.dll", own_dir);
    wsprintfA(hook_path, "%s\\vsza_hook.bin", own_dir);
    wsprintfA(cmdline, "\"%s\" -noverifyfiles -nobootstrapupdate", steam_exe);
    if (GetFileAttributesA(steam_exe) == INVALID_FILE_ATTRIBUTES ||
        GetFileAttributesA(helper) == INVALID_FILE_ATTRIBUTES ||
        !read_file(hook_path, hook, sizeof(hook), &hook_size)) {
        fail("steam.exe or patch files were not found."); return 2;
    }
    if (!CreateProcessA(steam_exe, cmdline, 0, 0, FALSE, 0, 0,
                        steam_dir, &si, &pi)) {
        fail("Steam could not be started. Try running this launcher as administrator."); return 3;
    }
    client_base = 0;
    for (i = 0; i < 30 && !client_base; ++i) {
        Sleep(500);
        client_base = remote_module(pi.dwProcessId, "steamclient.dll");
    }
    remote_kernel = remote_module(pi.dwProcessId, "kernel32.dll");
    local_kernel = (DWORD_PTR)GetModuleHandleA("kernel32.dll");
    if (!client_base || !remote_kernel ||
        !ReadProcessMemory(pi.hProcess, (LPCVOID)(client_base + STEAMCLIENT_ENTRY_RVA),
                           original, sizeof(original), 0)) goto patch_fail;
    for (i = 0; i < 9; ++i) if (original[i] != expected[i]) goto version_fail;

    remote_loadlibrary = remote_kernel + ((DWORD_PTR)GetProcAddress((HMODULE)local_kernel,
                         "LoadLibraryA") - local_kernel);
    path_len = (SIZE_T)lstrlenA(helper) + 1;
    remote_path = VirtualAllocEx(pi.hProcess, 0, path_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_path || !WriteProcessMemory(pi.hProcess, remote_path, helper, path_len, 0)) goto patch_fail;
    thread = CreateRemoteThread(pi.hProcess, 0, 0, (LPTHREAD_START_ROUTINE)remote_loadlibrary,
                                remote_path, 0, 0);
    if (!thread) goto patch_fail;
    WaitForSingleObject(thread, 15000);
    GetExitCodeThread(thread, &remote_exit);
    CloseHandle(thread);
    VirtualFreeEx(pi.hProcess, remote_path, 0, MEM_RELEASE);
    if (!remote_exit) goto helper_fail;

    if (!VirtualProtectEx(pi.hProcess, (LPVOID)(client_base + STEAMCLIENT_CAVE_RVA),
                          hook_size, PAGE_EXECUTE_READWRITE, &old_protect)) goto patch_fail;
    if (!WriteProcessMemory(pi.hProcess, (LPVOID)(client_base + STEAMCLIENT_CAVE_RVA),
                            hook, hook_size, &written) || written != hook_size) goto patch_fail;
    VirtualProtectEx(pi.hProcess, (LPVOID)(client_base + STEAMCLIENT_CAVE_RVA),
                     hook_size, old_protect, &written);
    *(LONG *)(jump + 1) = (LONG)(STEAMCLIENT_CAVE_RVA - (STEAMCLIENT_ENTRY_RVA + 5));
    if (!VirtualProtectEx(pi.hProcess, (LPVOID)(client_base + STEAMCLIENT_ENTRY_RVA),
                          sizeof(jump), PAGE_EXECUTE_READWRITE, &old_protect)) goto patch_fail;
    if (!WriteProcessMemory(pi.hProcess, (LPVOID)(client_base + STEAMCLIENT_ENTRY_RVA),
                            jump, sizeof(jump), &written) || written != sizeof(jump)) goto patch_fail;
    VirtualProtectEx(pi.hProcess, (LPVOID)(client_base + STEAMCLIENT_ENTRY_RVA),
                     sizeof(jump), old_protect, &written);
    FlushInstructionCache(pi.hProcess, 0, 0);
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    return 0;

version_fail:
    TerminateProcess(pi.hProcess, 1);
    fail("The loaded steamclient.dll is not the supported November 2024 build.");
    goto cleanup;
helper_fail:
    TerminateProcess(pi.hProcess, 1);
    fail("steam_zstd.dll could not be loaded. The Universal C Runtime may be missing.");
    goto cleanup;
patch_fail:
    TerminateProcess(pi.hProcess, 1);
    fail("The temporary memory patch could not be applied. No Steam file was changed.");
cleanup:
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    return 4;
}
