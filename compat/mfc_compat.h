// Umbrella MFC/Win32 compatibility for the Linux OpenHoldem port.
// Included by the replacement stdafx.h. Grows as the compiler demands.
#ifndef HISS_COMPAT_MFC_COMPAT_H
#define HISS_COMPAT_MFC_COMPAT_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <iostream>
#include <cstdint>

// ---- poker-eval integer typedefs ----
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t   sint8;
typedef int16_t  sint16;
typedef int32_t  sint32;
typedef int64_t  sint64;

// ---- MSVC keywords + OpenHoldem DLL export macros → no-ops on Linux ----
#define __declspec(x)
#define __forceinline inline
#define WINAPI
#define CALLBACK
#define GLOBALS_DLL_API
#define PREFERENCES_DLL_API
#define DEBUG_DLL_API
#define FILES_DLL_API
#define VALIDATOR_DLL_API
#define WINDOWFUNCTIONS_DLL_API
#define STRINGFUNCTIONS_DLL_API

#include "mfc_string.h"
#include "mfc_collections.h"

// ---- common Win32 typedefs/macros the engine sprinkles around ----
typedef int             INT;
typedef long            LONG;
typedef unsigned short  WORD;
typedef void*           HWND;
typedef void*           HANDLE;
typedef void*           HINSTANCE;
typedef void*           HBITMAP;
typedef void*           HDC;
typedef int64_t         __int64;
typedef uint64_t        ULONGLONG;
typedef intptr_t        INT_PTR;
typedef uintptr_t       UINT_PTR;
#ifndef NULL
#define NULL 0
#endif
#ifndef MAX_PATH
#define MAX_PATH 4096
#endif
#ifndef __cdecl
#define __cdecl
#endif

struct POINT { long x, y; };
struct RECT  { long left, top, right, bottom; };

// ---- message boxes / debug → stderr (headless) ----
inline int AfxMessageBox(const char* msg, unsigned = 0, unsigned = 0) {
  std::fprintf(stderr, "[MSG] %s\n", msg ? msg : "");
  return 1;
}
inline int AfxMessageBox(const CString& msg, unsigned = 0, unsigned = 0) {
  return AfxMessageBox(msg.GetString());
}
#ifndef TRACE
#define TRACE(...) ((void)0)
#endif
#ifndef ASSERT
#define ASSERT(x) assert(x)
#endif
#ifndef VERIFY
#define VERIFY(x) ((void)(x))
#endif
#ifndef AfxGetApp
#define AfxGetApp() (nullptr)
#endif

// ---- threading: MFC sync primitives → std ----
class CCriticalSection {
 public:
  void Lock() { m_.lock(); }
  void Unlock() { m_.unlock(); }
 private:
  std::recursive_mutex m_;
};
class CSingleLock {
 public:
  CSingleLock(CCriticalSection* cs, BOOL lock = 0) : cs_(cs), locked_(false) { if (lock) Lock(); }
  ~CSingleLock() { if (locked_) Unlock(); }
  void Lock() { if (!locked_ && cs_) { cs_->Lock(); locked_ = true; } }
  void Unlock() { if (locked_ && cs_) { cs_->Unlock(); locked_ = false; } }
 private:
  CCriticalSection* cs_;
  bool locked_;
};

// ---- minimal CFile (read text formula files; the engine also writes logs) ----
class CFile {
 public:
  enum { modeRead = 1, modeWrite = 2, modeCreate = 4, shareDenyNone = 0, typeText = 0, typeBinary = 0 };
  CFile() : fp_(nullptr) {}
  ~CFile() { Close(); }
  BOOL Open(const char* path, UINT mode, void* = nullptr) {
    const char* m = (mode & modeWrite) ? "w" : "r";
    fp_ = std::fopen(path, m);
    return fp_ != nullptr;
  }
  UINT Read(void* buf, UINT n) { return fp_ ? (UINT)std::fread(buf, 1, n, fp_) : 0; }
  void Write(const void* buf, UINT n) { if (fp_) std::fwrite(buf, 1, n, fp_); }
  void Close() { if (fp_) { std::fclose(fp_); fp_ = nullptr; } }
  ULONGLONG GetLength() {
    if (!fp_) return 0;
    long cur = std::ftell(fp_); std::fseek(fp_, 0, SEEK_END);
    long len = std::ftell(fp_); std::fseek(fp_, cur, SEEK_SET);
    return (ULONGLONG)len;
  }
 private:
  FILE* fp_;
};

// ---- MFC object/serialization/runtime machinery (mostly inert on Linux) ----

class CWinThread { public: virtual ~CWinThread() {} virtual BOOL InitInstance() { return 1; } };
inline CWinThread* AfxBeginThread(void*, void*) { return nullptr; }

class CObject {
 public:
  virtual ~CObject() {}
};
class CArchive {
 public:
  enum Mode { load, store };
  bool IsStoring() const { return false; }
  bool IsLoading() const { return true; }
  CArchive& operator<<(int) { return *this; }
  CArchive& operator<<(const CString&) { return *this; }
  CArchive& operator>>(int&) { return *this; }
  CArchive& operator>>(CString&) { return *this; }
  BOOL ReadString(CString&) { return FALSE; }
  void WriteString(const char*) {}
};
struct CRuntimeClass { const char* m_lpszClassName; };
class CException : public CObject {
 public:
  virtual BOOL GetErrorMessage(char* buf, UINT n, UINT* = nullptr) { if (buf && n) buf[0] = 0; return FALSE; }
  void Delete() {}
};
class CFileException : public CException { public: int m_cause = 0; };
class CMemoryException : public CException {};
class CFileFind {
 public:
  BOOL FindFile(const char* = nullptr) { return FALSE; }
  BOOL FindNextFile() { return FALSE; }
  CString GetFileName() { return CString(); }
  CString GetFilePath() { return CString(); }
};

#ifndef DECLARE_DYNAMIC
#define DECLARE_DYNAMIC(x)
#define IMPLEMENT_DYNAMIC(x, y)
#define DECLARE_DYNCREATE(x)
#define IMPLEMENT_DYNCREATE(x, y)
#define DECLARE_SERIAL(x)
#define IMPLEMENT_SERIAL(x, y, z)
#define DECLARE_MESSAGE_MAP()
#define RUNTIME_CLASS(x) (nullptr)
#define afx_msg
#endif


// ---- Win32 critical section (no-op; threading belongs to stripped subsystems) ----
typedef struct _CRITICAL_SECTION { int dummy; } CRITICAL_SECTION;
inline void InitializeCriticalSection(CRITICAL_SECTION*) {}
inline BOOL InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION*, DWORD) { return 1; }
inline void EnterCriticalSection(CRITICAL_SECTION*) {}
inline void LeaveCriticalSection(CRITICAL_SECTION*) {}
inline void DeleteCriticalSection(CRITICAL_SECTION*) {}
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef DWORD* LPDWORD;
typedef BOOL* LPBOOL;

// ---- satisfy OpenHoldem's PCH guard (#ifndef __AFXWIN_H__) ----
#ifndef __AFXWIN_H__
#define __AFXWIN_H__
#endif

// ---- more sync + registry stubs (CCritSec is defined by the engine itself) ----
typedef void* HKEY;
typedef long LSTATUS;
#define HKEY_CURRENT_USER ((HKEY)0x80000001)
#define HKEY_LOCAL_MACHINE ((HKEY)0x80000002)
#define ERROR_SUCCESS 0
#define KEY_READ 0
#define KEY_WRITE 0
#define REG_SZ 1
#define REG_DWORD 4
inline long RegOpenKeyExA(HKEY, const char*, DWORD, DWORD, HKEY*) { return 1; }
inline long RegQueryValueExA(HKEY, const char*, DWORD*, DWORD*, BYTE*, DWORD*) { return 1; }
inline long RegSetValueExA(HKEY, const char*, DWORD, DWORD, const BYTE*, DWORD) { return 1; }
inline long RegCloseKey(HKEY) { return 0; }
inline long RegCreateKeyExA(HKEY, const char*, DWORD, char*, DWORD, DWORD, void*, HKEY*, DWORD*) { return 1; }

// ---- GDI colour + misc Win32 types/macros/functions ----
typedef DWORD COLORREF;
#define RGB(r, g, b) ((COLORREF)(((BYTE)(r)) | (((BYTE)(g)) << 8) | (((BYTE)(b)) << 16)))
#define GetRValue(c) ((BYTE)(c))
#define GetGValue(c) ((BYTE)(((c) >> 8) & 0xff))
#define GetBValue(c) ((BYTE)(((c) >> 16) & 0xff))
#ifndef WM_USER
#define WM_USER 0x0400
#define WM_APP 0x8000
#endif
typedef uintptr_t SOCKET;
typedef long LRESULT;
typedef UINT_PTR WPARAM;
typedef intptr_t LPARAM;
typedef WORD ATOM;
inline long InterlockedExchange(long volatile* t, long v) { long o = *t; *t = v; return o; }
inline long InterlockedIncrement(long volatile* t) { return ++(*t); }
inline long InterlockedDecrement(long volatile* t) { return --(*t); }
inline long InterlockedCompareExchange(long volatile* t, long e, long c) { long o = *t; if (o == c) *t = e; return o; }

// ---- Win32 helpers occasionally called by engine utility code ----
inline DWORD GetTickCount() { return 0; }
inline void  Sleep(DWORD) {}
inline DWORD GetCurrentThreadId() { return 0; }
inline DWORD GetLastError() { return 0; }


// ---- misc small bits ----
#ifndef _TRUNCATE
#define _TRUNCATE ((size_t)-1)
#endif
typedef CArray<BYTE, BYTE> CByteArray_base;
class CByteArray : public CArray<BYTE, BYTE> {};
typedef CArray<DWORD, DWORD> CDWordArray;
inline int strcpy_s(char* d, size_t n, const char* s) { strncpy(d, s, n); d[n?n-1:0]=0; return 0; }
inline int strncpy_s(char* d, size_t n, const char* s, size_t) { strncpy(d, s, n); d[n?n-1:0]=0; return 0; }
inline int sprintf_s(char* d, size_t n, const char* fmt, ...) { va_list a; va_start(a,fmt); int r=vsnprintf(d,n,fmt,a); va_end(a); return r; }
inline int vsprintf_s(char* d, size_t n, const char* fmt, va_list a) { return vsnprintf(d,n,fmt,a); }

#endif  // HISS_COMPAT_MFC_COMPAT_H
