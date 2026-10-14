// ============================================================
// D3D9 hook — implementasi.
// ============================================================
#include "d3d9_hook.h"
#include "overlay.h"
#include <MinHook.h>

namespace rompmod {
namespace {

EndSceneFn oEndScene = nullptr;
ResetFn oReset = nullptr;

HRESULT __stdcall hkEndScene(IDirect3DDevice9* device) {
    OnFrame(device);  // render overlay ImGui
    return oEndScene(device);
}

HRESULT __stdcall hkReset(IDirect3DDevice9* device,
                          D3DPRESENT_PARAMETERS* pp) {
    Overlay_OnReset();
    HRESULT hr = oReset(device, pp);
    Overlay_OnResetDone(device);
    return hr;
}

// Hook CreateDevice untuk dapat vtable device yang asli.
using CreateDeviceFn = HRESULT(__stdcall*)(
    IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD,
    D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
CreateDeviceFn oCreateDevice = nullptr;

HRESULT __stdcall hkCreateDevice(IDirect3D9* self, UINT adapter,
                                 D3DDEVTYPE devtype, HWND hwnd,
                                 DWORD flags, D3DPRESENT_PARAMETERS* pp,
                                 IDirect3DDevice9** out) {
    HRESULT hr = oCreateDevice(self, adapter, devtype, hwnd, flags, pp, out);
    if (SUCCEEDED(hr) && *out && !oEndScene) {
        void** vtbl = *(void***)*out;
        MH_CreateHook(vtbl[42], (void*)hkEndScene, (void**)&oEndScene);
        MH_CreateHook(vtbl[16], (void*)hkReset, (void**)&oReset);
        MH_EnableHook(vtbl[42], TRUE);
        MH_EnableHook(vtbl[16], TRUE);
    }
    return hr;
}

using D3DCreate9Fn = IDirect3D9*(__stdcall*)(UINT);
D3DCreate9Fn oD3DCreate9 = nullptr;

IDirect3D9* __stdcall hkD3DCreate9(UINT ver) {
    IDirect3D9* d3d = oD3DCreate9(ver);
    if (d3d && !oCreateDevice) {
        void** vtbl = *(void***)d3d;
        MH_CreateHook(vtbl[16], (void*)hkCreateDevice, (void**)&oCreateDevice);
        MH_EnableHook(vtbl[16], TRUE);
    }
    return d3d;
}

}  // namespace

bool D3D9Hook_Install() {
    if (MH_Initialize() != MH_OK) return false;
    HMODULE d3d9 = GetModuleHandleA("d3d9.dll");
    if (!d3d9) d3d9 = LoadLibraryA("d3d9.dll");
    if (!d3d9) return false;
    void* fn = GetProcAddress(d3d9, "Direct3DCreate9");
    if (!fn) return false;
    if (MH_CreateHook(fn, (void*)hkD3DCreate9, (void**)&oD3DCreate9) != MH_OK)
        return false;
    return MH_EnableHook(fn, TRUE) == MH_OK;
}

void D3D9Hook_Remove() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}

}  // namespace rompmod
