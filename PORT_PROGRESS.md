# OpenHoldem → Linux port — progress & resume guide

A faithful port of `c:\www\openholdembot_old` (the Windows MFC OpenHoldem/Hiss
engine) to a stripped-down Linux server build. Replaces the earlier lean API
daemon (backed up at `/var/www/hiss-linux.bak-*`).

## Decisions (locked with the user)
- **MFC handled via a compat shim** (not a CString→std rewrite). Engine compiles
  largely unchanged against `compat/`.
- **Both** a play-loop daemon and an HTTP decision service.
- **Strategy:** load an OpenHoldem/OpenPPL formula file (config path) + bundle a
  minimal default.

## What's built (`/var/www/hiss-linux/`)
- `engine/oh/` — 200+ engine `.cpp/.h` pulled from Windows (Hiss/ + Shared/ +
  CTablemap/ + CTransform/ + DLLs/ + pokereval headers).
- `compat/` — the MFC/Win32 shim:
  - `mfc_string.h` — `CString` on std::string (Format, Find, Mid, Tokenize, …).
  - `mfc_collections.h` — `CArray`, `CMap`, `CStringArray`.
  - `mfc_compat.h` — umbrella: Win32 typedefs, `CObject`/`CArchive`/`CException`,
    `CFile`, `CCriticalSection`, `CRITICAL_SECTION` + no-op API, registry stubs,
    DECLARE/IMPLEMENT macros, `__AFXWIN_H__` (satisfies OpenHoldem.h PCH guard).
  - Stub headers: `afx*.h`, `atl*.h`, `windows.h`, CRT headers, `mousedll.h`,
    `libpq-fe.h` (PostgreSQL no-op), etc.
- `engine/oh/stdafx.h` — Linux version: `#include "mfc_compat.h"` +
  `"MagicNumbers.h"` (original saved as `stdafx.h.win`).
- Include normalization applied: backslash→/, paths flattened to basename, and
  case-mismatch symlinks (Windows was case-insensitive; Linux isn't).
- `engine/_stripped/` — Windows-only files moved out (the WinHTTP CScarletBeast +
  its symbol-engine/menu/lobby).

## Build / measure
Per-file syntax check: `g++ -std=c++17 -fsyntax-only -w -I compat -I engine/oh <f>`.
Bulk harness: `/tmp/bulk.sh` → writes `PASS=/FAIL=` and `/tmp/failed_files.txt`.

## Remaining (the grind) — in leverage order
1. Keep growing the shim from the **top-error tally** (each common error unblocks
   a wave). Last high-leverage items resolved: `CRITICAL_SECTION`,
   `InitializeCriticalSectionAndSpinCount`. Next up:
   - poker-eval consumers need `poker_defs.h`/`deck_std.h` types visible
     (`CardMask`, `Suit_CLUBS`, `HandVal`, `bitcount`).
   - `Preferences()` global accessor → stub or real `CPreferences` (it's the
     registry-backed config; replace with a config-file reader).
   - missing GUI headers (`SizerBar.h`, `PokerChat.hpp`) → strip those files.
   - `pokertracker_query_definitions.h` → stub (PokerTracker DB).
2. Decide the **strip set** (GUI/scraper/autoplayer ~42 files) vs keep+stub. The
   symbol engines read scraper output — bridge that to the API (see below).
3. Link the kept objects; resolve undefined symbols with stubs.
4. Wire **poker-eval** (`pokereval/*.c`) for hand strength.
5. **Bridge the input**: feed poker.scarletbeast.com API table state into the
   symbol engines (replace `CScraper`). Port the WinHTTP client to **libcurl**.
6. Build **both**: a play-loop daemon (`src/daemon`) + an HTTP decision service
   (`src/http`); add a `systemd` unit.
7. Bundle a default OpenPPL formula in `strategy/`; load a file via config.

## Notes
- Total ~204 source files; ~12 compile clean so far. The remaining are layered
  errors — fix the common root, re-measure, repeat.
- The original Windows code at `c:\www\openholdembot_old` is **untouched**.
