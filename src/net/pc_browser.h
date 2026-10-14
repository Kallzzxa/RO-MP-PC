// ============================================================
// RO-MP PC — server browser: UDP query + master list + LAN beacon.
// Adaptasi dari modul ROMP_Browser (Android). Semua fungsi sinkron
// dengan timeout — panggil dari thread terpisah bila perlu.
// ============================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace romppc {

struct BrowserServer {
    std::string ip;
    uint16_t port = 0;
    std::string hostname;
    std::string motd;
    std::string version;
    uint16_t players = 0;
    uint16_t maxplayers = 0;
    bool has_password = false;
    int ping_ms = -1;
};

struct BrowserPlayer {
    uint16_t id = 0;
    std::string name;
};

// Query info + daftar pemain satu server via UDP. true jika menjawab.
bool BrowserQuery(const std::string& host, uint16_t port,
                  BrowserServer& out, std::vector<BrowserPlayer>& players,
                  int timeout_ms = 1500);

// Ambil daftar server dari master (TCP). true jika berhasil.
bool BrowserFetchMaster(const std::string& master_host, uint16_t master_port,
                        std::vector<BrowserServer>& out, int timeout_ms = 4000);

// Dengarkan beacon LAN (UDP 7779). Tanpa perlu master/VPS.
bool BrowserListenLan(std::vector<BrowserServer>& out, int timeout_ms = 3000);

}  // namespace romppc
