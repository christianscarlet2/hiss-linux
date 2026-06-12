// winhttp.h — WinHTTP API surface for the Linux port.
//
// Currently INERT stubs so CScarletBeast links. The next step is to back these
// with libcurl (a WinHTTP-on-curl translation layer) so the Scarlet Beast API
// client actually fetches table state — CScarletBeast.cpp then needs no changes.
#ifndef HISS_WINHTTP_H
#define HISS_WINHTTP_H
#include "mfc_compat.h"

typedef void* HINTERNET;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef unsigned short INTERNET_PORT;

#define INTERNET_DEFAULT_HTTPS_PORT 443
#define INTERNET_DEFAULT_HTTP_PORT 80
#define WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY 4
#define WINHTTP_ACCESS_TYPE_DEFAULT_PROXY 0
#define WINHTTP_NO_PROXY_NAME nullptr
#define WINHTTP_NO_PROXY_BYPASS nullptr
#define WINHTTP_NO_REFERER nullptr
#define WINHTTP_DEFAULT_ACCEPT_TYPES nullptr
#define WINHTTP_NO_REQUEST_DATA nullptr
#define WINHTTP_NO_ADDITIONAL_HEADERS nullptr
#define WINHTTP_FLAG_SECURE 0x00800000
#define WINHTTP_HEADER_NAME_BY_INDEX nullptr
#define WINHTTP_NO_HEADER_INDEX nullptr
#define WINHTTP_NO_OUTPUT_BUFFER nullptr
#define WINHTTP_QUERY_FLAG_NUMBER 0x20000000
#define WINHTTP_QUERY_STATUS_CODE 19
#define WINHTTP_ADDREQ_FLAG_ADD 0x20000000

inline HINTERNET WinHttpOpen(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD) { return nullptr; }
inline HINTERNET WinHttpConnect(HINTERNET, LPCWSTR, INTERNET_PORT, DWORD) { return nullptr; }
inline HINTERNET WinHttpOpenRequest(HINTERNET, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD) { return nullptr; }
inline BOOL WinHttpSendRequest(HINTERNET, LPCWSTR, DWORD, void*, DWORD, DWORD, uintptr_t) { return 0; }
inline BOOL WinHttpReceiveResponse(HINTERNET, void*) { return 0; }
inline BOOL WinHttpQueryHeaders(HINTERNET, DWORD, LPCWSTR, void*, DWORD*, DWORD*) { return 0; }
inline BOOL WinHttpQueryDataAvailable(HINTERNET, DWORD*) { return 0; }
inline BOOL WinHttpReadData(HINTERNET, void*, DWORD, DWORD*) { return 0; }
inline BOOL WinHttpAddRequestHeaders(HINTERNET, LPCWSTR, DWORD, DWORD) { return 1; }
inline BOOL WinHttpCloseHandle(HINTERNET) { return 1; }

// wide<->narrow + wide CRT helpers the client uses
inline int MultiByteToWideChar(UINT, DWORD, const char* s, int, wchar_t* out, int n) {
  int i = 0; if (s) { for (; s[i] && (n == 0 || i < n - 1); i++) if (out) out[i] = (wchar_t)(unsigned char)s[i]; }
  if (out && n) out[i] = 0; return i + 1;
}
inline int WideCharToMultiByte(UINT, DWORD, const wchar_t* w, int, char* out, int n, const char*, BOOL*) {
  int i = 0; if (w) { for (; w[i] && (n == 0 || i < n - 1); i++) if (out) out[i] = (char)w[i]; }
  if (out && n) out[i] = 0; return i + 1;
}
inline int _wtoi(const wchar_t* w) { int v = 0, s = 1; if (!w) return 0; if (*w == '-') { s = -1; w++; } for (; *w >= '0' && *w <= '9'; w++) v = v * 10 + (*w - '0'); return v * s; }
#ifndef CP_UTF8
#define CP_UTF8 65001
#define CP_ACP 0
#endif

#endif  // HISS_WINHTTP_H
