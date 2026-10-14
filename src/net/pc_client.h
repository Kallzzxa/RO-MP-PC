// ============================================================
// RO-MP PC — TCP client untuk protokol RO-MP v1.
// Portable: POSIX + Windows (Winsock). Tanpa dependency GUI.
// Event-based: panggil Poll() tiap frame, ambil event via NextEvent().
// ============================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <deque>
#include <map>

namespace romppc {

enum class EventType {
    Connected,      // text = "nick (id=N)"
    Rejected,       // text = alasan
    Disconnected,   // text = alasan
    Chat,           // from = playerid (0xFFFF = server), kind, text
    PlayerJoin,     // from = playerid, text = nama
    PlayerLeave,    // from = playerid, text = nama
    PlayerList,     // players = snapshot
    NuiMessage,     // text = JSON
    NuiFocus,       // from = 1/0
};

struct PlayerInfo {
    uint16_t id = 0;
    std::string name;
    float x = 0, y = 0, z = 0, heading = 0;
    bool has_pos = false;
};

struct Event {
    EventType type = EventType::Chat;
    uint16_t from = 0;
    uint8_t kind = 0;
    std::string text;
    std::vector<PlayerInfo> players;
};

class Client {
public:
    Client();
    ~Client();

    // Connect ke server. Mengirim HELLO. return false jika TCP gagal.
    // WELCOME/REJECT datang sebagai event setelah Poll().
    bool Connect(const std::string& host, uint16_t port,
                 const std::string& nickname,
                 const std::string& password = "");
    void Disconnect(const std::string& reason = "");

    bool IsConnected() const { return connected_; }

    // Proses I/O + ping/pong. Panggil tiap frame (~60Hz).
    void Poll();

    bool HasEvent() const { return !events_.empty(); }
    Event NextEvent();

    // Kirim chat biasa / command ("/...").
    void SendChat(const std::string& text);

    // Kirim posisi pemain lokal (untuk radar/server lain).
    void SendPos(float x, float y, float z);

    uint16_t MyId() const { return my_id_; }
    const std::string& MyNick() const { return nick_; }
    const std::map<uint16_t, PlayerInfo>& Players() const { return players_; }

private:
    bool PumpRecv();
    void HandleFrame(uint8_t type, const uint8_t* p, size_t n);
    void PushChat(uint16_t from, uint8_t kind, const std::string& text);
    bool SendFrame(uint8_t type, const uint8_t* payload, size_t n);

    int sock_ = -1;  // di Windows: SOCKET (cast)
    bool connected_ = false;
    bool welcomed_ = false;
    std::string nick_;
    std::vector<uint8_t> rxbuf_;
    std::deque<Event> events_;
    std::map<uint16_t, PlayerInfo> players_;
    uint16_t my_id_ = 0;
    uint64_t last_ping_ms_ = 0;
    uint64_t last_pong_ms_ = 0;
};

}  // namespace romppc
