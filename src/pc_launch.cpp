// ============================================================
// Launcher — implementasi (Windows only).
// ============================================================
#include "pc_launch.h"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <cstdio>
#include <vector>

namespace romppc {
namespace {

std::string AppDataRoMp() {
    char p[MAX_PATH] = "";
    if (FAILED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, p)))
        return "";
    std::string d = std::string(p) + "\\RO-MP-PC";
    CreateDirectoryA(d.c_str(), NULL);
    return d;
}

bool FileExists(const std::string& p) {
    DWORD a = GetFileAttributesA(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

std::string ReadAll(const std::string& path) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return "";
    char buf[512] = "";
    fgets(buf, sizeof(buf), f);
    fclose(f);
    // trim
    std::string s = buf;
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
        s.pop_back();
    return s;
}

}  // namespace

std::string FindGtaSa() {
    // 0. romp.ini di folder launcher (ditulis installer).
    {
        char modPath[MAX_PATH] = "";
        GetModuleFileNameA(NULL, modPath, sizeof(modPath));
        std::string ini = modPath;
        auto bs = ini.find_last_of("\\/");
        ini = (bs == std::string::npos ? "" : ini.substr(0, bs + 1)) + "romp.ini";
        char folder[MAX_PATH] = "";
        GetPrivateProfileStringA("game", "folder", "", folder, sizeof(folder),
                                 ini.c_str());
        if (folder[0] && FileExists(std::string(folder) + "\\gta_sa.exe"))
            return folder;
    }
    // 1. Config tersimpan.
    std::string saved = ReadAll(AppDataRoMp() + "\\gta_path.txt");
    if (!saved.empty() && FileExists(saved + "\\gta_sa.exe")) return saved;

    // 2. Registry Rockstar.
    HKEY h;
    const char* keys[] = {
        "SOFTWARE\\Rockstar Games\\GTA San Andreas",
        "SOFTWARE\\WOW6432Node\\Rockstar Games\\GTA San Andreas",
    };
    for (auto k : keys) {
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, k, 0, KEY_READ, &h) ==
            ERROR_SUCCESS) {
            char v[MAX_PATH] = "";
            DWORD sz = sizeof(v);
            if (RegQueryValueExA(h, "InstallFolder", nullptr, nullptr,
                                 (LPBYTE)v, &sz) == ERROR_SUCCESS) {
                RegCloseKey(h);
                std::string d = v;
                if (FileExists(d + "\\gta_sa.exe")) return d;
            } else {
                RegCloseKey(h);
            }
        }
    }

    // 3. Path umum.
    const char* drives[] = {"C:", "D:", "E:"};
    const char* dirs[] = {
        "\\Program Files\\Rockstar Games\\GTA San Andreas",
        "\\Program Files (x86)\\Rockstar Games\\GTA San Andreas",
        "\\Games\\GTA San Andreas",
        "\\GTA San Andreas",
    };
    for (auto dr : drives)
        for (auto d : dirs) {
            std::string p = std::string(dr) + d;
            if (FileExists(p + "\\gta_sa.exe")) return p;
        }
    return "";
}

void SaveGtaPath(const std::string& folder) {
    std::string d = AppDataRoMp();
    if (d.empty()) return;
    FILE* f = fopen((d + "\\gta_path.txt").c_str(), "w");
    if (f) {
        fprintf(f, "%s\n", folder.c_str());
        fclose(f);
    }
}

bool WriteConnectInfo(const std::string& host, uint16_t port,
                      const std::string& nick) {
    std::string d = AppDataRoMp();
    if (d.empty()) return false;
    FILE* f = fopen((d + "\\connect.txt").c_str(), "w");
    if (!f) return false;
    fprintf(f, "%s\n%d\n%s\n", host.c_str(), port, nick.c_str());
    fclose(f);
    return true;
}

std::string LaunchGtaWithMod(const std::string& gta_folder,
                             const std::string& host, uint16_t port,
                             const std::string& nick) {
    std::string exe = gta_folder + "\\gta_sa.exe";
    if (!FileExists(exe)) return "gta_sa.exe tidak ditemukan di folder itu.";

    // romp.dll harus di folder yang sama dengan romp.exe (launcher).
    char modPath[MAX_PATH] = "";
    GetModuleFileNameA(NULL, modPath, sizeof(modPath));
    std::string dll = modPath;
    auto bs = dll.find_last_of("\\/");
    dll = (bs == std::string::npos ? "" : dll.substr(0, bs + 1)) + "romp.dll";
    if (!FileExists(dll))
        return "romp.dll tidak ditemukan (harus di folder yang sama dengan romp.exe).";

    if (!WriteConnectInfo(host, port, nick))
        return "Gagal tulis connect info.";

    // Luncurkan suspended.
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::string cmd = "\"" + exe + "\"";
    // Working dir = folder GTA (penting!).
    if (!CreateProcessA(exe.c_str(), cmd.data(), nullptr, nullptr, FALSE,
                        CREATE_SUSPENDED, nullptr, gta_folder.c_str(),
                        &si, &pi)) {
        return "Gagal jalankan gta_sa.exe.";
    }

    // Inject: LoadLibraryW di remote process.
    std::string err;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, dll.c_str(), -1, nullptr, 0);
    std::vector<wchar_t> wpath(wlen);
    MultiByteToWideChar(CP_UTF8, 0, dll.c_str(), -1, wpath.data(), wlen);
    SIZE_T bytes = wlen * sizeof(wchar_t);
    LPVOID remote =
        VirtualAllocEx(pi.hProcess, nullptr, bytes, MEM_COMMIT | MEM_RESERVE,
                       PAGE_READWRITE);
    if (remote &&
        WriteProcessMemory(pi.hProcess, remote, wpath.data(), bytes, nullptr)) {
        HMODULE k32 = GetModuleHandleA("kernel32.dll");
        auto loadlib = (LPTHREAD_START_ROUTINE)GetProcAddress(k32, "LoadLibraryW");
        HANDLE th = CreateRemoteThread(pi.hProcess, nullptr, 0, loadlib,
                                       remote, 0, nullptr);
        if (th) {
            WaitForSingleObject(th, 15000);
            DWORD code = 0;
            GetExitCodeThread(th, &code);
            CloseHandle(th);
            if (code == 0) err = "Inject gagal (LoadLibrary gagal).";
        } else {
            err = "Inject gagal (CreateRemoteThread).";
        }
        VirtualFreeEx(pi.hProcess, remote, 0, MEM_RELEASE);
    } else {
        err = "Inject gagal (alokasi memori).";
    }

    if (err.empty()) {
        ResumeThread(pi.hThread);
    } else {
        TerminateProcess(pi.hProcess, 0);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return err;  // "" = sukses
}

}  // namespace romppc

#else  // bukan Windows

namespace romppc {
std::string FindGtaSa() { return ""; }
void SaveGtaPath(const std::string&) {}
bool WriteConnectInfo(const std::string&, uint16_t, const std::string&) {
    return false;
}
std::string LaunchGtaWithMod(const std::string&, const std::string&,
                             uint16_t, const std::string&) {
    return "Hanya tersedia di Windows.";
}
}  // namespace romppc

#endif
