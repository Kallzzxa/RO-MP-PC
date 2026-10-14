# RO-MP PC

Standalone **PC client** untuk server [RO-MP](https://github.com/Kallzzxa/gta-reversed-android)
(multiplayer ala SA-MP di atas GTA:SA Android). Tidak butuh file GTA:SA —
langsung jalan di Windows & Linux.

Fitur v1:

- **Server browser** — 3 cara tanpa bingung connect kemana:
  - 📡 **LAN**: server di WiFi yang sama otomatis ketemu (beacon broadcast,
    tanpa VPS/master)
  - 🌐 **Internet**: daftar dari master server
  - ⭐ **Favorit**: tersimpan lokal + connect manual `IP:port`
- **Chat** penuh: kirim/terima pesan, command `/...` (server yang proses)
- **Daftar pemain** + **radar 2D** posisi pemain
- Nickname tersimpan otomatis

## Build

Butuh: CMake 3.16+, compiler C++17, Git. raylib + raygui diunduh otomatis
saat build pertama (FetchContent).

**Linux:**

```bash
# dependency sistem (Debian/Ubuntu):
sudo apt install cmake g++ git libx11-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libxi-dev mesa-common-dev
./build-linux.sh
./build/romp-pc
```

**Windows** (dari "x64 Native Tools Command Prompt" VS 2022):

```bat
build-windows.bat
build\Release\romp-pc.exe
```

## Cara pakai

1. Nyalakan server RO-MP (`server.cfg` — `lan_discovery 1` default nyala).
2. Buka RO-MP PC, isi nick, klik **Refresh** di tab **LAN** — server di
   jaringan yang sama langsung muncul. Atau tab Internet (isi master),
   Favorit, atau Direct `IP:port`.
3. Klik server → **CONNECT**. Chat di kolom kanan, `T` fokus input,
   `Enter` kirim. Command diawali `/` (mis. `/players`, `/help`).

## Struktur

```
src/
  main.cpp            entry + state machine layar
  app.h/.cpp          state global, path config per-OS
  net/
    romp_protocol.h   protokol RO-MP v1 (disalin dari repo utama)
    pc_client.h/.cpp  TCP client: HELLO/WELCOME, chat, pemain, ping
    pc_browser.h/.cpp server browser: LAN beacon, UDP query, master list
  ui/
    ui_browser.cpp    layar browser (raylib + raygui)
    ui_game.cpp       layar game: chat + daftar pemain + radar
tests/
  test_net.cpp        test headless net core (tanpa GUI)
```

## Test

```bash
# butuh server + master test jalan (lihat repo utama):
g++ -std=c++17 -I src tests/test_net.cpp src/net/pc_client.cpp \
    src/net/pc_browser.cpp -o /tmp/test_net
/tmp/test_net 127.0.0.1 17777
```

## Catatan

- NUI (WebView ala FiveM) belum dirender di PC — server yang membuka UI
  memunculkan notifikasi di chat.
- Protokol mengikuti `server/PROTOCOL.md` di repo utama. Kalau protokol
  berubah, salin ulang `romp_protocol.h` dari sana.
