#ifndef HISS_OH_MESSAGEBOX_H
#define HISS_OH_MESSAGEBOX_H
#include "mfc_compat.h"
inline void OH_MessageBox_Error_Warning(const char* text, const char* caption = "Error") {
  std::fprintf(stderr, "[%s] %s\n", caption ? caption : "Error", text ? text : "");
}
inline void OH_MessageBox_Interactive(const char* text, const char* caption = "", unsigned = 0) {
  std::fprintf(stderr, "[%s] %s\n", caption ? caption : "", text ? text : "");
}
inline int OH_MessageBox(const char* text, const char* = "", unsigned = 0) {
  std::fprintf(stderr, "%s\n", text ? text : ""); return 1;
}
#endif
