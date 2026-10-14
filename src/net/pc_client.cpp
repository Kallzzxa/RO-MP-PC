// ============================================================
// RO-MP PC — implementasi TCP client.
// ============================================================
#include "pc_client.h"
#include "romp_protocol.h"

#include <chrono>
#include <cstring>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET sock_t;
  #define BAD_SOCK INVALID_SOCKET
  #define CLOSE_SOCK(s) closesocket(s)
  static bool net_init() {
      WSADATA w; return WSAStartup(MAKEWORD(2, 2), &w) == 0;
  }
#else
  #include <arpa/inet.h>
  #include <fcntl.h>
  #include <netdb.h>
  #include <sys/socket.h>
  #include <unistd.h>
  typedef int sock_t;
  #define BAD_SOCK (-1)
  #define CLOSE_SOCK(s) ::close(s)
  static bool net_init() { return true; }
#endif

namespace romppc {
namespace {

uint64_t now_ms() {
    return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool set_nonblock(sock_t s) {
#ifdef _WIN32
    u_long m = 1;
    return ioctlsocket(s, FIONBIO, &m) == 0;
#else
    int f = fcntl(s, F_GETFL, 0);
    return fcntl(s, F_SETFL, f | O_NONBLOCK) == 0;
#endif
}

}  // namespace

Client::Client() { net_init(); }
Client::~Client() { Disconnect(); }

bool Client::Connect(const std::string& host, uint16_t port,
                     const std::string& nickname,
                     const std::string& password) {
    Disconnect();
    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == BAD_SOCK) return false;

    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    // Coba sebagai IP literal dulu, lalu DNS.
    if (inet_pton(AF_INET, host.c_str(), &a.sin_addr) != 1) {
        hostent* he = gethostbyname(host.c_str());
        if (!he) { CLOSE_SOCK(s); return false; }
        memcpy(&a.sin_addr, he->h_addr_list[0], (size_t)he->h_length);
    }
    // Connect blocking dengan timeout via nonblock + select.
    set_nonblock(s);
    int rc = ::connect(s, (sockaddr*)&a, sizeof(a));
    bool ok = false;
#ifdef _WIN32
    if (rc == 0 || WSAGetLastError() == WSAEWOULDBLOCK) {
#else
    if (rc == 0 || errno == EINPROGRESS) {
#endif
        fd_set wf; FD_ZERO(&wf); FD_SET(s, &wf);
        timeval tv{3, 0};
        if (select((int)s + 1, nullptr, &wf, nullptr, &tv) > 0) ok = true;
    } else {
        ok = true;  // connect langsung sukses (localhost)
    }
    if (!ok) { CLOSE_SOCK(s); return false; }

    sock_ = (int)s;
    nick_ = nickname.substr(0, ROMP::kMaxNickLen);
    connected_ = true;
    welcomed_ = false;
    rxbuf_.clear();
    players_.clear();
    last_ping_ms_ = last_pong_ms_ = now_ms();

    // Kirim HELLO via builder header.
    auto hello = ROMP::build_hello(nickname, password);
    size_t sent = 0;
    while (sent < hello.size()) {
        int r = ::send((sock_t)sock_, (const char*)hello.data() + sent,
                       (int)(hello.size() - sent), 0);
        if (r <= 0) {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEWOULDBLOCK) continue;
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
#endif
            CLOSE_SOCK((sock_t)sock_);
            sock_ = -1;
            connected_ = false;
            return false;
        }
        sent += (size_t)r;
    }
    return true;
}

void Client::Disconnect(const std::string& reason) {
    if (sock_ != -1) {
        CLOSE_SOCK((sock_t)sock_);
        sock_ = -1;
    }
    if (connected_) {
        connected_ = false;
        Event e;
        e.type = EventType::Disconnected;
        e.text = reason.empty() ? "Koneksi ditutup." : reason;
        events_.push_back(std::move(e));
    }
}

bool Client::SendFrame(uint8_t type, const uint8_t* payload, size_t n) {
    if (!connected_ || sock_ == -1) return false;
    std::vector<uint8_t> p(payload, payload + n);
    auto f = ROMP::make_frame((ROMP::PacketType)type, p);
    size_t sent = 0;
    while (sent < f.size()) {
        int r = ::send((sock_t)sock_, (const char*)f.data() + sent,
                       (int)(f.size() - sent), 0);
        if (r <= 0) {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEWOULDBLOCK) continue;
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
#endif
            return false;
        }
        sent += (size_t)r;
    }
    return true;
}

void Client::SendChat(const std::string& text) {
    if (text.empty() || !connected_) return;
    auto f = ROMP::build_chat(text);
    size_t sent = 0;
    while (sent < f.size()) {
        int r = ::send((sock_t)sock_, (const char*)f.data() + sent,
                       (int)(f.size() - sent), 0);
        if (r <= 0) break;
        sent += (size_t)r;
    }
}

void Client::SendPos(float x, float y, float z) {
    if (!connected_ || !welcomed_) return;
    auto f = ROMP::build_pos(x, y, z, 0.0f, ROMP::PlayerState::OnFoot, 0);
    ::send((sock_t)sock_, (const char*)f.data(), (int)f.size(), 0);
}

void Client::Poll() {
    if (!connected_) return;
    if (!PumpRecv()) {
        Disconnect("Koneksi ke server terputus.");
        return;
    }
    // Ping tiap 15 dtk.
    uint64_t t = now_ms();
    if (t - last_ping_ms_ >= 15000) {
        last_ping_ms_ = t;
        auto f = ROMP::build_ping((uint32_t)t);
        ::send((sock_t)sock_, (const char*)f.data(), (int)f.size(), 0);
    }
}

bool Client::PumpRecv() {
    uint8_t tmp[8192];
    for (;;) {
        int r = ::recv((sock_t)sock_, (char*)tmp, sizeof(tmp), 0);
        if (r > 0) {
            rxbuf_.insert(rxbuf_.end(), tmp, tmp + r);
        } else if (r == 0) {
            return false;  // server tutup
        } else {
#ifdef _WIN32
            int e = WSAGetLastError();
            if (e == WSAEWOULDBLOCK) break;
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
#endif
            return false;
        }
    }
    // Parse frame: [u16 len LE][u8 type][payload].
    while (rxbuf_.size() >= 3) {
        uint16_t len = (uint16_t)(rxbuf_[0] | (rxbuf_[1] << 8));
        if (len < 1 || len > ROMP::kMaxFrameLen) return false;
        if (rxbuf_.size() < (size_t)2 + len) break;
        uint8_t type = rxbuf_[2];
        HandleFrame(type, rxbuf_.data() + 3, len - 1);
        rxbuf_.erase(rxbuf_.begin(), rxbuf_.begin() + 2 + len);
    }
    return true;
}

void Client::PushChat(uint16_t from, uint8_t kind, const std::string& text) {
    Event e;
    e.type = EventType::Chat;
    e.from = from;
    e.kind = kind;
    e.text = text;
    events_.push_back(std::move(e));
}

void Client::HandleFrame(uint8_t type, const uint8_t* p, size_t n) {
    using M = ROMP::PacketType;
    if (type == (uint8_t)M::WELCOME && n >= 2) {
        my_id_ = (uint16_t)(p[0] | (p[1] << 8));
        welcomed_ = true;
        size_t o = 2;
        std::string srv, motd;
        if (o < n) { uint8_t l = p[o++]; if (o + l <= n) { srv.assign((char*)p + o, l); o += l; } }
        if (o < n) { uint8_t l = p[o++]; if (o + l <= n) { motd.assign((char*)p + o, l); o += l; } }
        Event e;
        e.type = EventType::Connected;
        e.text = srv + "\n" + motd;
        events_.push_back(std::move(e));
    } else if (type == (uint8_t)M::REJECT && n >= 1) {
        uint8_t l = p[0];
        std::string reason;
        if (1 + l <= n) reason.assign((char*)p + 1, l);
        Event e;
        e.type = EventType::Rejected;
        e.text = reason;
        events_.push_back(std::move(e));
        Disconnect("Ditolak: " + reason);
    } else if (type == (uint8_t)M::CHAT_MSG && n >= 4) {
        uint16_t pid = (uint16_t)(p[0] | (p[1] << 8));
        uint8_t kind = p[2], ml = p[3];
        std::string msg;
        if (4 + ml <= n) msg.assign((char*)p + 4, ml);
        PushChat(pid, kind, msg);
    } else if (type == (uint8_t)M::PLAYER_JOIN && n >= 3) {
        uint16_t pid = (uint16_t)(p[0] | (p[1] << 8));
        uint8_t nl = p[2];
        PlayerInfo pi;
        pi.id = pid;
        if (3 + nl <= n) pi.name.assign((char*)p + 3, nl);
        players_[pid] = pi;
        Event e;
        e.type = EventType::PlayerJoin;
        e.from = pid;
        e.text = pi.name;
        events_.push_back(std::move(e));
    } else if (type == (uint8_t)M::PLAYER_LEAVE && n >= 2) {
        uint16_t pid = (uint16_t)(p[0] | (p[1] << 8));
        std::string nm;
        auto it = players_.find(pid);
        if (it != players_.end()) { nm = it->second.name; players_.erase(it); }
        Event e;
        e.type = EventType::PlayerLeave;
        e.from = pid;
        e.text = nm;
        events_.push_back(std::move(e));
    } else if (type == (uint8_t)M::PLAYER_LIST && n >= 1) {
        uint8_t cnt = p[0];
        size_t o = 1;
        players_.clear();
        std::vector<PlayerInfo> snap;
        for (int i = 0; i < cnt && o + 3 <= n; i++) {
            uint16_t pid = (uint16_t)(p[o] | (p[o + 1] << 8)); o += 2;
            uint8_t nl = p[o++];
            PlayerInfo pi;
            pi.id = pid;
            if (o + nl <= n) { pi.name.assign((char*)p + o, nl); o += nl; }
            players_[pid] = pi;
            snap.push_back(pi);
        }
        Event e;
        e.type = EventType::PlayerList;
        e.players = std::move(snap);
        events_.push_back(std::move(e));
    } else if (type == (uint8_t)M::PLAYER_POS && n >= 2 + 18) {
        uint16_t pid = (uint16_t)(p[0] | (p[1] << 8));
        auto it = players_.find(pid);
        if (it != players_.end()) {
            float v[4];
            for (int i = 0; i < 4; i++) {
                uint32_t u = (uint32_t)p[2 + i * 4] |
                             ((uint32_t)p[3 + i * 4] << 8) |
                             ((uint32_t)p[4 + i * 4] << 16) |
                             ((uint32_t)p[5 + i * 4] << 24);
                memcpy(&v[i], &u, 4);
            }
            it->second.x = v[0]; it->second.y = v[1];
            it->second.z = v[2]; it->second.heading = v[3];
            it->second.has_pos = true;
        }
    } else if (type == (uint8_t)M::PONG) {
        last_pong_ms_ = now_ms();
    } else if (type == (uint8_t)M::NUI_MESSAGE && n >= 1) {
        Event e;
        e.type = EventType::NuiMessage;
        e.text.assign((char*)p, n);
        events_.push_back(std::move(e));
    } else if (type == (uint8_t)M::NUI_FOCUS && n >= 1) {
        Event e;
        e.type = EventType::NuiFocus;
        e.from = p[0];
        events_.push_back(std::move(e));
    }
}

Event Client::NextEvent() {
    Event e = std::move(events_.front());
    events_.pop_front();
    return e;
}

}  // namespace romppc
