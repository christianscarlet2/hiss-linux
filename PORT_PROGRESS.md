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

## STATUS: engine compiles 147/147 ✅
All engine `.cpp` files pass `g++ -std=c++17 -fpermissive -fsyntax-only` against
the `compat/` shim. The shim now covers the full MFC/Win32/GDI surface the engine
touches (CString/CArray/CMap, CWnd/CDC/CDialog + GDI object classes, the Win32
constant/typedef/function universe, libpq + PokerTracker + OpenCV/Tesseract stubs).
Key reconstructed pieces: `Preferences.h` (real class, found in-tree), synthesized
`prw1326.h`, `CSymbolEngineFormulaLoading.h`, PT-DLL stub. A few MSVC-isms were
patched in-source (HashKey/CompareElements `template<>`, const-ref params, a dead
`(COpenHoldemDoc*)` cast, an unused `using namespace std`, the lowercase
`preferences` → `Preferences()`).

Next: **link** the objects (compile to .o, resolve undefined symbols with stubs),
then wire poker-eval, bridge the API input, build the daemon + HTTP service.

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

## MILESTONE: the engine LINKS + runs on Linux ✅ (`./build.sh` -> `build/hiss`)
The full engine now **compiles (147/147), links into a 5.1 MB ELF, and starts
cleanly** (exit 0). Reproducible via `./build.sh` (poker-eval -> support libs ->
engine/subsystems -> link).

What was needed to link (197 -> 0 unresolved):
- **poker-eval**: pulled `pokereval/*.c` from Windows, built `libpokereval.a`.
- **support libs**: Files, string_functions, debug, globals, window_functions,
  MessageBoxes, Preferences (real `CPreferences`), lookup3 (hashword).
- **CTablemap/CTransform** + ~15 subsystem classes compiled against the shim.
- **headless_stubs.cpp**: the scraper / OCR / autoplayer-widget / chat / MFC
  doc+frame / user-DLL layer — REPLACED by the API feed — stubbed inert. The
  scraper/OCR singletons are NULL (never invoked headless).
- **CScarletBeast** (real 461-line API client) compiles against a `winhttp.h`
  shim; **CSymbolEngineScarletBeast** (sb_connected/sb_pot/sb_to_call/sb_to_act/
  sb_my_seat/sb_table_id) compiles.
- Fixed two real upstream bugs: a duplicate `CSymbolEngineVersusmod.cpp` and a
  copy-paste stray global in `CSymbolengineUserDLL.cpp`.

## API client is LIVE ✅
`compat/winhttp.h` + `compat/winhttp_curl.cpp` implement the WinHTTP API on
**libcurl** — CScarletBeast.cpp fetches table state for real, unchanged. Verified
end-to-end against poker.scarletbeast.com (HTTP 200, live lobby JSON). Build links
`-lcurl`; `build.sh` is 6-way parallel (~45s clean).

## Remaining to a FUNCTIONAL server
2. **Real entrypoint**: replace the trivial `main()` with the play-loop daemon +
   the HTTP decision service; initialise the memory pool + engine container +
   load a formula on startup.
3. Bundle a default OpenPPL formula in `strategy/` + config to load one.

## Table-state bridge: POST /decide -> populated CTableState -> live symbols
`POST /decide` with a seat-view JSON ({hole, board, userchair, dealer, nchairs,
occupied, active, stack, bet, bblind}) populates `CTableState` (hero known cards
+ opponents' card-backs, board, per-seat seated/active/dealer/stack, blind bets)
and runs `EvaluateAll()` + `CalcPrimary/SecondaryFormulas()`. **Verified**: the
engine computes real symbols from the populate — `userchair`, `nplayersdealt`,
`betround` (1 preflop / 2 flop / ... tracks the board), `position`. A bundled
`strategy/default.ohf` is loaded at boot via a now-functional `CArchive`/`CFile`
(reads lines from disk). Symbols are read via `p_engine_container->EvaluateSymbol`
(the path the formula uses), exposed in the `"sym"` debug field of /decide.

Engine edits for headless: `CTablemap::set_nchairs` (seat count without a .tm
file); `CCasinoInterface::IsMyTurn` honours `g_hiss_force_my_turn`; prwin's
iterator call is null-guarded (the Monte-Carlo `p_iterator_thread` is GUI/threads
-init only).

### Last mile — the "my turn" heavy-symbol path
`handrank169` / `prwin` only compute on `UpdateOnMyTurn()`, which spins the
threaded `CIteratorThread` Monte-Carlo and other GUI/thread-coupled code that
isn't initialised headless (crashes / garbage symbol names). So my-turn is
OPT-IN (`HISS_MYTURN=1`); by default the daemon is robust and computes the base
(non-prwin) symbols. Finishing this means standing up the iterator-thread infra
(or a synchronous prwin) so hand-strength symbols fire — then a hand-strength
OpenPPL formula produces full decisions.

## hand-strength symbols WORK; root-caused the formula-parser garbage
With `HISS_MYTURN=1` + a populated table, **`handrank169` computes correctly and
deterministically** — AsAh=1, QQ=3, AKs=11, Ts9h=77, 72o=165. The full
bridge→symbol-engine→poker-eval pipeline produces real hand strength. Fix that
made it deterministic: a one-line clamp in `CSymbolEngineHandrank::CalculateHandrank`
(`handrank_table_169[nopponents-1]` read `[-1]` when prwin hadn't set nopponents
yet — headless has no iterator thread; the OOB read made handrank flaky).

**The last blocker is a CString/varargs bug in the compat shim.** The formula
parser's `CParseErrors::Error` (and much engine logging) calls
`CString::Format("%s%s...", short_message, CTokenizer::CurrentFunctionName(), ...)`
passing **CString objects to `%s`**. MFC's CString is a thin char* wrapper
(trivially copyable → works through `...`); the shim's CString wraps std::string
(non-trivially-copyable → UB through varargs → garbage strings, e.g.
"Error: ����U====", "Unknown symbol x.."). This corrupts parsed symbol names so
`f$` functions don't evaluate (even a constant `f$raise=1` yields 0). FIX: make
`compat/mfc_string.h` CString varargs-/`%s`-compatible — e.g. give it a leading
`const char*` member kept pointing at the data (MFC-style), or have Format route
through a CString-aware formatter. Then formulas parse and produce fold/call/raise.


## ✅ WORKING DECISION ENGINE — real hand-strength decisions
The headless engine now makes **real, deterministic poker decisions** out of the
box. `POST /decide` with table state -> the full pipeline runs (populate
CTableState -> EvaluateAll -> poker-eval handrank -> formula -> autoplayer action):

    AsAh -> raise   QhQd -> raise   AhKs -> raise        (strong: handrank169 <= 40)
    Ts9h -> call                                          (medium: 40 < hr <= 90)
    72o  -> fold                                          (weak:   hr > 90)
    AsKs/Qd7c2s -> raise (flop, betround=2)               (board updates the street)

Stable across 20+ requests; my-turn is default ON (a /decide call IS our turn;
HISS_NOMYTURN=1 to disable).

### The two bugs that unlocked it
1. `CSymbolEngineHandrank` indexed `handrank_table_169[nopponents-1]` with
   nopponents=0 (no iterator thread headless) -> OOB read -> flaky handrank.
   Clamped nopponents to [1, max].
2. **The real blocker**: `CTokenizer::SetInputBuffer` did `input_buffer =
   (char*)formula_text` — aliasing a TEMPORARY `CString::c_str()`. MFC's
   ref-counted COW CString kept that buffer alive; the std::string-backed shim
   copy dangled, so the tokenizer parsed FREED MEMORY -> garbage tokens -> no
   parse tree -> every f$ function evaluated to 0. Fixed by having the tokenizer
   OWN a std::string copy of the input. (Also gave compat CString a leading
   char* for MFC-style %s, though g++'s by-reference varargs passing limits that.)

The engine is a complete headless OpenHoldem decision service: boots, ingests
table state over HTTP (or the live API via the libcurl bridge), and returns
fold/check/call/raise/betsize.
