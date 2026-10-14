// ============================================================
// RO-MP PC — shared app state.
// ============================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "net/pc_client.h"
#include "net/pc_browser.h"

namespace romppc {

enum class Screen { Browser, Game };

struct ChatLine {
    std::string text;
    int color = 0;  // 0 putih, 1 abu, 2 hijau, 3 kuning, 4 merah, 5 cyan
};

struct App {
    Screen screen = Screen::Browser;
    Client client;

    // --- Browser state ---
    char nick[25] = "Player";
    char master_host[128] = "127.0.0.1";
    int master_port = 7778;
    int brow_tab = 0;  // 0 LAN, 1 Internet, 2 Favorit, 3 Direct
    std::vector<BrowserServer> servers;
    std::vector<BrowserPlayer> srv_players;
    int srv_active = -1;
    bool loading = false;
    std::string status = "Siap.";
    char direct_ip[128] = "";
    char gta_path[260] = "";  // folder GTA:SA (Windows, untuk PLAY)
    bool gta_path_known = false;

    // --- Game state ---
    std::vector<ChatLine> chat;
    char chat_input[129] = "";
    bool chat_edit = false;
    std::string srv_name;
    std::string srv_motd;

    void AddChat(const std::string& t, int color = 0) {
        chat.push_back({t, color});
        if (chat.size() > 300) chat.erase(chat.begin());
    }
};

const char* ConfigPath(const char* name);

}  // namespace romppc
