// ============================================================
// Overlay ImGui: server browser + chat + nametag pemain.
// Dirender dari EndScene hook (game thread).
// ============================================================
#pragma once
#include <d3d9.h>

namespace rompmod {

// Dipanggil dari hkEndScene / hkReset.
void OnFrame(IDirect3DDevice9* device);
void Overlay_OnReset();
void Overlay_OnResetDone(IDirect3DDevice9* device);

// Dipanggil game thread tiap tick untuk proses event network.
void Overlay_ProcessEvents();

// Toggle browser/chat (dipanggil dari game key hook atau auto).
void Overlay_SetShowBrowser(bool show);

}  // namespace rompmod
