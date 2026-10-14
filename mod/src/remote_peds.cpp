// ============================================================
// Remote peds — implementasi.
// ============================================================
#include "remote_peds.h"
#include "romp_mod.h"
#include "net_worker.h"

namespace rompmod {

void RemotePeds::Add(uint16_t id, const std::string& name) {
    if (peds.count(id)) return;
    // Model 0 = CJ; selalu tersedia (pemain lokal memakainya).
    CCivilianPed* ped = new CCivilianPed(PED_TYPE_CIVMALE, 0);
    if (!ped) return;
    ped->SetPosn(0.0f, 0.0f, -100.0f);  // parkir di bawah map dulu
    ped->bIsVisible = true;
    CWorld::Add(ped);
    RemotePed r;
    r.ped = ped;
    r.name = name;
    peds[id] = r;
}

void RemotePeds::Remove(uint16_t id) {
    auto it = peds.find(id);
    if (it == peds.end()) return;
    if (it->second.ped) {
        CWorld::Remove(it->second.ped);
        delete it->second.ped;
    }
    peds.erase(it);
}

void RemotePeds::SetPos(uint16_t id, float x, float y, float z) {
    auto it = peds.find(id);
    if (it == peds.end() || !it->second.ped) return;
    it->second.target = CVector(x, y, z);
    it->second.has_target = true;
}

void RemotePeds::Clear() {
    for (auto& kv : peds) {
        if (kv.second.ped) {
            CWorld::Remove(kv.second.ped);
            delete kv.second.ped;
        }
    }
    peds.clear();
}

void RemotePeds::Tick() {
    for (auto& kv : peds) {
        auto* ped = kv.second.ped;
        if (!ped || !kv.second.has_target) continue;
        // Lerp 25% per tick ke target (haluskan sync 10Hz).
        CVector cur = ped->GetPosition();
        CVector tgt = kv.second.target;
        float dx = tgt.x - cur.x, dy = tgt.y - cur.y, dz = tgt.z - cur.z;
        float dist2 = dx * dx + dy * dy + dz * dz;
        if (dist2 > 2500.0f) {
            // teleport kalau terlalu jauh (spawn/join)
            ped->SetPosn(tgt.x, tgt.y, tgt.z);
        } else {
            ped->SetPosn(cur.x + dx * 0.25f, cur.y + dy * 0.25f,
                         cur.z + dz * 0.25f);
        }
    }
}

void RemotePeds::SendLocalPos() {
    CPlayerPed* me = FindPlayerPed();
    if (!me) return;
    CVector p = me->GetPosition();
    NetWorker::Get().SendPos(p.x, p.y, p.z);
}

}  // namespace rompmod
