// engined.cpp — engine side of the Hiss-Linux daemon.
//
// Boots the faithful OpenHoldem engine and exposes a tiny C interface to the
// socket layer (src/server.cpp). Kept free of system networking/unistd headers
// so the MFC compat shim and libc headers never collide in one translation unit.
#include "stdafx.h"
#include "Singletons.h"
#include "CSessionCounter.h"
#include "CFormulaParser.h"
#include "CEngineContainer.h"
#include "CAutoplayerFunctions.h"
#include "COpenHoldemStatusbar.h"
#include "globals.h"
#include "MagicNumbers.h"
#include <cstdio>

extern "C" void hiss_boot() {
  std::fprintf(stderr, "[hiss] booting engine...\n");
  Preferences()->LoadPreferences();
  if (!p_sessioncounter) p_sessioncounter = new CSessionCounter();
  InstantiateAllSingletons();
  if (p_formula_parser) p_formula_parser->ParseDefaultLibraries();
  if (!p_openholdem_statusbar) p_openholdem_statusbar = new COpenHoldemStatusbar(nullptr);
  if (p_engine_container) p_engine_container->UpdateOnConnection();  // so EvaluateAll() runs
  std::fprintf(stderr, "[hiss] engine booted (pools, table state, symbol engines, formula parser)\n");
}

// One evaluation pass over the current table state -> JSON autoplayer surface.
// Returns a pointer to a static buffer (single-threaded server).
bool g_table_state_ready = false;  // set true by the (forthcoming) table-state bridge

extern "C" const char* hiss_decide() {
  static char buf[640];
  // EvaluateAll() over EMPTY table state dereferences scraper-populated data
  // (e.g. nopponents-1 indexing) — so only run it once the bridge has filled
  // CTableState. Until then report the engine's default decision surface.
  if (g_table_state_ready && p_engine_container) p_engine_container->EvaluateAll();
  auto f = [](int code) -> double {
    return p_autoplayer_functions ? p_autoplayer_functions->GetAutoplayerFunctionValue(code) : 0.0;
  };
  double fold = f(k_autoplayer_function_fold), check = f(k_autoplayer_function_check),
         call = f(k_autoplayer_function_call), raise = f(k_autoplayer_function_raise),
         betsize = f(k_autoplayer_function_betsize), allin = f(k_autoplayer_function_allin);
  const char* action = "none";
  if (allin) action = "allin"; else if (raise) action = "raise";
  else if (call) action = "call"; else if (check) action = "check";
  else if (fold) action = "fold";
  std::snprintf(buf, sizeof(buf),
    "{\"action\":\"%s\",\"table_state\":\"%s\","
    "\"f$fold\":%g,\"f$check\":%g,\"f$call\":%g,"
    "\"f$raise\":%g,\"f$betsize\":%g,\"f$allin\":%g}",
    action, g_table_state_ready ? "populated" : "empty",
    fold, check, call, raise, betsize, allin);
  return buf;
}
