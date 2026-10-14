# 0x563280 — CWorld::Add (validated)

**Status:** validated — re-agent AI reconstruction completed 2026-10-14
(checker PASS, objective verifier PASS, parity GREEN).
Derived from Ghidra headless evidence (`ghidra-export/00563280.json`).

**Binary:** gta_sa.exe 1.0 US (HOODLUM), unmodified `.text`.

## Observed behavior (our own words)

The routine takes a single pointer argument (an entity). It performs three steps:

1. **Virtual dispatch** — it loads the entity's vtable and invokes the method at
   offset `+0x0C`. In public community headers (plugin-sdk) this slot corresponds
   to `CEntity::Add`, the per-entity "register me with the world" hook.
2. **Type filter** — it reads a type byte at entity offset `+0x36`, keeps the low
   three bits (`& 7`), and continues only when the value is 2, 3, or 4.
   Public entity-type enumerations map 2/3/4 to the dynamic classes (vehicle, ped,
   object); static types (building, dummy) are skipped.
3. **List unlink** — for dynamic entities it tail-calls a secondary routine at
   `0x542860`, which *unlinks* the node at entity offset `+0xB4` from a
   doubly-linked list: it patches the list head global when the node is
   first, fixes neighbor `next`/`prev` links, invokes `0x5523D0` on the
   removed node, then zeroes the entity's `+0xB4` slot. (Correction to the
   earlier draft: the evidence shows removal/unlinking here, not insertion.)

In short: *register the entity via its virtual Add(), then, if it is a dynamic
entity type, detach its `+0xB4` list node via `0x542860`.* The unlink-then-add
shape is consistent with re-registering an entity that may already sit in a
sector list (e.g. moving between sectors); the community identification below
is retained as the cross-reference.

## re-agent reconstruction (2026-10-14)

```cpp
#include <cstdint>

void FUN_00542860();

void FUN_00563280(int *param_1)
{
    uint8_t bVar1;

    (**reinterpret_cast<void (**)()>(uintptr_t(*param_1 + 0xc)))();
    bVar1 = *reinterpret_cast<uint8_t*>(uintptr_t(param_1) + 0x36) & 7;
    if ((1 < bVar1) && (bVar1 < 5)) {
        FUN_00542860();
        return;
    }
    return;
}
```

Parameter/return types are evidence-driven placeholders; per plugin-sdk the
real signature is `void CWorld::Add(CEntity *entity)`. The forward declaration
of the tail-called helper is the model's own addition.

| Check | Result |
|---|---|
| Independent checker model | PASS — "accurately matches the Ghidra decompilation and control-flow logic" |
| Objective structural verifier | PASS — no strong mismatches |
| Parity signals (11 built-in) | GREEN |
| Build/test gates | N/A (binary-only target; disabled in config) |
| Semantic equivalence | Not proven — conservative evidence only |

Run: 2 review rounds, 8 model calls, ~56.7k tokens
(`ag/gemini-3.8-flash-medium`, re-agent 0.4.0 + Ghidra 12.1.4).

## Cross-references (public community knowledge)

- plugin-sdk documents `CWorld::Add` at `0x563280` with the same two-phase shape.
- Dryxio's `samp-source` (byte-for-byte SA-MP 0.3.7-R5 rebuild) is the preferred
  verified source for any address we reuse in `mod/` — cross-check before wiring
  into romp.dll.

## Relevance to romp.dll

The playable-client scaffold already references world-entity registration.
When wiring real ped/vehicle spawning, call through this address (or the
plugin-sdk-style wrapper) instead of reimplementing list management —
the engine's own linked-list bookkeeping must stay consistent.

## Validation command (for reference)

```bash
cd ~/workspace/reagent-lab/gta-sa
export RE_AGENT_LLM_API_KEY=$(grep '^BOTKEEP_API_KEY=' ~/.hermes/.env | cut -d= -f2-)
env -u NO_PROXY -u no_proxy \
  ~/workspace/reagent-lab/.venv/bin/re-agent reverse --address 0x563280
```
