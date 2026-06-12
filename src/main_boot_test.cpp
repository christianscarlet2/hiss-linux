#include "stdafx.h"
#include "Singletons.h"
#include "CFormulaParser.h"
#include "globals.h"
#include "CSessionCounter.h"
#include <cstdio>

int main() {
  std::printf("[hiss] booting engine...\n");
  Preferences()->LoadPreferences();
  std::printf("[hiss] preferences loaded\n");
  if (!p_sessioncounter) p_sessioncounter = new CSessionCounter();
  InstantiateAllSingletons();
  std::printf("[hiss] singletons instantiated (pools, table state, engine container, formula parser)\n");
  if (p_formula_parser) {
    p_formula_parser->ParseDefaultLibraries();
    std::printf("[hiss] default OpenPPL libraries parsed\n");
  }
  std::printf("[hiss] engine booted OK\n");
  return 0;
}
