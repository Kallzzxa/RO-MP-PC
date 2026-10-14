// ============================================================
// RO-MP PC — implementasi server browser.
// Protokol: lihat server/PROTOCOL.md di repo utama.
// ============================================================
#include "pc_browser.h"

#include <chrono>
#include <cstring>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET sock_t;
  #define BAD_SOCK INVALID_SOCKET
  #define CLOSE_SOCK(s) closesocket(s)
  typedef int ssize_t;
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

const char kMagic[4] = {'R', 'O', 'M', 'P'};
const uint16_t kLanPort = 7779;

bool wait_readable(sock_t s, int timeout_ms) {
    fd_set rf; FD_ZERO(&rf); FD_SET(s, &rf);
    timeval tv{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    return select((int)s + 1, &rf, nullptr, nullptr, &tv) > 0;
}

bool wait_writable(sock_t s, int timeout_ms) {
    fd_set wf; FD_ZERO(&wf); FD_SET(s, &wf);
    timeval tv{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    return select((int)s + 1, nullptr, &wf, nullptr, &tv) > 0;
}

sock_t udp_socket() {
    net_init();
    return socket(AF_INET, SOCK_DGRAM, 0);
}

bool resolve(const std::string& host, uint16_t port, sockaddr_in& out) {
    memset(&out, 0, sizeof(out));
    out.sin_family = AF_INET;
    out.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &out.sin_addr) == 1) return true;
    hostent* he = gethostbyname(host.c_str());
    if (!he) return false;
    memcpy(&out.sin_addr, he->h_addr_list[0], (size_t)he->h_length);
    return true;
}

// Kirim query UDP, tunggu reply bertipe want_type. Return payload (tanpa magic+type).
bool udp_query(const std::string& host, uint16_t port, uint8_t qtype,
               uint8_t want_type, std::vector<uint8_t>& payload,
               int timeout_ms, int* ping_ms) {
    sock_t s = udp_socket();
    if (s == BAD_SOCK) return false;
    sockaddr_in a;
    if (!resolve(host, port, a)) { CLOSE_SOCK(s); return false; }
    uint8_t q[5];
    memcpy(q, kMagic, 4); q[4] = qtype;
    auto t0 = std::chrono::steady_clock::now();
    if (sendto(s, (const char*)q, 5, 0, (sockaddr*)&a, sizeof(a)) != 5) {
        CLOSE_SOCK(s); return false;
    }
    uint8_t buf[2048];
    bool ok = false;
    while (std::chrono::steady_clock::now() - t0 <
           std::chrono::milliseconds(timeout_ms)) {
        if (!wait_readable(s, 200)) continue;
        sockaddr_in from{}; socklen_t fl = sizeof(from);
        ssize_t n = recvfrom(s, (char*)buf, sizeof(buf), 0,
                             (sockaddr*)&from, &fl);
        if (n >= 5 && !memcmp(buf, kMagic, 4) && buf[4] == want_type) {
            payload.assign(buf + 5, buf + n);
            ok = true;
            break;
        }
    }
    if (ok && ping_ms) {
        *ping_ms = (int)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t0).count();
    }
    CLOSE_SOCK(s);
    return ok;
}

bool parse_info(const std::vector<uint8_t>& p, BrowserServer& e) {
    // hostnamelen(u8) hostname motdlen(u8) motd versionlen(u8) version
    // players(u16) maxplayers(u16) has_password(u8)
    size_t o = 0;
    if (o + 1 > p.size()) return false;
    uint8_t hl = p[o++];
    if (o + hl + 1 > p.size()) return false;
    e.hostname.assign((char*)p.data() + o, hl); o += hl;
    uint8_t ml = p[o++];
    if (o + ml + 1 > p.size()) return false;
    e.motd.assign((char*)p.data() + o, ml); o += ml;
    uint8_t vl = p[o++];
    if (o + vl + 5 > p.size()) return false;
    e.version.assign((char*)p.data() + o, vl); o += vl;
    e.players = (uint16_t)(p[o] | (p[o + 1] << 8)); o += 2;
    e.maxplayers = (uint16_t)(p[o] | (p[o + 1] << 8)); o += 2;
    e.has_password = p[o] != 0;
    return true;
}

}  // namespace

bool BrowserQuery(const std::string& host, uint16_t port,
                  BrowserServer& out, std::vector<BrowserPlayer>& players,
                  int timeout_ms) {
    out = BrowserServer();
    players.clear();
    out.ip = host; out.port = port;
    std::vector<uint8_t> p;
    int ping = -1;
    if (!udp_query(host, port, 0x01, 0x02, p, timeout_ms, &ping)) return false;
    if (!parse_info(p, out)) return false;
    out.ping_ms = ping;
    out.ip = host; out.port = port;
    // Daftar pemain (opsional, gagal -> tetap OK).
    std::vector<uint8_t> pp;
    if (udp_query(host, port, 0x03, 0x04, pp, timeout_ms / 2, nullptr) &&
        pp.size() >= 1) {
        uint8_t cnt = pp[0];
        size_t o = 1;
        for (int i = 0; i < cnt && o + 3 <= pp.size(); i++) {
            BrowserPlayer bp;
            bp.id = (uint16_t)(pp[o] | (pp[o + 1] << 8)); o += 2;
            uint8_t nl = pp[o++];
            if (o + nl > pp.size()) break;
            bp.name.assign((char*)pp.data() + o, nl); o += nl;
            players.push_back(std::move(bp));
        }
    }
    return true;
}

bool BrowserFetchMaster(const std::string& master_host, uint16_t master_port,
                        std::vector<BrowserServer>& out, int timeout_ms) {
    out.clear();
    net_init();
    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == BAD_SOCK) return false;
    sockaddr_in a;
    if (!resolve(master_host, master_port, a)) { CLOSE_SOCK(s); return false; }
    // Connect dengan timeout.
#ifdef _WIN32
    u_long m = 1; ioctlsocket(s, FIONBIO, &m);
#else
    int fl = fcntl(s, F_GETFL, 0); fcntl(s, F_SETFL, fl | O_NONBLOCK);
#endif
    ::connect(s, (sockaddr*)&a, sizeof(a));
    if (!wait_writable(s, timeout_ms)) { CLOSE_SOCK(s); return false; }
    // Cek error connect via SO_ERROR.
    int se = 0; socklen_t sl = sizeof(se);
    getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&se, &sl);
    if (se != 0) { CLOSE_SOCK(s); return false; }
    // Kirim LIST_REQUEST: [u16 len=1][0x51].
    uint8_t req[3] = {1, 0, 0x51};
    if (::send(s, (const char*)req, 3, 0) != 3) { CLOSE_SOCK(s); return false; }
    // Baca frame jawaban.
    std::vector<uint8_t> buf;
    auto end = std::chrono::steady_clock::now() +
               std::chrono::milliseconds(timeout_ms);
    bool ok = false;
    uint8_t tmp[2048];
    while (std::chrono::steady_clock::now() < end) {
        if (!wait_readable(s, 300)) continue;
        ssize_t n = ::recv(s, (char*)tmp, sizeof(tmp), 0);
        if (n <= 0) break;
        buf.insert(buf.end(), tmp, tmp + n);
        if (buf.size() >= 3) {
            uint16_t len = (uint16_t)(buf[0] | (buf[1] << 8));
            if (len >= 1 && buf.size() >= (size_t)2 + len && buf[2] == 0x52) {
                size_t o = 3;
                if (o + 2 > buf.size()) break;
                uint16_t cnt = (uint16_t)(buf[o] | (buf[o + 1] << 8)); o += 2;
                for (int i = 0; i < cnt; i++) {
                    if (o + 1 > buf.size()) break;
                    uint8_t il = buf[o++];
                    if (o + il + 2 + 2 + 2 + 1 > buf.size()) break;
                    BrowserServer e;
                    e.ip.assign((char*)buf.data() + o, il); o += il;
                    e.port = (uint16_t)(buf[o] | (buf[o + 1] << 8)); o += 2;
                    e.players = (uint16_t)(buf[o] | (buf[o + 1] << 8)); o += 2;
                    e.maxplayers = (uint16_t)(buf[o] | (buf[o + 1] << 8)); o += 2;
                    uint8_t hl = buf[o++];
                    if (o + hl + 1 > buf.size()) break;
                    e.hostname.assign((char*)buf.data() + o, hl); o += hl;
                    uint8_t ml = buf[o++];
                    if (o + ml > buf.size()) break;
                    e.motd.assign((char*)buf.data() + o, ml); o += ml;
                    e.version = "romp1";
                    e.ping_ms = -1;
                    out.push_back(std::move(e));
                }
                ok = true;
                break;
            }
        }
    }
    CLOSE_SOCK(s);
    return ok;
}

bool BrowserListenLan(std::vector<BrowserServer>& out, int timeout_ms) {
    out.clear();
    sock_t s = udp_socket();
    if (s == BAD_SOCK) return false;
    int one = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&one, sizeof(one));
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = INADDR_ANY;
    a.sin_port = htons(kLanPort);
    if (bind(s, (sockaddr*)&a, sizeof(a)) != 0) { CLOSE_SOCK(s); return false; }

    auto end = std::chrono::steady_clock::now() +
               std::chrono::milliseconds(timeout_ms);
    uint8_t buf[512];
    while (std::chrono::steady_clock::now() < end) {
        int remain = (int)std::chrono::duration_cast<std::chrono::milliseconds>(
                         end - std::chrono::steady_clock::now()).count();
        if (remain <= 0) break;
        if (!wait_readable(s, remain > 500 ? 500 : remain)) continue;
        sockaddr_in from{}; socklen_t fl = sizeof(from);
        ssize_t n = recvfrom(s, (char*)buf, sizeof(buf), 0,
                             (sockaddr*)&from, &fl);
        // "ROMP" 0x11 gameport(u16) hostnamelen(u8) hostname
        // players(u16) maxplayers(u16) has_password(u8) motdlen(u8) motd
        if (n < 4 + 1 + 2 + 1 || memcmp(buf, kMagic, 4) || buf[4] != 0x11)
            continue;
        size_t o = 5;
        BrowserServer e;
        char ipstr[48];
        inet_ntop(AF_INET, &from.sin_addr, ipstr, sizeof(ipstr));
        e.ip = ipstr;
        e.port = (uint16_t)(buf[o] | (buf[o + 1] << 8)); o += 2;
        uint8_t hl = buf[o++];
        if (o + hl + 5 > (size_t)n) continue;
        e.hostname.assign((char*)(buf + o), hl); o += hl;
        e.players = (uint16_t)(buf[o] | (buf[o + 1] << 8)); o += 2;
        e.maxplayers = (uint16_t)(buf[o] | (buf[o + 1] << 8)); o += 2;
        e.has_password = buf[o++] != 0;
        uint8_t ml = buf[o++];
        if (o + ml > (size_t)n) continue;
        e.motd.assign((char*)(buf + o), ml); o += ml;
        e.version = "romp1";
        e.ping_ms = 0;
        bool found = false;
        for (auto& x : out) {
            if (x.ip == e.ip && x.port == e.port) { x = e; found = true; break; }
        }
        if (!found) out.push_back(std::move(e));
    }
    CLOSE_SOCK(s);
    return true;
}

}  // namespace romppc
