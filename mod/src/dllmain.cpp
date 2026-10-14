// ============================================================
// RO-MP playable mod — ASI entry.
// Copy romp.asi ke folder GTA:SA 1.0 US (butuh ASI loader).
// ============================================================
#include <windows.h>

#include "plugin_sa/plugin_sa.h"
#include "romp_mod.h"
#include "net_worker.h"
#include "d3d9_hook.h"

namespace rompmod {
namespace {

DWORD WINAPI InitThread(LPVOID) {
    // Kecilkan prioritas sedikit agar tidak ganggu loading game.
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

    // Hook D3D9 (akan catch device saat game membuatnya).
    D3D9Hook_Install();

    // Jalankan network worker.
    NetWorker::Get().Start();

    Mod::Get().AddChat("RO-MP mod dimuat. F7 = server browser.", 2);
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
