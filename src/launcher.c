#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <wincrypt.h>

#define STEAMCLIENT_ENTRY_RVA 0x00AF8110u
#define STEAMCLIENT_CAVE_RVA  0x0005F940u

static HANDLE frozen[1024];
static DWORD frozen_count;
static void thaw(void) {
    while (frozen_count) { HANDLE h = frozen[--frozen_count]; ResumeThread(h); CloseHandle(h); }
}
static int freeze(DWORD pid, DWORD_PTR entry, DWORD_PTR cave, DWORD hook_size) {
    THREADENTRY32 te;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    te.dwSize = sizeof(te);
    if (!Thread32First(snap, &te)) { CloseHandle(snap); return 0; }
    do {
        if (te.th32OwnerProcessID == pid) {
            HANDLE h; CONTEXT ctx;
            if (frozen_count == 1024) { SetLastError(ERROR_TOO_MANY_TCBS); goto bad; }
            h = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, te.th32ThreadID);
            if (!h) goto bad;
            if (SuspendThread(h) == (DWORD)-1) { CloseHandle(h); goto bad; }
            frozen[frozen_count++] = h;
            ctx.ContextFlags = CONTEXT_CONTROL;
            if (!GetThreadContext(h, &ctx)) goto bad;
            if ((ctx.Eip >= entry && ctx.Eip < entry + 9) ||
                (ctx.Eip >= cave && ctx.Eip < cave + hook_size)) {
                SetLastError(ERROR_BUSY); goto bad;
            }
        }
    } while (Thread32Next(snap, &te));
    CloseHandle(snap);
    return frozen_count != 0;
bad:
    { DWORD e = GetLastError(); CloseHandle(snap); thaw(); SetLastError(e); }
    return 0;
}
static int supported_file(const char *path) {
    static const BYTE wanted[32] = {
        0xd0,0xe8,0x3c,0x51,0x5f,0x17,0xca,0x57,0x09,0x0c,0x8c,0x73,0x66,0x4e,0x5d,0x61,
        0xe3,0x7e,0xae,0x71,0x8d,0xfa,0x3a,0x5c,0xbb,0x1e,0x4b,0x90,0x95,0x48,0xfc,0x34};
    HCRYPTPROV provider = 0; HCRYPTHASH hash = 0;
    HANDLE f; static BYTE data[8192]; BYTE digest[32]; DWORD got, size = 32; int ok = 0, i;
    f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (f == INVALID_HANDLE_VALUE) return 0;
    if (!CryptAcquireContextA(&provider, 0, 0, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) ||
        !CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash)) goto done;
    for (;;) {
        if (!ReadFile(f, data, sizeof(data), &got, 0)) goto done;
        if (!got) break;
        if (!CryptHashData(hash, data, got, 0)) goto done;
    }
    if (!CryptGetHashParam(hash, HP_HASHVAL, digest, &size, 0) || size != 32) goto done;
    for (i = 0; i < 32; ++i) if (digest[i] != wanted[i]) goto done;
    ok = 1;
done:
    if (hash) CryptDestroyHash(hash);
    if (provider) CryptReleaseContext(provider, 0);
    CloseHandle(f); return ok;
}
/* Resolve the actual process, independently of SteamPath spelling or stale
   registration. The loaded DLL hash and entry are checked before mutation. */
static char selected_path[MAX_PATH];
static DWORD find_running(DWORD *error) {
    PROCESSENTRY32 pe; DWORD pid = 0, count = 0; HANDLE snap;
    *error = 0;
    snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) { *error = GetLastError(); return 0; }
    pe.dwSize = sizeof(pe);
    if (Process32First(snap, &pe)) do {
        if (lstrcmpiA(pe.szExeFile, "steam.exe") == 0) {
            ++count; pid = pe.th32ProcessID;
        }
    } while (Process32Next(snap, &pe));
    CloseHandle(snap);
    if (count > 1) { *error = ERROR_MORE_DATA; return 0; }
    if (count == 1) {
        DWORD len = sizeof(selected_path);
        HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!h) { *error = GetLastError(); return 0; }
        if (!QueryFullProcessImageNameA(h, 0, selected_path, &len)) {
            *error = GetLastError(); CloseHandle(h); return 0;
        }
        CloseHandle(h); return pid;
    }
    return 0;
}
static int loaded_client_path(DWORD pid, char *path) {
    MODULEENTRY32 me; HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    me.dwSize = sizeof(me);
    if (Module32First(snap, &me)) do {
        if (!lstrcmpiA(me.szModule, "steamclient.dll")) {
            lstrcpyA(path, me.szExePath); CloseHandle(snap); return 1;
        }
    } while (Module32Next(snap, &me));
    CloseHandle(snap); return 0;
}

/* UTF-8 source; wide strings keep Chinese text independent of ANSI code page. */
static int ui_language;
static void detect_ui_language(void) {
    LANGID id = GetSystemDefaultUILanguage();
    ui_language = 0;
    if (PRIMARYLANGID(id) != LANG_CHINESE) return;
    switch (SUBLANGID(id)) {
        case SUBLANG_CHINESE_SIMPLIFIED:
        case SUBLANG_CHINESE_SINGAPORE: ui_language = 1; break;
        case SUBLANG_CHINESE_TRADITIONAL:
        case SUBLANG_CHINESE_HONGKONG:
        case SUBLANG_CHINESE_MACAU: ui_language = 2; break;
        default: break;
    }
}
typedef struct { const char *key; const WCHAR *en, *sc, *tc; } UiText;
static const UiText ui_texts[] = {
    {"unknown", L"unknown", L"未知", L"未知"},
    {"Patch files were not found. Extract all files together.", L"Patch files were not found. Extract all files together.", L"找不到补丁文件。请完整解压所有文件到同一文件夹。", L"找不到修補檔案。請完整解壓縮所有檔案至同一資料夾。"},
    {"Cannot inspect Steam. Run this launcher as administrator.", L"Cannot inspect Steam. Run this launcher as administrator.", L"无法读取 Steam 进程信息。请以管理员身份运行本程序。", L"無法讀取 Steam 處理程序資訊。請以系統管理員身分執行本程式。"},
    {"Multiple steam.exe processes were found. Wait for startup to finish, then retry.", L"Multiple steam.exe processes were found. Wait for startup to finish, then retry.", L"发现多个 steam.exe 进程。请等待启动完成后重试。", L"發現多個 steam.exe 處理程序。請等待啟動完成後重試。"},
    {"query running Steam executable path", L"query running Steam executable path", L"查询正在运行的 Steam 路径", L"查詢正在執行的 Steam 路徑"},
    {"open running Steam (try administrator)", L"open running Steam (try administrator)", L"打开正在运行的 Steam 进程（请尝试管理员权限）", L"開啟正在執行的 Steam 處理程序（請嘗試系統管理員權限）"},
    {"Steam installation path was not found. Open Steam normally, then run this launcher.", L"Steam installation path was not found. Open Steam normally, then run this launcher.", L"找不到 Steam 安装路径。请先正常打开 Steam，再运行本程序。", L"找不到 Steam 安裝路徑。請先正常開啟 Steam，再執行本程式。"},
    {"Registered steam.exe was not found. Open Steam normally, then run this launcher.", L"Registered steam.exe was not found. Open Steam normally, then run this launcher.", L"注册表中记录的 steam.exe 不存在。请先正常打开 Steam，再运行本程序。", L"登錄資料中記錄的 steam.exe 不存在。請先正常開啟 Steam，再執行本程式。"},
    {"Steam command line is too long.", L"Steam command line is too long.", L"Steam 启动参数过长。", L"Steam 啟動參數過長。"},
    {"start Steam (try administrator)", L"start Steam (try administrator)", L"启动 Steam（请尝试管理员权限）", L"啟動 Steam（請嘗試系統管理員權限）"},
    {"inspect replacement Steam (try administrator)", L"inspect replacement Steam (try administrator)", L"读取接替的 Steam 进程（请尝试管理员权限）", L"讀取接替的 Steam 處理程序（請嘗試系統管理員權限）"},
    {"replacement Steam belongs to a different installation", L"replacement Steam belongs to a different installation", L"接替的 Steam 进程来自另一个安装位置", L"接替的 Steam 處理程序來自另一個安裝位置"},
    {"open replacement Steam", L"open replacement Steam", L"打开接替的 Steam 进程", L"開啟接替的 Steam 處理程序"},
    {"query Steam process state", L"query Steam process state", L"查询 Steam 进程状态", L"查詢 Steam 處理程序狀態"},
    {"Steam exited before steamclient.dll loaded", L"Steam exited before steamclient.dll loaded", L"Steam 在加载 steamclient.dll 前退出", L"Steam 在載入 steamclient.dll 前結束"},
    {"inspect Steam modules (run as administrator)", L"inspect Steam modules (run as administrator)", L"读取 Steam 模块（请以管理员身份运行）", L"讀取 Steam 模組（請以系統管理員身分執行）"},
    {"wait for steamclient.dll (60 second timeout)", L"wait for steamclient.dll (60 second timeout)", L"等待 steamclient.dll 加载（60 秒超时）", L"等待 steamclient.dll 載入（60 秒逾時）"},
    {"locate kernel32.dll", L"locate kernel32.dll", L"查找 kernel32.dll", L"尋找 kernel32.dll"},
    {"read steamclient.dll memory", L"read steamclient.dll memory", L"读取 steamclient.dll 内存", L"讀取 steamclient.dll 記憶體"},
    {"This Steam process is already patched.", L"This Steam process is already patched.", L"当前 Steam 进程已应用此补丁，无需重复注入。", L"目前的 Steam 處理程序已套用此修補，無須重複注入。"},
    {"resolve LoadLibraryA owner", L"resolve LoadLibraryA owner", L"确定 LoadLibraryA 所属模块", L"確認 LoadLibraryA 所屬模組"},
    {"locate loader owner in Steam", L"locate loader owner in Steam", L"查找 Steam 中的加载函数所属模块", L"尋找 Steam 中載入函式所屬的模組"},
    {"allocate helper path in Steam", L"allocate helper path in Steam", L"在 Steam 中分配辅助 DLL 路径内存", L"在 Steam 中配置輔助 DLL 路徑記憶體"},
    {"write helper path into Steam", L"write helper path into Steam", L"将辅助 DLL 路径写入 Steam", L"將輔助 DLL 路徑寫入 Steam"},
    {"start helper loader in Steam", L"start helper loader in Steam", L"在 Steam 中启动辅助 DLL 加载线程", L"在 Steam 中啟動輔助 DLL 載入執行緒"},
    {"wait for helper loader", L"wait for helper loader", L"等待辅助 DLL 加载完成", L"等待輔助 DLL 載入完成"},
    {"read helper loader result", L"read helper loader result", L"读取辅助 DLL 加载结果", L"讀取輔助 DLL 載入結果"},
    {"pause Steam threads safely (pause downloads and retry)", L"pause Steam threads safely (pause downloads and retry)", L"安全暂停 Steam 线程（请暂停下载后重试）", L"安全暫停 Steam 執行緒（請暫停下載後重試）"},
    {"recheck entry", L"recheck entry", L"再次检查补丁入口", L"再次檢查修補入口"},
    {"change hook memory protection", L"change hook memory protection", L"修改兼容代码区域的内存保护", L"修改相容程式碼區域的記憶體保護"},
    {"write compatibility hook", L"write compatibility hook", L"写入兼容代码", L"寫入相容程式碼"},
    {"change entry memory protection", L"change entry memory protection", L"修改入口区域的内存保护", L"修改入口區域的記憶體保護"},
    {"activate compatibility hook", L"activate compatibility hook", L"启用兼容代码入口", L"啟用相容程式碼入口"},
    {"Patch applied to the running Steam process. You may resume downloads.\r\nApply again after Steam fully exits or restarts.", L"Patch applied to the running Steam process. You may resume downloads.\r\nApply again after Steam fully exits or restarts.", L"已成功为当前 Steam 进程应用补丁，可以恢复下载。\r\nSteam 完全退出或重启后，需要重新运行本程序。", L"已成功為目前的 Steam 處理程序套用修補，可以繼續下載。\r\nSteam 完全結束或重新啟動後，需要再次執行本程式。"},
    {"The loaded steamclient.dll is not the supported November 2024 build.", L"The loaded steamclient.dll is not the supported November 2024 build.", L"已加载的 steamclient.dll 不是本补丁支持的 2024 年 11 月构建，或入口已被其它补丁修改。", L"已載入的 steamclient.dll 不是本修補支援的 2024 年 11 月組建，或入口已被其他修補修改。"},
    {"steam_zstd.dll could not be loaded. The Universal C Runtime may be missing.", L"steam_zstd.dll could not be loaded. The Universal C Runtime may be missing.", L"无法加载 steam_zstd.dll，可能缺少通用 C 运行库（UCRT）。", L"無法載入 steam_zstd.dll，可能缺少通用 C 執行階段（UCRT）。"},
};
static const WCHAR *ui_text(const char *key) {
    DWORD i;
    for (i = 0; i < sizeof(ui_texts) / sizeof(ui_texts[0]); ++i)
        if (!lstrcmpA(key, ui_texts[i].key))
            return ui_language == 1 ? ui_texts[i].sc : ui_language == 2 ? ui_texts[i].tc : ui_texts[i].en;
    return L"Unknown";
}
static void info(const char *message) {
    MessageBoxW(0, ui_text(message), L"Steam VSZa launcher", MB_OK | MB_ICONINFORMATION);
}
static void fail(const char *message) {
    MessageBoxW(0, ui_text(message), L"Steam VSZa launcher", MB_OK | MB_ICONERROR);
}

static void fail_stage(const char *stage, DWORD error) {
    WCHAR message[768];
    const WCHAR *format = ui_language == 1 ?
        L"无法应用临时内存补丁。\r\n\r\n阶段：%s\r\nWindows 错误码：%lu\r\n\r\n未修改任何磁盘上的 Steam 文件。" :
        ui_language == 2 ?
        L"無法套用暫時記憶體修補。\r\n\r\n階段：%s\r\nWindows 錯誤碼：%lu\r\n\r\n未修改任何磁碟上的 Steam 檔案。" :
        L"The temporary memory patch could not be applied.\r\n\r\nStage: %s\r\nWindows error: %lu\r\n\r\nNo Steam file was changed.";
    wsprintfW(message, format, ui_text(stage), error);
    MessageBoxW(0, message, L"Steam VSZa launcher", MB_OK | MB_ICONERROR);
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
    SetLastError(ERROR_MOD_NOT_FOUND);
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

static const char *additional_command_line(void) {
    const char *p = GetCommandLineA();
    if (!p) return "";
    while (*p == ' ' || *p == '\t') ++p;
    if (*p == '"') {
        ++p; while (*p && *p != '"') ++p; if (*p == '"') ++p;
    } else while (*p && *p != ' ' && *p != '\t') ++p;
    while (*p == ' ' || *p == '\t') ++p;
    return p;
}
static int same_executable(const char *a, const char *b) {
    HANDLE x, y; BY_HANDLE_FILE_INFORMATION ix, iy; int same = 0;
    x = CreateFileA(a, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0);
    if (x == INVALID_HANDLE_VALUE) return 0;
    y = CreateFileA(b, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0);
    if (y != INVALID_HANDLE_VALUE) {
        if (GetFileInformationByHandle(x, &ix) && GetFileInformationByHandle(y, &iy))
            same = ix.dwVolumeSerialNumber == iy.dwVolumeSerialNumber &&
                   ix.nFileIndexHigh == iy.nFileIndexHigh && ix.nFileIndexLow == iy.nFileIndexLow;
        CloseHandle(y);
    }
    CloseHandle(x); return same;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command, int show) {
    static char own_dir[MAX_PATH];
    static char helper[MAX_PATH], hook_path[MAX_PATH];

    char client_path[MAX_PATH];
    DWORD select_error = 0;
    int entry_touched = 0, launched = 0;
    static char steam_dir[MAX_PATH], steam_exe[MAX_PATH], steam_command_line[MAX_PATH + 1024];
    STARTUPINFOA si;
    DWORD steam_dir_size = sizeof(steam_dir);
    const char *args = additional_command_line();
    PROCESS_INFORMATION pi;
    static BYTE hook[0x6C0]; BYTE original[9], jump[9] = {0xE9,0,0,0,0,0x90,0x90,0x90,0x90};
    const BYTE expected[9] = {0x55,0x8B,0xEC,0x81,0xEC,0x8C,0x02,0x00,0x00};
    DWORD hook_size, old_protect, written, remote_exit = 0, process_exit = STILL_ACTIVE;
    DWORD failure_error = 0;
    const char *failure_stage = "unknown";
    DWORD_PTR client_base, local_kernel, remote_kernel, remote_loadlibrary;
    SIZE_T path_len;
    LPVOID remote_path;
    HANDLE thread;

    int i;

    (void)instance; (void)previous; (void)command; (void)show;
    detect_ui_language();
    zero_bytes(&pi, sizeof(pi));
    GetModuleFileNameA(0, own_dir, sizeof(own_dir)); dirname_in_place(own_dir);
    wsprintfA(helper, "%s\\steam_zstd.dll", own_dir);
    wsprintfA(hook_path, "%s\\vsza_hook.bin", own_dir);
    if (GetFileAttributesA(helper) == INVALID_FILE_ATTRIBUTES ||
        !read_file(hook_path, hook, sizeof(hook), &hook_size)) {
        fail("Patch files were not found. Extract all files together."); return 2;
    }
    pi.dwProcessId = find_running(&select_error);
    if (!pi.dwProcessId && select_error) {
        if (select_error == ERROR_ACCESS_DENIED)
            fail("Cannot inspect Steam. Run this launcher as administrator.");
        else if (select_error == ERROR_MORE_DATA)
            fail("Multiple steam.exe processes were found. Wait for startup to finish, then retry.");
        else fail_stage("query running Steam executable path", select_error);
        return 3;
    }
    if (pi.dwProcessId) {
        pi.hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
            PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | SYNCHRONIZE,
            FALSE, pi.dwProcessId);
        if (!pi.hProcess) { fail_stage("open running Steam (try administrator)", GetLastError()); return 3; }
    } else {
        if (RegGetValueA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath",
                        RRF_RT_REG_SZ, 0, steam_dir, &steam_dir_size) != ERROR_SUCCESS ||
            lstrlenA(steam_dir) > MAX_PATH - 12) {
            fail("Steam installation path was not found. Open Steam normally, then run this launcher."); return 3;
        }
        wsprintfA(steam_exe, "%s\\steam.exe", steam_dir);
        if (GetFileAttributesA(steam_exe) == INVALID_FILE_ATTRIBUTES) {
            fail("Registered steam.exe was not found. Open Steam normally, then run this launcher."); return 3;
        }
        wsprintfA(steam_command_line, "\"%s\" -noverifyfiles -nobootstrapupdate", steam_exe);
        if (lstrlenA(steam_command_line) + lstrlenA(args) + 2 >= (int)sizeof(steam_command_line)) {
            fail("Steam command line is too long."); return 3;
        }
        if (*args) { lstrcatA(steam_command_line, " "); lstrcatA(steam_command_line, args); }
        zero_bytes(&si, sizeof(si)); si.cb = sizeof(si);
        if (!CreateProcessA(steam_exe, steam_command_line, 0, 0, FALSE, 0, 0, steam_dir, &si, &pi)) {
            fail_stage("start Steam (try administrator)", GetLastError()); return 3;
        }
        CloseHandle(pi.hThread); pi.hThread = 0; launched = 1;
    }
    client_base = 0;
    /* steamclient.dll is delay-loaded by this old Steam build, not imported
       before the primary thread begins. Wait for that normal load instead of
       assuming it exists in a newly created suspended process. */
    for (i = 0; i < 120 && !client_base; ++i) {
        Sleep(500);
        if (!pi.hProcess) {
            pi.dwProcessId = find_running(&select_error);
            if (!pi.dwProcessId) {
                if (select_error == ERROR_ACCESS_DENIED) {
                    failure_stage = "inspect replacement Steam (try administrator)";
                    failure_error = select_error; goto patch_fail;
                }
                continue;
            }
            if (!same_executable(steam_exe, selected_path)) {
                failure_stage = "replacement Steam belongs to a different installation";
                failure_error = ERROR_INVALID_DATA; goto patch_fail;
            }
            pi.hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | SYNCHRONIZE,
                FALSE, pi.dwProcessId);
            if (!pi.hProcess) {
                failure_stage = "open replacement Steam"; failure_error = GetLastError(); goto patch_fail;
            }
        }
        if (!GetExitCodeProcess(pi.hProcess, &process_exit)) {
            failure_stage = "query Steam process state";
            failure_error = GetLastError();
            goto patch_fail;
        }
        if (process_exit != STILL_ACTIVE) {
            if (launched) { CloseHandle(pi.hProcess); pi.hProcess = 0; continue; }
            failure_stage = "Steam exited before steamclient.dll loaded";
            failure_error = process_exit;
            goto patch_fail;
        }
        client_base = remote_module(pi.dwProcessId, "steamclient.dll");
        if (!client_base && GetLastError() == ERROR_ACCESS_DENIED) {
            failure_stage = "inspect Steam modules (run as administrator)";
            failure_error = ERROR_ACCESS_DENIED; goto patch_fail;
        }
    }
    if (!client_base) {
        failure_stage = "wait for steamclient.dll (60 second timeout)";
        failure_error = WAIT_TIMEOUT;
        goto patch_fail;
    }
    if (!loaded_client_path(pi.dwProcessId, client_path) || !supported_file(client_path))
        goto version_fail;
    *(LONG *)(jump + 1) = (LONG)(STEAMCLIENT_CAVE_RVA - (STEAMCLIENT_ENTRY_RVA + 5));
    remote_kernel = remote_module(pi.dwProcessId, "kernel32.dll");
    local_kernel = (DWORD_PTR)GetModuleHandleA("kernel32.dll");
    if (!remote_kernel || !local_kernel) {
        failure_stage = "locate kernel32.dll";
        failure_error = GetLastError();
        goto patch_fail;
    }
    if (!ReadProcessMemory(pi.hProcess, (LPCVOID)(client_base + STEAMCLIENT_ENTRY_RVA),
                           original, sizeof(original), 0)) {
        failure_stage = "read steamclient.dll memory";
        failure_error = GetLastError();
        goto patch_fail;
    }
    for (i = 0; i < 9 && original[i] == jump[i]; ++i) {}
    if (i == 9) {
        static BYTE installed[0x6C0];
        if (remote_module(pi.dwProcessId, "steam_zstd.dll") &&
            ReadProcessMemory(pi.hProcess, (LPCVOID)(client_base + STEAMCLIENT_CAVE_RVA), installed, hook_size, 0)) {
            for (i = 0; (DWORD)i < hook_size && installed[i] == hook[i]; ++i) {}
            if ((DWORD)i == hook_size) {
                info("This Steam process is already patched.");
                CloseHandle(pi.hProcess); return 0;
            }
        }
        goto version_fail;
    }
    for (i = 0; i < 9; ++i) if (original[i] != expected[i]) goto version_fail;

    {
        MEMORY_BASIC_INFORMATION mbi; char owner[MAX_PATH]; char *name, *q;
        FARPROC load = GetProcAddress((HMODULE)local_kernel, "LoadLibraryA");
        if (!load || !VirtualQuery((LPCVOID)load, &mbi, sizeof(mbi)) ||
            !GetModuleFileNameA((HMODULE)mbi.AllocationBase, owner, sizeof(owner))) {
            failure_stage = "resolve LoadLibraryA owner"; failure_error = GetLastError(); goto patch_fail;
        }
        name = owner;
        for (q = owner; *q; ++q) if (*q == '\\' || *q == '/') name = q + 1;
        remote_kernel = remote_module(pi.dwProcessId, name);
        if (!remote_kernel) { failure_stage = "locate loader owner in Steam"; failure_error = ERROR_MOD_NOT_FOUND; goto patch_fail; }
        remote_loadlibrary = remote_kernel + ((DWORD_PTR)load - (DWORD_PTR)mbi.AllocationBase);
    }
    path_len = (SIZE_T)lstrlenA(helper) + 1;
    remote_path = VirtualAllocEx(pi.hProcess, 0, path_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_path) {
        failure_stage = "allocate helper path in Steam";
        failure_error = GetLastError();
        goto patch_fail;
    }
    if (!WriteProcessMemory(pi.hProcess, remote_path, helper, path_len, 0)) {
        failure_stage = "write helper path into Steam";
        failure_error = GetLastError();
        goto patch_fail;
    }
    thread = CreateRemoteThread(pi.hProcess, 0, 0, (LPTHREAD_START_ROUTINE)remote_loadlibrary,
                                remote_path, 0, 0);
    if (!thread) {
        failure_stage = "start helper loader in Steam";
        failure_error = GetLastError();
        goto patch_fail;
    }
    written = WaitForSingleObject(thread, 15000);
    if (written != WAIT_OBJECT_0) {
        failure_stage = "wait for helper loader";
        failure_error = written;
        CloseHandle(thread);
        goto patch_fail;
    }
    if (!GetExitCodeThread(thread, &remote_exit)) {
        failure_stage = "read helper loader result";
        failure_error = GetLastError();
        CloseHandle(thread);
        goto patch_fail;
    }
    CloseHandle(thread);
    VirtualFreeEx(pi.hProcess, remote_path, 0, MEM_RELEASE);
    if (!remote_exit) goto helper_fail;

    if (!freeze(pi.dwProcessId, client_base + STEAMCLIENT_ENTRY_RVA,
                client_base + STEAMCLIENT_CAVE_RVA, hook_size)) {
        failure_stage = "pause Steam threads safely (pause downloads and retry)";
        failure_error = GetLastError(); goto patch_fail;
    }
    if (!ReadProcessMemory(pi.hProcess, (LPCVOID)(client_base + STEAMCLIENT_ENTRY_RVA), original, 9, 0)) {
        failure_stage = "recheck entry"; failure_error = GetLastError(); goto patch_fail;
    }
    for (i = 0; i < 9; ++i) if (original[i] != expected[i]) goto version_fail;
    if (!VirtualProtectEx(pi.hProcess, (LPVOID)(client_base + STEAMCLIENT_CAVE_RVA),
                          hook_size, PAGE_EXECUTE_READWRITE, &old_protect)) {
        failure_stage = "change hook memory protection";
        failure_error = GetLastError();
        goto patch_fail;
    }
    if (!WriteProcessMemory(pi.hProcess, (LPVOID)(client_base + STEAMCLIENT_CAVE_RVA),
                            hook, hook_size, &written) || written != hook_size) {
        failure_stage = "write compatibility hook";
        failure_error = GetLastError();
        goto patch_fail;
    }
    VirtualProtectEx(pi.hProcess, (LPVOID)(client_base + STEAMCLIENT_CAVE_RVA),
                     hook_size, old_protect, &written);
    *(LONG *)(jump + 1) = (LONG)(STEAMCLIENT_CAVE_RVA - (STEAMCLIENT_ENTRY_RVA + 5));
    if (!VirtualProtectEx(pi.hProcess, (LPVOID)(client_base + STEAMCLIENT_ENTRY_RVA),
                          sizeof(jump), PAGE_EXECUTE_READWRITE, &old_protect)) {
        failure_stage = "change entry memory protection";
        failure_error = GetLastError();
        goto patch_fail;
    }
    entry_touched = 1;
    if (!WriteProcessMemory(pi.hProcess, (LPVOID)(client_base + STEAMCLIENT_ENTRY_RVA),
                            jump, sizeof(jump), &written) || written != sizeof(jump)) {
        failure_stage = "activate compatibility hook";
        failure_error = GetLastError();
        goto patch_fail;
    }
    VirtualProtectEx(pi.hProcess, (LPVOID)(client_base + STEAMCLIENT_ENTRY_RVA),
                     sizeof(jump), old_protect, &written);
    FlushInstructionCache(pi.hProcess, 0, 0);
    thaw();
    CloseHandle(pi.hProcess);
    info("Patch applied to the running Steam process. You may resume downloads.\r\nApply again after Steam fully exits or restarts.");
    return 0;

version_fail:
    thaw();
    fail("The loaded steamclient.dll is not the supported November 2024 build.");
    goto cleanup;
helper_fail:
    thaw();
    fail("steam_zstd.dll could not be loaded. The Universal C Runtime may be missing.");
    goto cleanup;
patch_fail:
    if (entry_touched) {
        DWORD restore_protect;
        if (VirtualProtectEx(pi.hProcess, (LPVOID)(client_base + STEAMCLIENT_ENTRY_RVA), 9,
                             PAGE_EXECUTE_READWRITE, &restore_protect)) {
            WriteProcessMemory(pi.hProcess, (LPVOID)(client_base + STEAMCLIENT_ENTRY_RVA), expected, 9, 0);
            VirtualProtectEx(pi.hProcess, (LPVOID)(client_base + STEAMCLIENT_ENTRY_RVA), 9, old_protect, &restore_protect);
            FlushInstructionCache(pi.hProcess, 0, 0);
        }
    }
    thaw();
    fail_stage(failure_stage, failure_error);
cleanup:
    if (pi.hProcess) CloseHandle(pi.hProcess);
    return 4;
}
