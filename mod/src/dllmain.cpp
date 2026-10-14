// ============================================================
// RO-MP playable mod — ASI entry.
// Copy romp.asi ke folder GTA:SA 1.0 US (butuh ASI loader).
// ============================================================
#include <windows.h>

#include "plugin_sa/plugin_sa.h"
#include "romp_mod.h"
#include "net_worker.h"
#include "d3d9_hook.h"

#include <shlobj.h>
#include <cstdio>

namespace rompmod {
namespace {

// Baca %APPDATA%/RO-MP-PC/connect.txt yang ditulis launcher (one-shot).
// Format: host\nport\nnick\n
bool ReadConnectInfo(std::string& host, uint16_t& port, std::string& nick) {
    char appdata[MAX_PATH] = "";
    if (FAILED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata)))
        return false;
    std::string path = std::string(appdata) + "\\RO-MP-PC\\connect.txt";
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return false;
    char h[128] = "", n[32] = "";
    int p = 0;
    bool ok = fgets(h, sizeof(h), f) != nullptr &&
              fscanf(f, "%d\n", &p) == 1 &&
              fgets(n, sizeof(n), f) != nullptr;
    fclose(f);
    DeleteFileA(path.c_str());  // one-shot: hapus setelah dibaca
    if (!ok || p <= 0) return false;
    auto trim = [](char* s) {
        size_t l = strlen(s);
        while (l && (s[l-1] == '\n' || s[l-1] == '\r' || s[l-1] == ' '))
            s[--l] = 0;
    };
    trim(h); trim(n);
    if (!h[0]) return false;
    host = h; port = (uint16_t)p; nick = n[0] ? n : "Player";
    return true;
}

DWORD WINAPI InitThread(LPVOID) {
    // Kecilkan prioritas sedikit agar tidak ganggu loading game.
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

    // Hook D3D9 (akan catch device saat game membuatnya).
    D3D9Hook_Install();

    // Jalankan network worker.
    NetWorker::Get().Start();

    // Auto-connect kalau launcher menulis connect.txt.
    std::string h; uint16_t p = 0; std::string n;
    if (ReadConnectInfo(h, p, n)) {
        Mod::Get().connecting = true;
        Mod::Get().status = "Menghubungi " + h + "...";
        NetWorker::Get().RequestConnect(h, p, n);
    } else {
        Mod::Get().AddChat("RO-MP mod dimuat. F7 = server browser.", 2);
    }
    return 0;
}

}  // namespace
}  // namespace rompmod

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, rompmod::InitThread, nullptr, 0, nullptr);
    } else if (reason == DLL_PROCESS_DETACH) {
        rompmod::NetWorker::Get().Stop();
        rompmod::D3D9Hook_Remove();
    }
    return TRUE;
}
