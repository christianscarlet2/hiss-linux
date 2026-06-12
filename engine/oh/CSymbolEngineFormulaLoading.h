#ifndef HISS_CSYMBOLENGINEFORMULALOADING_H
#define HISS_CSYMBOLENGINEFORMULALOADING_H
// Reconstructed for the Linux port from CSymbolEngineFormulaLoading.cpp
// (the original header was not carried over). Loads/switches OpenPPL formula
// profiles in response to the load$XYZ command.
#include "CVirtualSymbolEngine.h"

class CSymbolEngineFormulaLoading : public CVirtualSymbolEngine {
 public:
  CSymbolEngineFormulaLoading();
  ~CSymbolEngineFormulaLoading();
 public:
  // Mandatory reset/update functions
  void InitOnStartup();
  void ResetOnConnection();
  void ResetOnHandreset();
  void ResetOnNewRound();
  void ResetOnMyTurn();
  void ResetOnHeartbeat();
 public:
  void ChangeProfileOnLoadCommand();
  void RememberProfileForLoading(const char* symbol_name);
  bool EvaluateSymbol(const char* name, double* result, bool log = false);
  CString SymbolsProvided();
 private:
  CString _profile_to_be_loaded;
};
#endif
