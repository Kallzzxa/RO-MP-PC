// ============================================================
// RO-MP PC — standalone client untuk server RO-MP.
// Layar: Browser (LAN/Internet/Favorit/Direct) -> Game (chat+radar).
// Tidak butuh file GTA:SA. Build: lihat build-linux.sh / .bat.
// ============================================================
#include "raylib.h"
#include "app.h"

namespace romppc {
// Didefinisikan di ui_*.cpp
void BrowserEnter(App& app);
void DrawBrowser(App& app);
void GameEnter(App& app);
void DrawGame(App& app);
}  // namespace romppc

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(960, 640, "RO-MP PC");
    SetTargetFPS(60);
    SetExitKey(0);  // ESC jangan langsung tutup (biar disconnect rapi)

    romppc::App app;
    romppc::BrowserEnter(app);
    bool wasBrowser = true;

    while (!WindowShouldClose()) {
        bool isBrowser = (app.screen == romppc::Screen::Browser);
        if (isBrowser && !wasBrowser) romppc::BrowserEnter(app);
        if (!isBrowser && wasBrowser) romppc::GameEnter(app);
        wasBrowser = isBrowser;

        BeginDrawing();
        ClearBackground({11, 13, 17, 255});
        if (isBrowser) romppc::DrawBrowser(app);
        else romppc::DrawGame(app);
        EndDrawing();
    }

    app.client.Disconnect("Keluar.");
    CloseWindow();
    return 0;
}
