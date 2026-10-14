# Reverse Engineering Notes — GTA SA 1.0 US (HOODLUM)

Clean-room research notes supporting the RO-MP playable client (`mod/`).
These documents record **our own observations**: function addresses,
struct layouts, and behavioral descriptions derived from analyzing a
legally owned copy of the game. No copyrighted code is reproduced here.

## Methodology

- Binary analyzed with Ghidra (headless) + [re-agent](https://github.com/Dryxio/reagent)
  (AI-assisted reconstruction with checker/verifier/parity gates).
- Each function entry lists: address (1.0 US HOODLUM), observed signature,
  behavior summary, and cross-references to public community knowledge
  (plugin-sdk, GTA modding wiki) where applicable.
- Findings are written from scratch in our own words; decompiler output
  is used as evidence, never copied verbatim.

## Contents

- `functions/` — per-function notes (`0x<address>-<name>.md`)
- `structs.md` — struct/class layouts (offsets, sizes)
- `globals.md` — notable global variables/pools

## Status

Work in progress. See git log for added entries.
