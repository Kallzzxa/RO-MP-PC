# RO-MP Playable Mod (romp.dll)

Mod **playable** untuk GTA:SA PC — main GTA beneran (jalan, lihat pemain
lain, chat) sambil connect ke server RO-MP. Referensi arsitektur client:
[FlexodMR/SA-MP](https://github.com/FlexodMR/SA-MP) (reverse SA-MP 0.3.7,
dipakai sebagai acuan pola — implementasi di sini ditulis ulang untuk
protokol RO-MP).

**Tidak butuh ASI loader.** `romp.exe` (launcher) otomatis meng-inject
`romp.dll` ke `gta_sa.exe` saat kamu tekan **PLAY** — persis cara kerja
SA-MP (samp.exe + samp.dll).

## Status: ALPHA (belum di-test in-game)

Kode ditulis lengkap dan hati-hati, tapi **belum pernah jalan di dalam
game** (tidak ada GTA:SA di mesin build). Kemungkinan ada bug — laporkan
dan akan di-fix. Jangan ekspektasi langsung sempurna.

## Syarat

- **GTA:SA PC v1.0 US** (gta_sa.exe 1.0 — versi lain belum tentu cocok)
- **Visual Studio 2022** + workload "Desktop development with C++"

## Build

```bat
:: dari "x86 Native Tools Command Prompt" (HARUS 32-bit!)
cd mod
build-windows.bat
```

Hasil: `mod\build\out\Release\romp.dll` → copy ke folder yang sama
dengan `romp.exe`. Launcher akan inject otomatis.

Pertama kali akan clone submodule (plugin-sdk, imgui, minhook) otomatis.

## Cara main

1. Buka `romp.exe` → server browser (kaya open.mp).
2. Pilih server → **PLAY di GTA**.
3. GTA:SA terbuka, `romp.dll` ter-inject, auto-connect ke server.
4. **T** = buka chat, **Enter** = kirim. **F7** = browser (ganti server).

## Cara kerja

```
GTA:SA (game thread)
  └─ D3D9 EndScene hook (MinHook)
       ├─ ImGui overlay: browser + chat + nametag (proyeksi 3D→2D)
       ├─ proses event network → spawn/sync/hapus ped (plugin-sdk)
       └─ kirim posisi lokal 10Hz

NetWorker (thread terpisah)
  └─ PcClient (protokol RO-MP, reuse dari ../src/net — sudah ter-test)
       └─ event queue → game thread
```

- Remote player = `CCivilianPed` (model CJ) via `CWorld::Add`, posisi
  di-interpolasi dari paket `PLAYER_POS` 10Hz.
- Chat & browser 100% ImGui di atas D3D9 — tidak butuh file game tambahan.

## File

| File | Isi |
|---|---|
| `src/dllmain.cpp` | ASI entry |
| `src/romp_mod.h` | state global + event queue |
| `src/net_worker.*` | thread network (PcClient) |
| `src/d3d9_hook.*` | hook Direct3DCreate9 → EndScene/Reset (MinHook) |
| `src/overlay.*` | ImGui: browser, chat, nametag |
| `src/remote_peds.*` | sync ped via plugin-sdk |

## TODO (diketahui)

- Spawn pakai model dinamis per pemain (saat ini semua model CJ)
- Sinkronisasi heading/animasi/state (jalan, lari, duduk)
- Kendaraan (masuk/keluar mobil, sync)
- NUI (WebView) — belum ada di game
- Damage/kesehatan sync
