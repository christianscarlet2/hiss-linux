#ifndef HISS_POKERTRACKER_QUERY_DEFINITIONS_H
#define HISS_POKERTRACKER_QUERY_DEFINITIONS_H
// PokerTracker DLL interface — inert stubs for the Linux port (no PT DB).
// The PT symbol engine compiles against these; all stats read as undefined.
#include "mfc_compat.h"

inline int     PT_DLL_GetNumberOfStats()                              { return 0; }
inline CString PT_DLL_GetBasicSymbolNameWithoutPTPrefix(int)          { return CString(); }
inline CString PT_DLL_GetDescription(int)                             { return CString(); }
inline CString PT_DLL_GetQuery(int, ...)                              { return CString(); }
inline double  PT_DLL_GetStat(const char*, int)                      { return 0.0; }
inline double  PT_DLL_GetStat(CString, int)                          { return 0.0; }
inline void    PT_DLL_SetStat(int, int, double)                      {}
inline void    PT_DLL_ClearPlayerStats(int)                          {}
inline bool    PT_DLL_IsValidSymbol(CString)                         { return false; }
inline bool    PT_DLL_IsAdvancedStat(int)                            { return false; }
inline bool    PT_DLL_IsPositionalPreflopStat(int)                   { return false; }

// callback registration the PT thread performs against the (absent) PT DLL
inline void    pUpdateStat(double (*)(int, int))                      {}
#endif
