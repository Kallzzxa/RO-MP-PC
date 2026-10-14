# RO-MP PC

**RO-MP ala SA-MP untuk PC** — launcher + client mod, persis cara kerja SA-MP:

| File | Fungsi |
|---|---|
| `romp-setup.exe` | Installer (NSIS) — install ke Program Files, deteksi folder GTA:SA |
| `romp.exe` | Launcher + server browser (kaya open.mp) |
| `romp.dll` | Client mod — di-inject otomatis ke `gta_sa.exe` saat tekan PLAY |

Tidak butuh ASI loader. `romp.exe` meluncurkan GTA:SA dan meng-inject
`romp.dll` sendiri (CreateRemoteThread + LoadLibrary, seperti samp.exe).

Butuh: **GTA:SA PC v1.0 US**.

## Download (releases)

Ambil `romp-setup.exe` dari [releases](../../releases) — install, buka
RO-MP dari desktop, pilih server, tekan **PLAY di GTA**.

## Build manual

**Launcher** (`romp.exe`) — Windows & Linux:

```bash
# Linux:
sudo apt install cmake g++ git libx11-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libxi-dev mesa-common-dev
./build-linux.sh && ./build/romp
```
```bat
:: Windows (x64 Native Tools):
build-windows.bat
```

**Mod** (`romp.dll`) — Windows 32-bit, MSVC:

```bat
:: dari "x86 Native Tools Command Prompt" (HARUS 32-bit!)
cd mod
build-windows.bat
:: -> mod\build\out\Release\romp.dll (taruh di folder yang sama dengan romp.exe)
```

**Installer** (`romp-setup.exe`) — butuh NSIS 3.x + hasil build di atas:

```bat
cd installer
build-installer.bat
```

## Cara pakai

1. Buka `romp.exe` → server browser: **LAN** (otomatis, tanpa VPS),
   **Internet** (master), **Favorit**, atau Direct `IP:port`.
2. Pilih server → **PLAY di GTA** → GTA:SA terbuka + auto-connect.
   (Tombol **CONNECT** = mode lite, chat saja tanpa game.)
3. Di dalam game: **T** = chat, **Enter** = kirim, **F7** = browser.

Isi folder GTA:SA sekali di bagian bawah browser (atau otomatis
terdeteksi / dari installer).

## Struktur

```
src/           launcher romp.exe: browser + chat lite + PLAY (inject)
  pc_launch.*  luncurkan gta_sa.exe + inject romp.dll (Windows)
mod/           romp.dll: playable mod (D3D9 hook + ImGui + plugin-sdk)
  src/         dllmain, net_worker, d3d9_hook, overlay, remote_peds
installer/     romp-setup.nsi (NSIS) + build script
tests/         test headless net core
```

## Catatan

- Mod (`romp.dll`) masih **alpha** — belum di-test in-game.
- NUI (WebView) belum dirender di PC.
- Protokol mengikuti `server/PROTOCOL.md` di repo utama.
