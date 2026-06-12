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
#include "CFunctionCollection.h"
#include "COpenHoldemStatusbar.h"
#include "globals.h"
#include "MagicNumbers.h"
#include "CardFunctions.h"
#include "CTableState.h"
#include "CPlayer.h"
#include "CTablemap.h"
#include <string>
#include <cstdlib>
#include <cstdio>

extern "C" void hiss_boot() {
  std::fprintf(stderr, "[hiss] booting engine...\n");
  Preferences()->LoadPreferences();
  if (!p_sessioncounter) p_sessioncounter = new CSessionCounter();
  InstantiateAllSingletons();
  if (p_formula_parser) p_formula_parser->ParseDefaultLibraries();
  // Load the bundled strategy formula (path overridable via HISS_FORMULA).
  {
    const char* fp = getenv("HISS_FORMULA");
    CString path = fp ? fp : "strategy/default.ohf";
    CFile file;
    if (p_formula_parser && file.Open(path.GetString(), CFile::modeRead | CFile::typeText)) {
      CArchive ar(&file, CArchive::load);
      p_formula_parser->ParseFormulaFileWithUserDefinedBotLogic(ar);
      std::fprintf(stderr, "[hiss] strategy formula loaded: %s\n", path.GetString());
    } else {
      std::fprintf(stderr, "[hiss] no strategy formula (%s) — decisions will be default\n", path.GetString());
    }
  }
  if (!p_openholdem_statusbar) p_openholdem_statusbar = new COpenHoldemStatusbar(nullptr);
  if (p_engine_container) p_engine_container->UpdateOnConnection();  // so EvaluateAll() runs
  std::fprintf(stderr, "[hiss] engine booted (pools, table state, symbol engines, formula parser)\n");
}

// One evaluation pass over the current table state -> JSON autoplayer surface.
// Returns a pointer to a static buffer (single-threaded server).

// ------------------------------------------------- minimal JSON field readers
// Good enough for the flat seat-view schema we accept (no nested-escape edge
// cases in our own payloads). Returns "" / default when the key is absent.
static std::string JStr(const std::string& s, const std::string& key) {
  std::string needle = "\"" + key + "\"";
  size_t p = s.find(needle); if (p == std::string::npos) return "";
  p = s.find(':', p + needle.size()); if (p == std::string::npos) return "";
  ++p; while (p < s.size() && (s[p]==' '||s[p]=='\t'||s[p]=='\n')) ++p;
  if (p < s.size() && s[p]=='"') { size_t q = s.find('"', ++p); return s.substr(p, q==std::string::npos?0:q-p); }
  return "";
}
static double JNum(const std::string& s, const std::string& key, double dflt) {
  std::string needle = "\"" + key + "\"";
  size_t p = s.find(needle); if (p == std::string::npos) return dflt;
  p = s.find(':', p + needle.size()); if (p == std::string::npos) return dflt;
  ++p; while (p < s.size() && (s[p]==' '||s[p]=='\t')) ++p;
  size_t start = p; if (p<s.size() && (s[p]=='-'||s[p]=='+')) ++p;
  bool any=false; while (p<s.size() && ((s[p]>='0'&&s[p]<='9')||s[p]=='.')) { ++p; any=true; }
  return any ? atof(s.substr(start, p-start).c_str()) : dflt;
}

// Set a Card from a 2-char string like "As" (no-op on empty/"??").
static void SetCard(Card* c, const std::string& cs) {
  if (!c) return;
  if (cs.size() < 2 || cs[0]=='?') { c->ClearValue(); return; }
  char tmp[3] = { cs[0], cs[1], 0 };
  c->SetValue(CardStringToCardNumber(tmp));
}

// Populate CTableState from a seat-view JSON body. Schema (all optional):
//   { "hole":"AsKh", "board":"Qd7c2s", "userchair":2, "dealer":0,
//     "nchairs":6, "occupied":"111111", "active":"101101",
//     "stack":50.0, "bet":0.0 }
// 'occupied'/'active' are per-seat bit strings (char '1' = yes), left->right by
// chair. The user's hole cards go on 'userchair' (that is how the engine derives
// who the hero is). Returns true if enough state to evaluate.
static bool PopulateTableState(const std::string& body) {
  if (!p_table_state) return false;
  p_table_state->Reset();
  if (p_tablemap) p_tablemap->set_nchairs((int)JNum(body, "nchairs", 6));  // many engines loop to nchairs()

  int nchairs   = (int)JNum(body, "nchairs", 6);
  int userchair = (int)JNum(body, "userchair", 0);
  int dealer    = (int)JNum(body, "dealer", -1);
  double stack  = JNum(body, "stack", 100.0);
  double bet    = JNum(body, "bet", 0.0);
  std::string hole = JStr(body, "hole"), board = JStr(body, "board");
  std::string occ = JStr(body, "occupied"), act = JStr(body, "active");

  double bb = JNum(body, "bblind", 1.0);
  double sb = JNum(body, "sblind", bb / 2.0);
  int sbchair = (int)JNum(body, "sbchair", dealer >= 0 ? (dealer + 1) % nchairs : -1);
  int bbchair = (int)JNum(body, "bbchair", dealer >= 0 ? (dealer + 2) % nchairs : -1);
  for (int i = 0; i < nchairs; ++i) {
    bool seated = occ.empty() ? true : (i < (int)occ.size() && occ[i] == '1');
    bool active = act.empty() ? seated : (i < (int)act.size() && act[i] == '1');
    CPlayer* pl = p_table_state->Player(i);
    pl->set_seated(seated);
    pl->set_active(active);
    pl->set_dealer(i == dealer);
    pl->_balance.SetValue(stack);
    double pbet = 0.0;
    if (i == sbchair) pbet = sb; else if (i == bbchair) pbet = bb;
    if (i == userchair && bet > 0) pbet = bet;
    pl->_bet.SetValue(pbet);
    // All seated players are "in the hand": hero gets known cards (below);
    // everyone else gets card backs so HasAnyCards()/dealt logic includes them.
    if (seated && i != userchair) {
      pl->hole_cards(0)->SetValue(CARD_BACK);
      pl->hole_cards(1)->SetValue(CARD_BACK);
    }
  }
  // Hero hole cards -> the engine reads userchair off whoever holds KNOWN cards.
  if (hole.size() >= 4 && userchair >= 0 && userchair < nchairs) {
    p_table_state->Player(userchair)->hole_cards(0)->SetValue(CardStringToCardNumber((char*)hole.substr(0,2).c_str()));
    p_table_state->Player(userchair)->hole_cards(1)->SetValue(CardStringToCardNumber((char*)hole.substr(2,2).c_str()));
  }
  // Board / community cards.
  for (size_t j = 0; j*2 + 1 < board.size() && j < 5; ++j)
    SetCard(p_table_state->CommonCards((int)j), board.substr(j*2, 2));
  return true;
}

bool g_table_state_ready = false;
bool g_hiss_force_my_turn = false;  // set true once table state is populated for a decision

extern "C" const char* hiss_decide(const char* body_c) {
  static char buf[640];
  std::string body = body_c ? body_c : "";
  if (!body.empty() && body.find('{') != std::string::npos) {
    g_table_state_ready = PopulateTableState(body);
    g_hiss_force_my_turn = g_table_state_ready && getenv("HISS_MYTURN");  // heavy prwin/handrank path; opt-in
  }
  // EvaluateAll() over EMPTY table state dereferences scraper-populated data
  // (e.g. nopponents-1 indexing) — so only run it once the bridge has filled
  // CTableState. Until then report the engine's default decision surface.
  if (g_table_state_ready && p_engine_container) {
    std::fprintf(stderr, "[dbg] EvaluateAll start\n"); std::fflush(stderr);
    p_engine_container->EvaluateAll();
    std::fprintf(stderr, "[dbg] EvaluateAll done\n"); std::fflush(stderr);
    if (p_autoplayer_functions) {
      std::fprintf(stderr, "[dbg] CalcPrimary start\n"); std::fflush(stderr);
      p_autoplayer_functions->CalcPrimaryFormulas();
      std::fprintf(stderr, "[dbg] CalcPrimary done\n"); std::fflush(stderr);
      p_autoplayer_functions->CalcSecondaryFormulas();
      std::fprintf(stderr, "[dbg] CalcSecondary done\n"); std::fflush(stderr);
    }
  }
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
  auto sym = [](const char* n) -> double {
    double r = 0; if (p_engine_container) p_engine_container->EvaluateSymbol(n, &r, false); return r; };
  double hr169 = sym("handrank169"), uchair = sym("userchair"),
         ndealt = sym("nplayersdealt"), bround = sym("betround"), nopp = sym("nopponents");
  int tm_nchairs = p_tablemap ? p_tablemap->nchairs() : -1;
  int is_parsing = (p_formula_parser && p_formula_parser->IsParsing()) ? 1 : 0;
  int rcoc = (p_engine_container) ? 1 : 0;
  int user_hasany = (p_table_state && p_table_state->Player((int)JNum(std::string("{}"),"x",0)) ) ? 0 : 0;
  int p2_hasany = (p_table_state && p_table_state->Player(2)->HasAnyCards()) ? 1 : 0;
  int p2_hasknown = (p_table_state && p_table_state->Player(2)->HasKnownCards()) ? 1 : 0;
  std::snprintf(buf, sizeof(buf),
    "{\"action\":\"%s\",\"table_state\":\"%s\","
    "\"f$fold\":%g,\"f$check\":%g,\"f$call\":%g,\"f$raise\":%g,\"f$betsize\":%g,\"f$allin\":%g,"
    "\"sym\":{\"handrank169\":%g,\"userchair\":%g,\"nplayersdealt\":%g,\"betround\":%g,\"nopponents\":%g,\"tm_nchairs\":%d,\"p2_hasanycards\":%d,\"p2_hasknown\":%d,\"is_parsing\":%d}}",
    action, g_table_state_ready ? "populated" : "empty",
    fold, check, call, raise, betsize, allin, hr169, uchair, ndealt, bround, nopp, tm_nchairs, p2_hasany, p2_hasknown, is_parsing);
  return buf;
}
