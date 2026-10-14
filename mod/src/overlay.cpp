// ============================================================
// Overlay ImGui — implementasi.
// Browser (LAN/Internet/Direct) + chat + nametag 3D->2D.
// ============================================================
#include "overlay.h"
#include "romp_mod.h"
#include "net_worker.h"
#include "remote_peds.h"

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <shlobj.h>

namespace rompmod {
namespace {

bool g_imgui_init = false;
bool g_show_browser = true;
bool g_show_chat = true;
WNDPROC g_orig_wndproc = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK WndProcHook(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    // ImGui dulu; kalau di-handle (klik di window), jangan teruskan ke game.
    if (g_imgui_init && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) {
        // Tetap teruskan ke game kecuali input mouse di atas window ImGui,
        // supaya game tidak freeze saat klik UI.
        if (ImGui::GetIO().WantCaptureMouse &&
            (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP ||
             msg == WM_RBUTTONDOWN || msg == WM_MOUSEMOVE))
            return 0;
    }
    return CallWindowProc(g_orig_wndproc, hwnd, msg, wp, lp);
}
char g_nick[25] = "Player";
char g_direct[128] = "127.0.0.1:7777";
char g_master[128] = "127.0.0.1";
int g_master_port = 7778;
int g_tab = 0;
std::vector<BrowserServer> g_servers;
int g_active = -1;
bool g_loading = false;
char g_chat_input[129] = "";
bool g_chat_open = false;  // input aktif (T untuk buka)

// --- Favorit (simpan di %APPDATA%/RO-MP-PC/mod_favorites.txt, "ip:port") ---
static std::string ModFavPath() {
    char appdata[MAX_PATH] = "";
    if (FAILED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata)))
        return "";
    return std::string(appdata) + "\\RO-MP-PC\\mod_favorites.txt";
}
static std::vector<BrowserServer> g_favs;

static void LoadModFavs() {
    g_favs.clear();
    std::string p = ModFavPath();
    if (p.empty()) return;
    FILE* f = fopen(p.c_str(), "r");
    if (!f) return;
    char line[160];
    while (fgets(line, sizeof(line), f)) {
        std::string s = line;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
            s.pop_back();
        auto c = s.find(':');
        if (c == std::string::npos) continue;
        BrowserServer e;
        e.ip = s.substr(0, c);
        e.port = (uint16_t)atoi(s.substr(c + 1).c_str());
        e.hostname = e.ip + ":" + std::to_string(e.port);
        if (!e.ip.empty() && e.port) g_favs.push_back(e);
    }
    fclose(f);
}

static void SaveModFavs() {
    std::string p = ModFavPath();
    if (p.empty()) return;
    FILE* f = fopen(p.c_str(), "w");
    if (!f) return;
    for (auto& e : g_favs) fprintf(f, "%s:%d\n", e.ip.c_str(), e.port);
    fclose(f);
}

static bool IsModFav(const std::string& ip, uint16_t port) {
    for (auto& e : g_favs)
        if (e.ip == ip && e.port == port) return true;
    return false;
}

static void ToggleModFav(const BrowserServer& s) {
    for (auto it = g_favs.begin(); it != g_favs.end(); ++it) {
        if (it->ip == s.ip && it->port == s.port) {
            g_favs.erase(it);
            SaveModFavs();
            return;
        }
    }
    g_favs.push_back(s);
    SaveModFavs();
}

ImVec4 ChatCol(int c) {
    switch (c) {
        case 1: return {0.6f, 0.6f, 0.6f, 1};
        case 2: return {0.3f, 1.0f, 0.3f, 1};
        case 3: return {1.0f, 0.85f, 0.3f, 1};
        case 4: return {1.0f, 0.35f, 0.35f, 1};
        case 5: return {0.4f, 0.85f, 1.0f, 1};
        default: return {1, 1, 1, 1};
    }
}

void RefreshLan() {
    g_loading = true;
    // jalan di game thread tapi blocking 6 dtk — acceptable untuk v1;
    // TODO: pindah ke thread.
    std::vector<BrowserServer> out;
    romppc::BrowserListenLan(out, 6000);
    g_servers = out;
    g_active = -1;
    g_loading = false;
}

void RefreshNet() {
    g_loading = true;
    std::vector<BrowserServer> list;
    std::vector<BrowserServer> out;
    if (romppc::BrowserFetchMaster(g_master, (uint16_t)g_master_port, list,
                                   4000)) {
        for (auto& e : list) {
            romppc::BrowserServer q;
            std::vector<romppc::BrowserPlayer> pl;
            if (romppc::BrowserQuery(e.ip, e.port, q, pl, 1200))
                out.push_back(q);
            else
                out.push_back(e);
        }
    }
    g_servers = out;
    g_active = -1;
    g_loading = false;
}

void DrawBrowser() {
    Mod& mod = Mod::Get();
    ImGui::SetNextWindowSize(ImVec2(560, 420), ImGuiCond_FirstUseEver);
    ImGui::Begin("RO-MP Server Browser", &g_show_browser);

    ImGui::InputText("Nick", g_nick, sizeof(g_nick));
    if (ImGui::Button("LAN")) { g_tab = 0; RefreshLan(); }
    ImGui::SameLine();
    if (ImGui::Button("Internet")) { g_tab = 1; RefreshNet(); }
    ImGui::SameLine();
    if (ImGui::Button("Favorit")) {
        g_tab = 2;
        LoadModFavs();
        g_servers = g_favs;
        g_active = -1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh") && !g_loading && g_tab != 2) {
        if (g_tab == 0) RefreshLan(); else RefreshNet();
    }
    if (g_tab == 1) {
        ImGui::InputText("Master", g_master, sizeof(g_master));
        ImGui::InputInt("Port", &g_master_port);
    }

    ImGui::Separator();
    if (g_loading) {
        ImGui::Text("Mencari server...");
    } else if (g_servers.empty()) {
        ImGui::Text("(kosong — klik LAN/Internet/Refresh)");
    } else {
        if (ImGui::BeginListBox("##servers", ImVec2(-1, 220))) {
            for (int i = 0; i < (int)g_servers.size(); i++) {
                auto& s = g_servers[i];
                char row[256];
                snprintf(row, sizeof(row), "%s%s (%d/%d) %s:%d",
                         s.has_password ? "[K] " : "", s.hostname.c_str(),
                         s.players, s.maxplayers, s.ip.c_str(), s.port);
                if (ImGui::Selectable(row, g_active == i))
                    g_active = i;
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    g_active = i;
                    mod.pending_host = s.ip;
                    mod.pending_port = s.port;
                    mod.pending_nick = g_nick;
                    mod.want_connect = true;
                    g_show_browser = false;
                }
            }
            ImGui::EndListBox();
        }
    }

    if (g_active >= 0 && g_active < (int)g_servers.size()) {
        auto& s = g_servers[g_active];
        if (ImGui::Button("CONNECT")) {
            mod.pending_host = s.ip;
            mod.pending_port = s.port;
            mod.pending_nick = g_nick;
            mod.want_connect = true;
            g_show_browser = false;
        }
        ImGui::SameLine();
        bool fav = IsModFav(s.ip, s.port);
        if (ImGui::Button(fav ? "Hapus Favorit" : "Tambah Favorit")) {
            ToggleModFav(s);
            if (g_tab == 2) {  // refresh tab favorit
                LoadModFavs();
                g_servers = g_favs;
                g_active = -1;
            }
        }
    }

    ImGui::Separator();
    ImGui::InputText("Direct IP:port", g_direct, sizeof(g_direct));
    ImGui::SameLine();
    if (ImGui::Button("Go")) {
        std::string v = g_direct;
        auto c = v.find(':');
        if (c != std::string::npos) {
            mod.pending_host = v.substr(0, c);
            mod.pending_port = (uint16_t)atoi(v.substr(c + 1).c_str());
            mod.pending_nick = g_nick;
            mod.want_connect = true;
            g_show_browser = false;
        }
    }
    if (!mod.status.empty()) {
        ImGui::TextWrapped("%s", mod.status.c_str());
    }
    ImGui::End();
}

void DrawChat() {
    Mod& mod = Mod::Get();
    ImGui::SetNextWindowPos(ImVec2(16, 16), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_FirstUseEver);
    ImGui::Begin("Chat", &g_show_chat,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    ImGui::BeginChild("log", ImVec2(0, -28), false,
                      ImGuiWindowFlags_HorizontalScrollbar);
    for (auto& l : mod.chat)
        ImGui::TextColored(ChatCol(l.color), "%s", l.text.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20)
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
    if (g_chat_open) {
        ImGui::SetKeyboardFocusHere();
        if (ImGui::InputText("##in", g_chat_input, sizeof(g_chat_input),
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            std::string msg = g_chat_input;
            g_chat_input[0] = 0;
            g_chat_open = false;
            if (!msg.empty()) {
                NetWorker::Get().SendChat(msg);
                if (msg[0] != '/')
                    mod.AddChat(std::string(g_nick) + ": " + msg, 0);
            }
        }
    } else {
        ImGui::TextDisabled("T = chat");
    }
    ImGui::End();

    // T toggle chat input (saat game window fokus & tidak ngetik di browser)
    if (ImGui::IsKeyPressed(ImGuiKey_T) && !g_show_browser &&
        !ImGui::IsAnyItemActive()) {
        g_chat_open = !g_chat_open;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        g_chat_open = false;
    }
}

// Nametag: proyeksi posisi ped ke layar via matriks D3D.
void DrawNametags(IDirect3DDevice9* device) {
    D3DVIEWPORT9 vp;
    D3DMATRIX view, proj;
    if (FAILED(device->GetViewport(&vp))) return;
    if (FAILED(device->GetTransform(D3DTS_VIEW, &view))) return;
    if (FAILED(device->GetTransform(D3DTS_PROJECTION, &proj))) return;

    for (auto& kv : RemotePeds::Get().peds) {
        auto* ped = kv.second.ped;
        if (!ped) continue;
        CVector pos = ped->GetPosition();
        pos.z += 1.2f;  // di atas kepala
        // transform manual world -> clip (hindari D3DX dependency)
        float vx = pos.x, vy = pos.y, vz = pos.z;
        float cx = vx * view._11 + vy * view._21 + vz * view._31 + view._41;
        float cy = vx * view._12 + vy * view._22 + vz * view._32 + view._42;
        float cz = vx * view._13 + vy * view._23 + vz * view._33 + view._43;
        float cw = vx * view._14 + vy * view._24 + vz * view._34 + view._44;
        float px = cx * proj._11 + cy * proj._21 + cz * proj._31 + cw * proj._41;
        float py = cx * proj._12 + cy * proj._22 + cz * proj._32 + cw * proj._42;
        float pw = cx * proj._14 + cy * proj._24 + cz * proj._34 + cw * proj._44;
        if (pw <= 0.01f) continue;
        float sx = vp.X + (1.0f + px / pw) * vp.Width * 0.5f;
        float sy = vp.Y + (1.0f - py / pw) * vp.Height * 0.5f;
        ImGui::GetBackgroundDrawList()->AddText(
            ImVec2(sx - 30, sy), IM_COL32(77, 227, 200, 255),
            kv.second.name.c_str());
    }
}

}  // namespace

void Overlay_SetShowBrowser(bool show) { g_show_browser = show; }

void Overlay_OnReset() {
    if (g_imgui_init) ImGui_ImplDX9_InvalidateDeviceObjects();
}

void Overlay_OnResetDone(IDirect3DDevice9* device) {
    if (g_imgui_init) ImGui_ImplDX9_CreateDeviceObjects();
    (void)device;
}

void OnFrame(IDirect3DDevice9* device) {
    if (!g_imgui_init) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        HWND hwnd = FindWindowA("Grand Theft Auto: San Andreas", nullptr);
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX9_Init(device);
        // Hook WndProc supaya input masuk ke ImGui.
        if (hwnd && !g_orig_wndproc) {
            g_orig_wndproc = (WNDPROC)SetWindowLongPtr(
                hwnd, GWLP_WNDPROC, (LONG_PTR)WndProcHook);
        }
        g_imgui_init = true;
    }
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    Mod& mod = Mod::Get();
    if (g_show_browser && !mod.connected) DrawBrowser();
    if (mod.connected || mod.connecting) {
        if (g_show_chat) DrawChat();
        DrawNametags(device);
    }
    // tombol F7 = browser
    if (ImGui::IsKeyPressed(ImGuiKey_F7)) g_show_browser = !g_show_browser;

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    // --- game thread work (aman di sini: EndScene jalan di main thread) ---
    Overlay_ProcessEvents();
    RemotePeds::Get().Tick();
}

void Overlay_ProcessEvents() {
    Mod& mod = Mod::Get();
    GameEvent ev;
    while (mod.PopEvent(ev)) {
        switch (ev.type) {
            case GameEvent::Connected:
                mod.connected = true;
                mod.connecting = false;
                mod.server_name = ev.text.substr(0, ev.text.find('\n'));
                mod.AddChat("== Terhubung: " + mod.server_name + " ==", 2);
                g_show_browser = false;
                break;
            case GameEvent::Rejected:
                mod.connecting = false;
                mod.status = "Ditolak: " + ev.text;
                mod.AddChat("Ditolak: " + ev.text, 4);
                g_show_browser = true;
                break;
            case GameEvent::Disconnected:
                mod.connected = mod.connecting = false;
                mod.status = ev.text;
                mod.AddChat("Terputus: " + ev.text, 4);
                RemotePeds::Get().Clear();
                g_show_browser = true;
                break;
            case GameEvent::Chat:
                mod.AddChat(ev.text, ev.kind == 1 ? 1 : 0);
                break;
            case GameEvent::PlayerJoin:
                mod.AddChat(">> " + ev.text + " bergabung.", 2);
                RemotePeds::Get().Add(ev.player_id, ev.text);
                break;
            case GameEvent::PlayerLeave:
                mod.AddChat("<< " + ev.text + " keluar.", 1);
                RemotePeds::Get().Remove(ev.player_id);
                break;
            case GameEvent::PlayerList:
                break;
            case GameEvent::PlayerPos:
                RemotePeds::Get().SetPos(ev.player_id, ev.x, ev.y, ev.z);
                break;
            case GameEvent::NuiMessage:
                mod.AddChat("[NUI] " + ev.text.substr(0, 80), 1);
                break;
        }
    }

    // kirim posisi lokal 10Hz
    static uint64_t lastPos = 0;
    uint64_t now = GetTickCount64();
    if (mod.connected && now - lastPos >= 100) {
        lastPos = now;
        RemotePeds::SendLocalPos();
    }

    // request connect/disconnect dari overlay
    if (mod.want_connect) {
        mod.want_connect = false;
        mod.connecting = true;
        mod.status = "Menghubungi...";
        NetWorker::Get().RequestConnect(mod.pending_host, mod.pending_port,
                                        mod.pending_nick);
    }
    if (mod.want_disconnect) {
        mod.want_disconnect = false;
        NetWorker::Get().RequestDisconnect();
    }
}

}  // namespace rompmod
