// Test headless untuk net core RO-MP PC (tanpa raylib).
// Butuh server RO-MP asli jalan di 127.0.0.1:PORT (argumen).
// Build: g++ -std=c++17 -I../src test_net.cpp ../src/net/pc_client.cpp
//        ../src/net/pc_browser.cpp -o test_net
#include <cstdio>
#include <thread>
#include <chrono>
#include "../src/net/pc_client.h"
#include "../src/net/pc_browser.h"

static int fails = 0;
static void check(bool ok, const char* name) {
    printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) fails++;
}

int main(int argc, char** argv) {
    const char* host = argc > 1 ? argv[1] : "127.0.0.1";
    int port = argc > 2 ? atoi(argv[2]) : 17777;
    using namespace romppc;

    // 1. Master list (butuh master jalan di 17779 kalau mau).
    {
        std::vector<BrowserServer> m;
        bool ok = BrowserFetchMaster("127.0.0.1", 17779, m, 2000);
        printf("[INFO] master: %s (%zu server)\n", ok ? "ok" : "gagal",
               m.size());
    }
    // 2. UDP query langsung (butuh UDP; di sandbox ini diblokir -> NOTE).
    {
        BrowserServer s; std::vector<BrowserPlayer> pl;
        bool ok = BrowserQuery(host, (uint16_t)port, s, pl, 1500);
        if (ok) {
            check(!s.hostname.empty(), "query_hostname");
            printf("[INFO] server: %s (%d/%d) ping %dms\n",
                   s.hostname.c_str(), s.players, s.maxplayers, s.ping_ms);
        } else {
            printf("[NOTE] udp_query: tidak menjawab (UDP mungkin diblokir)\n");
        }
    }
    // 3. Connect + chat + player list + disconnect.
    {
        Client c;
        check(c.Connect(host, (uint16_t)port, "PcTest"), "tcp_connect");
        bool welcomed = false, gotChat = false, gotList = false;
        auto end = std::chrono::steady_clock::now() + std::chrono::seconds(8);
        c.SendChat("halo dari PC test");
        while (std::chrono::steady_clock::now() < end) {
            c.Poll();
            while (c.HasEvent()) {
                Event e = c.NextEvent();
                if (e.type == EventType::Connected) welcomed = true;
                if (e.type == EventType::Chat) gotChat = true;
                if (e.type == EventType::PlayerList) gotList = true;
            }
            if (welcomed && gotChat && gotList) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        check(welcomed, "welcome");
        check(gotChat, "chat_echo");
        check(gotList, "player_list");
        c.SendChat("/players");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        c.Poll();
        c.Disconnect();
        check(!c.IsConnected(), "disconnect");
    }
    printf(fails ? "GAGAL\n" : "SEMUA TEST LOLOS\n");
    return fails ? 1 : 0;
}
