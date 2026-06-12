# Hiss-Linux daemon

Two entrypoints share the engine objects:

- **`engined.cpp` + `server.cpp`** — the faithful-engine daemon (default `build/hiss`).
  Boots the OpenHoldem engine (memory pools, table state, ~50 symbol engines,
  formula parser) and serves an HTTP decision API:
  - `GET  /health` → `{"status":"ok","engine":"booted"}`
  - `GET|POST /decide` → `{"action":..., "table_state":..., "f$fold":..., ...}`
  Split in two TUs so the MFC compat shim (engined) and POSIX sockets (server)
  never collide. Run: `build/hiss [port]` (default 8087).

- **`main.cpp` + `api.hpp`/`brain.hpp`** — the older lean, API-native daemon
  (no OpenHoldem engine; a fresh REST/GraphQL client + heuristic brain).

## Status / next
`/decide` returns the engine's default decision surface safely. Real,
context-aware decisions need the **table-state bridge**: populate `CTableState`
(hole cards, board, seated/active bits, userchair, bets, stacks, betround) from
the API/request, set `g_table_state_ready`, then `EvaluateAll()` runs the full
symbol-engine → poker-eval → formula → action pipeline.
