// prw1326.h — the OpenHoldem prwin "1326" manual hand-weighting structure.
// The original definition lived in a prwin header not carried over in this port;
// reconstructed faithfully from engine usage (CIteratorThread, CSymbolEnginePrwin,
// CSymbolEngineRange). 1326 = C(52,2), the number of distinct starting hands.
#ifndef HISS_PRW1326_H
#define HISS_PRW1326_H

#define k_1326_total_combinations 1326

// Per-chair (and the "vanilla" template chair) hand-range weighting table.
struct sprw1326_chair {
  int ignore;                              // skip this chair entirely
  int level;                               // default weight level for the chair
  int limit;                               // number of populated combos
  int rankhi[k_1326_total_combinations];   // high card of each combo
  int ranklo[k_1326_total_combinations];   // low  card of each combo
  int weight[k_1326_total_combinations];   // per-combo weight
};

// One table per seat plus a vanilla template, with global enable flags.
struct sprw1326 {
  sprw1326_chair chair[13];                // indexed by chair/seat number
  sprw1326_chair vanilla_chair;            // template used to (re)seed chairs
  int preflop;                             // ==1326 → apply ranges preflop
  int useme;                               // ==1326 → manual weights are active
  int usecallback;                         // ==1326 → invoke prw_callback()
  void (*prw_callback)();                  // optional user callback
};

#endif  // HISS_PRW1326_H
