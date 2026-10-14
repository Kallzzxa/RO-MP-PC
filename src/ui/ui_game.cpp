// ============================================================
// RO-MP PC — layar game: chat + daftar pemain + radar 2D.
// ============================================================
#include "../app.h"

#include "raylib.h"
#include "raygui.h"

#include <cstdio>
#include <cstring>

namespace romppc {
namespace {

Color ChatColor(int c) {
    switch (c) {
        case 1: return GRAY;
        case 2: return GREEN;
        case 3: return YELLOW;
        case 4: return RED;
        case 5: return SKYBLUE;
        default: return RAYWHITE;
    }
}

// Nama pemain dari ID (untuk prefix chat).
std::string NameOf(const App& app, uint16_t pid) {
    if (pid == 0xFFFF) return "";
    auto it = app.client.Players().find(pid);
    if (it != app.client.Players().end()) return it->second.name;
    if (pid == app.client.MyId()) return app.client.MyNick();
    return "pemain#" + std::to_string(pid);
}

void HandleEvent(App& app, Event ev) {
    switch (ev.type) {
        case EventType::Connected:
            app.srv_name = ev.text.substr(0, ev.text.find('\n'));
            app.srv_motd = ev.text.substr(ev.text.find('\n') + 1);
            app.AddChat("== Terhubung ke " + app.srv_name + " ==", 2);
            if (!app.srv_motd.empty()) app.AddChat(app.srv_motd, 1);
            break;
        case EventType::Rejected:
            app.AddChat("Ditolak server: " + ev.text, 4);
            app.screen = Screen::Browser;
            app.status = "Ditolak: " + ev.text;
            break;
        case EventType::Disconnected:
            app.AddChat("Terputus: " + ev.text, 4);
            app.screen = Screen::Browser;
            app.status = "Terputus: " + ev.text;
            break;
        case EventType::Chat: {
            std::string nm = NameOf(app, ev.from);
            std::string line = nm.empty() ? ev.text : nm + ": " + ev.text;
            int col = 0;
            if (ev.kind == 1) col = 1;        // info/abu
            else if (ev.kind == 2) col = 2;  // sukses/hijau
            else if (ev.kind == 3) col = 4;  // error/merah
            app.AddChat(line, col);
            break;
        }
        case EventType::PlayerJoin:
            app.AddChat(">> " + ev.text + " bergabung.", 2);
            break;
        case EventType::PlayerLeave:
            app.AddChat("<< " + ev.text + " keluar.", 1);
            break;
        case EventType::PlayerList:
            app.AddChat("Pemain online: " + std::to_string(ev.players.size()), 5);
            break;
        case EventType::NuiMessage:
            app.AddChat("[NUI] server membuka UI (belum didukung di PC).", 1);
            break;
        case EventType::NuiFocus:
            break;
    }
}

}  // namespace

void GameEnter(App& app) {
    app.chat_input[0] = 0;
    app.chat_edit = false;
}

void DrawGame(App& app) {
    int W = GetScreenWidth(), H = GetScreenHeight();
    GuiSetStyle(DEFAULT, TEXT_SIZE, 15);

    // Poll network.
    app.client.Poll();
    while (app.client.HasEvent()) HandleEvent(app, app.client.NextEvent());
    if (app.screen != Screen::Game) return;  // keputus di tengah frame

    // Header.
    DrawRectangle(0, 0, W, 44, {20, 24, 30, 255});
    DrawText(app.srv_name.empty() ? "RO-MP" : app.srv_name.c_str(),
             14, 8, 20, {77, 227, 200, 255});
    char pc[64];
    snprintf(pc, sizeof(pc), "%d pemain", (int)app.client.Players().size());
    DrawText(pc, 14, 28, 12, GRAY);
    if (GuiButton({(float)W - 120, 8, 106, 30}, "Disconnect")) {
        app.client.Disconnect("Keluar.");
        app.screen = Screen::Browser;
        app.status = "Terputus.";
        return;
    }

    // --- Radar 2D (kiri) ---
    float rx = 14, ry = 54, rw = 300, rh = 300;
    DrawRectangle((int)rx - 2, (int)ry - 2, (int)rw + 4, (int)rh + 4,
                  {40, 46, 54, 255});
    DrawRectangle((int)rx, (int)ry, (int)rw, (int)rh, {13, 17, 23, 255});
    // grid
    for (int i = 1; i < 6; i++) {
        DrawLine((int)(rx + rw * i / 6), (int)ry,
                 (int)(rx + rw * i / 6), (int)(ry + rh), {30, 36, 44, 255});
        DrawLine((int)rx, (int)(ry + rh * i / 6),
                 (int)(rx + rw), (int)(ry + rh * i / 6), {30, 36, 44, 255});
    }
    DrawText("RADAR", (int)rx + 6, (int)ry + 4, 12, {90, 100, 110, 255});
    // San Andreas ~6000x6000 (x,y dalam [-3000,3000]).
    auto toXY = [&](float x, float y) {
        float nx = (x + 3000.0f) / 6000.0f, ny = (y + 3000.0f) / 6000.0f;
        if (nx < 0) nx = 0; if (nx > 1) nx = 1;
        if (ny < 0) ny = 0; if (ny > 1) ny = 1;
        return Vector2{rx + nx * rw, ry + (1 - ny) * rh};
    };
    for (auto& kv : app.client.Players()) {
        auto& p = kv.second;
        if (!p.has_pos) continue;
        Vector2 v = toXY(p.x, p.y);
        bool me = (p.id == app.client.MyId());
        DrawCircle((int)v.x, (int)v.y, me ? 6 : 4,
                   me ? GREEN : SKYBLUE);
        DrawText(p.name.c_str(), (int)v.x + 8, (int)v.y - 6, 10,
                 me ? GREEN : LIGHTGRAY);
    }

    // --- Daftar pemain (bawah radar) ---
    float py = ry + rh + 10, ph = (float)H - py - 90;
    DrawText("PEMAIN", 14, (int)py - 16, 12, {90, 100, 110, 255});
    std::string items;
    for (auto& kv : app.client.Players()) {
        if (!items.empty()) items += ";";
        char row[96];
        snprintf(row, sizeof(row), "%s%s", kv.second.name.c_str(),
                 kv.first == app.client.MyId() ? " (kamu)" : "");
        items += row;
    }
    if (items.empty()) items = "(sendiri)";
    static int pscroll = 0, pactive = -1;
    GuiListView({rx, py, rw, ph}, items.c_str(), &pscroll, &pactive);

    // --- Chat (kanan) ---
    float cx = rx + rw + 16, cw = (float)W - cx - 14;
    float chTop = 54, chH = (float)H - chTop - 70;
    DrawRectangle((int)cx - 2, (int)chTop - 2, (int)cw + 4, (int)chH + 4,
                  {40, 46, 54, 255});
    // 18 baris terakhir.
    int lines = (int)(chH / 20);
    int start = (int)app.chat.size() > lines ? (int)app.chat.size() - lines : 0;
    BeginScissorMode((int)cx, (int)chTop, (int)cw, (int)chH);
    for (int i = start; i < (int)app.chat.size(); i++) {
        float y = chTop + 6 + (i - start) * 20;
        DrawText(app.chat[i].text.c_str(), (int)cx + 8, (int)y, 14,
                 ChatColor(app.chat[i].color));
    }
    EndScissorMode();

    // Input chat.
    float iy = (float)H - 52;
    if (GuiTextBox({cx, iy, cw - 90, 34}, app.chat_input, 128, app.chat_edit))
        app.chat_edit = !app.chat_edit;
    // Enter = kirim.
    if (IsKeyPressed(KEY_ENTER) && app.chat_input[0]) {
        std::string msg = app.chat_input;
        app.chat_input[0] = 0;
        // Echo lokal untuk pesan biasa.
        if (!msg.empty() && msg[0] != '/')
            app.AddChat(app.client.MyNick() + ": " + msg, 0);
        app.client.SendChat(msg);
    }
    if (GuiButton({cx + cw - 80, iy, 80, 34}, "Kirim") && app.chat_input[0]) {
        std::string msg = app.chat_input;
        app.chat_input[0] = 0;
        if (!msg.empty() && msg[0] != '/')
            app.AddChat(app.client.MyNick() + ": " + msg, 0);
        app.client.SendChat(msg);
    }
    if (!app.chat_edit && IsKeyPressed(KEY_T)) app.chat_edit = true;
}

}  // namespace romppc
