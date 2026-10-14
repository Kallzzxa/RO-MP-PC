// ============================================================
// RO-MP playable mod — core state.
// ASI untuk GTA:SA PC 1.0 US. Arsitektur:
//   - net_worker (thread): PcClient (RO-MP protocol) -> event queue
//   - game thread: proses event -> spawn/sync peds via plugin-sdk
//   - overlay (ImGui, di EndScene hook): browser + chat + nametag
// Referensi arsitektur client: https://github.com/FlexodMR/SA-MP
//   (reverse SA-MP 0.3.7; dipakai sebagai acuan pola, bukan kode).
// ============================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <deque>
#include <mutex>

#include "pc_client.h"
#include "pc_browser.h"

namespace rompmod {

// Event dari network thread ke game thread.
struct GameEvent {
    enum Type {
        Connected, Rejected, Disconnected,
        Chat, PlayerJoin, PlayerLeave, PlayerList, PlayerPos,
        NuiMessage,
    } type = Chat;
    uint16_t player_id = 0;
    uint8_t kind = 0;
    std::string text;
    float x = 0, y = 0, z = 0;
};

struct ChatLine {
    std::string text;
    int color = 0;
};

struct Mod {
    // --- koneksi (di-set dari overlay) ---
    std::string pending_host;
    uint16_t pending_port = 0;
    std::string pending_nick = "Player";
    bool want_connect = false;
    bool want_disconnect = false;

    // --- state ---
    bool connected = false;      // sudah WELCOME
    bool connecting = false;
    std::string server_name;
    std::string status;

    // --- chat ---
    std::vector<ChatLine> chat;
    static constexpr size_t kMaxChat = 100;

    // --- event queue (net -> game), mutex-protected ---
    std::deque<GameEvent> events;
    std::mutex events_mutex;

    void PushEvent(GameEvent ev) {
        std::lock_guard<std::mutex> l(events_mutex);
        events.push_back(std::move(ev));
    }
    bool PopEvent(GameEvent& out) {
        std::lock_guard<std::mutex> l(events_mutex);
        if (events.empty()) return false;
        out = std::move(events.front());
        events.pop_front();
        return true;
    }

    void AddChat(const std::string& t, int color = 0) {
        chat.push_back({t, color});
        if (chat.size() > kMaxChat) chat.erase(chat.begin());
    }

    static Mod& Get() {
        static Mod m;
        return m;
    }
};

}  // namespace rompmod
