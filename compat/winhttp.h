// winhttp.h — WinHTTP API surface for the Linux port, backed by libcurl.
//
// CScarletBeast.cpp drives the classic WinHTTP sequence (Open -> Connect ->
// OpenRequest -> SendRequest -> ReceiveResponse -> QueryHeaders(status) ->
// QueryDataAvailable/ReadData). These declarations are implemented in
// compat/winhttp_curl.cpp on top of libcurl, so the client fetches for real
// without any change to CScarletBeast.cpp. Link with -lcurl.
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

// Implemented over libcurl in compat/winhttp_curl.cpp.
HINTERNET WinHttpOpen(LPCWSTR user_agent, DWORD, LPCWSTR, LPCWSTR, DWORD);
HINTERNET WinHttpConnect(HINTERNET session, LPCWSTR host, INTERNET_PORT port, DWORD);
HINTERNET WinHttpOpenRequest(HINTERNET connect, LPCWSTR method, LPCWSTR path, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD flags);
BOOL WinHttpSendRequest(HINTERNET request, LPCWSTR headers, DWORD headers_len, void* body, DWORD body_len, DWORD total_len, uintptr_t);
BOOL WinHttpReceiveResponse(HINTERNET request, void*);
BOOL WinHttpQueryHeaders(HINTERNET request, DWORD flags, LPCWSTR, void* out, DWORD* out_len, DWORD*);
BOOL WinHttpQueryDataAvailable(HINTERNET request, DWORD* available);
BOOL WinHttpReadData(HINTERNET request, void* buf, DWORD buf_len, DWORD* read);
BOOL WinHttpAddRequestHeaders(HINTERNET request, LPCWSTR, DWORD, DWORD);
BOOL WinHttpCloseHandle(HINTERNET);

// wide<->narrow + wide CRT helpers the client uses (header-inline; trivial)
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
