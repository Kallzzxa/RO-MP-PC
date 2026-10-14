// ============================================================
// D3D9 hook untuk overlay ImGui.
// Pola: hook Direct3DCreate9 -> hook IDirect3D9::CreateDevice ->
// hook device EndScene/Reset via MinHook. Tidak butuh alamat game.
// ============================================================
#pragma once
#include <d3d9.h>

namespace rompmod {

using EndSceneFn = HRESULT(__stdcall*)(IDirect3DDevice9*);
using ResetFn = HRESULT(__stdcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);

// Dipanggil tiap frame dari EndScene yang di-hook.
void OnFrame(IDirect3DDevice9* device);

bool D3D9Hook_Install();
void D3D9Hook_Remove();

}  // namespace rompmod
