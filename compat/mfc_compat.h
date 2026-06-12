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

// ---- Win32 helpers occasionally called by engine utility code ----
inline DWORD GetTickCount() { return 0; }
inline void  Sleep(DWORD) {}
inline DWORD GetCurrentThreadId() { return 0; }
inline DWORD GetLastError() { return 0; }

#endif  // HISS_COMPAT_MFC_COMPAT_H
