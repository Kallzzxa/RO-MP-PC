// ============================================================
// RO-MP launcher — luncurkan GTA:SA + inject romp.dll (ala SA-MP).
// Windows only. Dipakai tombol PLAY di server browser.
// Info server diteruskan via %APPDATA%/RO-MP-PC/connect.txt
// yang dibaca romp.dll saat game start (one-shot).
// ============================================================
#pragma once
#include <cstdint>
#include <string>

namespace romppc {

// Cari folder GTA:SA: config tersimpan -> registry -> path umum.
// Return "" jika tidak ketemu.
std::string FindGtaSa();

// Simpan folder GTA:SA pilihan user.
void SaveGtaPath(const std::string& folder);

// Tulis connect.txt untuk dibaca romp.dll.
bool WriteConnectInfo(const std::string& host, uint16_t port,
                      const std::string& nick);

// Luncurkan gta_sa.exe (suspended) + inject romp.dll + resume.
// romp.dll harus ada di folder yang sama dengan romp.exe.
// Return "" jika sukses, atau pesan error.
std::string LaunchGtaWithMod(const std::string& gta_folder,
                             const std::string& host, uint16_t port,
                             const std::string& nick);

}  // namespace romppc
