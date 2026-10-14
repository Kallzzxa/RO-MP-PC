// ============================================================
// Remote players: spawn/sync/hapus ped via plugin-sdk.
// SEMUA fungsi di sini hanya dipanggil dari game thread.
// ============================================================
#pragma once
#include <cstdint>
#include <string>
#include <map>

#include "plugin_sa/game_sa/CCivilianPed.h"
#include "plugin_sa/game_sa/CWorld.h"
#include "plugin_sa/game_sa/common.h"
#include "plugin_sa/game_sa/CStreaming.h"

namespace rompmod {

struct RemotePed {
    CCivilianPed* ped = nullptr;
    std::string name;
    // interpolasi sederhana
    CVector target{0, 0, 0};
    bool has_target = false;
};

class RemotePeds {
public:
    static RemotePeds& Get() {
        static RemotePeds r;
        return r;
    }

    std::map<uint16_t, RemotePed> peds;

    void Add(uint16_t id, const std::string& name);
    void Remove(uint16_t id);
    void SetPos(uint16_t id, float x, float y, float z);
    void Clear();
    void Tick();  // interpolasi, panggil tiap frame game thread

    // Kirim posisi pemain lokal ke server.
    static void SendLocalPos();

private:
    RemotePeds() = default;
};

}  // namespace rompmod
