// ============================================================
// RO-MP PC — layar server browser (raylib + raygui).
// Tab: LAN (beacon, tanpa VPS) | Internet (master) | Favorit | Direct.
// ============================================================
#include "../app.h"

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <cstdio>
#include <cstring>
#include <thread>

namespace romppc {
namespace {

// --- Favorites: file teks "host:port # hostname" ---
std::string FavPath() { return ConfigPath("favorites.txt"); }

std::vector<BrowserServer> LoadFavs() {
    std::vector<BrowserServer> v;
    FILE* f = fopen(FavPath().c_str(), "r");
    if (!f) return v;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char ip[128] = ""; int port = 0; char hn[128] = "";
        if (sscanf(line, "%127[^:]:%d # %127[^\n]", ip, &port, hn) >= 2) {
            BrowserServer e;
            e.ip = ip; e.port = (uint16_t)port;
            e.hostname = hn[0] ? hn : (e.ip + ":" + std::to_string(port));
            v.push_back(e);
        }
    }
    fclose(f);
    return v;
}

void SaveFavs(const std::vector<BrowserServer>& v) {
    FILE* f = fopen(FavPath().c_str(), "w");
    if (!f) return;
    for (auto& e : v)
        fprintf(f, "%s:%d # %s\n", e.ip.c_str(), e.port, e.hostname.c_str());
    fclose(f);
}

bool IsFav(const std::vector<BrowserServer>& f, const BrowserServer& e) {
    for (auto& x : f)
        if (x.ip == e.ip && x.port == e.port) return true;
    return false;
}

void ToggleFav(std::vector<BrowserServer>& f, const BrowserServer& e) {
    for (size_t i = 0; i < f.size(); i++) {
        if (f[i].ip == e.ip && f[i].port == e.port) {
            f.erase(f.begin() + i);
            SaveFavs(f);
            return;
        }
    }
    f.push_back(e);
    SaveFavs(f);
}

}  // namespace

// Dipanggil sekali saat masuk layar browser.
void BrowserEnter(App& app) {
    app.servers.clear();
    app.srv_active = -1;
    app.srv_players.clear();
    app.status = "Siap.";
    // Load nickname tersimpan.
    FILE* f = fopen(ConfigPath("nick.txt"), "r");
    if (f) {
        if (fgets(app.nick, sizeof(app.nick), f)) {
            size_t l = strlen(app.nick);
            while (l && (app.nick[l-1] == '\n' || app.nick[l-1] == '\r'))
                app.nick[--l] = 0;
        }
        fclose(f);
    }
    auto favs = LoadFavs();
    if (app.brow_tab == 2) app.servers = favs;
}

static void RefreshAsync(App& app, std::vector<BrowserServer> favs) {
    app.loading = true;
    app.status = "Mencari server…";
    int tab = app.brow_tab;
    std::string mh = app.master_host;
    int mp = app.master_port;
    std::thread([&app, tab, mh, mp, favs]() {
        std::vector<BrowserServer> out;
        std::string st;
        if (tab == 0) {
            BrowserListenLan(out, 3000);
            st = out.empty()
                ? "Tidak ada server di LAN. Cek WiFi & lan_discovery server."
                : std::to_string(out.size()) + " server ditemukan di LAN.";
        } else if (tab == 1) {
            std::vector<BrowserServer> list;
            if (BrowserFetchMaster(mh, (uint16_t)mp, list, 4000)) {
                // Query info live tiap server.
                for (auto& e : list) {
                    BrowserServer q; std::vector<BrowserPlayer> pl;
                    if (BrowserQuery(e.ip, e.port, q, pl, 1200)) out.push_back(q);
                    else {
                        e.ping_ms = -1;
                        out.push_back(e);
                    }
                }
                st = std::to_string(out.size()) + " server dari master.";
            } else {
                st = "Gagal hubungi master. Cek master_host/port.";
            }
        } else if (tab == 2) {
            out = favs;
            for (auto& e : out) {
                BrowserServer q; std::vector<BrowserPlayer> pl;
                if (BrowserQuery(e.ip, e.port, q, pl, 1200)) e = q;
            }
            st = std::to_string(out.size()) + " favorit.";
        }
        app.servers = out;
        app.srv_active = -1;
        app.srv_players.clear();
        app.status = st;
        app.loading = false;
    }).detach();
}

static void QueryPlayersAsync(App& app) {
    if (app.srv_active < 0 || app.srv_active >= (int)app.servers.size()) return;
    auto e = app.servers[app.srv_active];
    std::thread([&app, e]() {
        BrowserServer q; std::vector<BrowserPlayer> pl;
        if (BrowserQuery(e.ip, e.port, q, pl, 1500)) {
            app.srv_players = pl;
            app.status = std::to_string(pl.size()) + " pemain di " + q.hostname;
        } else {
            app.status = "Server tidak menjawab query.";
        }
    }).detach();
}

void BrowserConnect(App& app, const std::string& ip, uint16_t port) {
    if (app.loading) return;
    std::string nick = app.nick;
    if (nick.empty()) nick = "Player";
    // Simpan nick.
    FILE* f = fopen(ConfigPath("nick.txt"), "w");
    if (f) { fprintf(f, "%s\n", nick.c_str()); fclose(f); }
    app.status = ("Menghubungi " + ip + ":" + std::to_string(port) + "…").c_str();
    if (!app.client.Connect(ip, port, nick)) {
        app.status = "Gagal connect (TCP). Cek IP/port & firewall.";
        return;
    }
    app.chat.clear();
    app.AddChat("Menghubungi " + ip + ":" + std::to_string(port) + "…", 1);
    app.screen = Screen::Game;
}

void DrawBrowser(App& app) {
    int W = GetScreenWidth(), H = GetScreenHeight();
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);

    DrawText("RO-MP PC", 20, 14, 28, {77, 227, 200, 255});
    DrawText("Server Browser", 20, 46, 14, GRAY);

    // Nickname.
    GuiLabel({(float)W - 330, 14, 70, 24}, "Nick:");
    static bool nickEdit = false;
    if (GuiTextBox({(float)W - 260, 12, 160, 28}, app.nick, 24, nickEdit))
        nickEdit = !nickEdit;

    // Tab.
    const char* tabs = "LAN;Internet;Favorit";
    GuiToggleGroup({20, 78, 110, 30}, tabs, &app.brow_tab);

    // Tombol refresh + master host (tab internet).
    if (GuiButton({360, 78, 110, 30}, app.loading ? "..." : "Refresh") &&
        !app.loading) {
        RefreshAsync(app, LoadFavs());
    }
    if (app.brow_tab == 1) {
        GuiLabel({490, 78, 60, 30}, "Master:");
        static bool mhEdit = false;
        if (GuiTextBox({550, 78, 150, 30}, app.master_host, 127, mhEdit))
            mhEdit = !mhEdit;
    }

    // Daftar server.
    float listY = 120, listH = (float)H - 250;
    std::string items;
    for (auto& s : app.servers) {
        char row[256];
        const char* lock = s.has_password ? "[K] " : "";
        int ping = s.ping_ms;
        snprintf(row, sizeof(row), "%s%s (%d/%d) %s:%d %dms", lock,
                 s.hostname.c_str(), s.players, s.maxplayers,
                 s.ip.c_str(), s.port, ping);
        if (!items.empty()) items += ";";
        items += row;
    }
    if (items.empty()) items = "(kosong — klik Refresh)";
    static int scroll = 0;
    GuiListView({20, listY, (float)W - 40, listH}, items.c_str(),
                &scroll, &app.srv_active);

    // Detail: daftar pemain server terpilih.
    float detY = listY + listH + 8;
    if (app.srv_active >= 0 && app.srv_active < (int)app.servers.size()) {
        auto& s = app.servers[app.srv_active];
        DrawText(TextFormat("Pemain di %s:", s.hostname.c_str()),
                 20, (int)detY, 14, LIGHTGRAY);
        std::string pl;
        for (auto& p : app.srv_players) {
            if (!pl.empty()) pl += ";";
            pl += p.name;
        }
        static int pscroll = 0; static int pactive = -1;
        if (pl.empty()) pl = "(klik Lihat Pemain)";
        GuiListView({20, detY + 20, (float)W - 40, 56}, pl.c_str(),
                    &pscroll, &pactive);
        if (GuiButton({(float)W - 250, detY - 4, 110, 26}, "Lihat Pemain"))
            QueryPlayersAsync(app);
        // Favorit toggle.
        auto favs = LoadFavs();
        const char* favLabel = IsFav(favs, s) ? "- Favorit" : "+ Favorit";
        if (GuiButton({(float)W - 130, detY - 4, 110, 26}, favLabel)) {
            ToggleFav(favs, s);
            if (app.brow_tab == 2) app.srv_active = -1;
        }
        // Connect.
        if (GuiButton({20, (float)H - 44, 200, 32}, "CONNECT") && !app.loading)
            BrowserConnect(app, s.ip, s.port);
    }

    // Direct connect.
    GuiLabel({240, (float)H - 44, 90, 32}, "Direct IP:");
    static bool dipEdit = false;
    if (GuiTextBox({330, (float)H - 44, 220, 32}, app.direct_ip, 127, dipEdit))
        dipEdit = !dipEdit;
    if (GuiButton({560, (float)H - 44, 110, 32}, "Connect")) {
        std::string v = app.direct_ip;
        auto c = v.find(':');
        if (c != std::string::npos) {
            BrowserConnect(app, v.substr(0, c),
                           (uint16_t)atoi(v.substr(c + 1).c_str()));
        } else {
            app.status = "Format: IP:port (mis. 192.168.1.5:7777)";
        }
    }

    // Status bar.
    DrawText(app.status.c_str(), 20, H - 20, 13, {120, 120, 120, 255});
}

}  // namespace romppc
