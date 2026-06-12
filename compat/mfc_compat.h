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
#include <cfloat>
#include <ctime>
#include <algorithm>
// Win32 provides global min/max; the engine calls them unqualified.
using std::min;
using std::max;
using std::to_string;

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
#define __stdcall
#define __thiscall
#define __fastcall
#define DLL_IMPLEMENTS extern "C"
#define EXE_IMPLEMENTS extern "C"

// Minimal OpenCV / Tesseract namespace stubs — the OCR scraper (CAutoOcr) is
// replaced by the API feed; these let its header parse so its includers compile.
namespace cv { class Mat { public: Mat() {} }; class Rect { public: Rect() {} }; class Scalar { public: Scalar() {} }; class Size { public: Size() {} }; }
namespace tesseract { class TessBaseAPI { public: TessBaseAPI() {} }; }
#define GLOBALS_DLL_API
#define PREFERENCES_DLL_API
#define DEBUG_DLL_API
#define FILES_DLL_API
#define VALIDATOR_DLL_API
#define WINDOWFUNCTIONS_DLL_API
#define STRINGFUNCTIONS_DLL_API

#ifndef BETPOT_DEFAULT
#define BETPOT_DEFAULT 0
#define BETPOT_RAISE 1
#define BETPOT_CALL 2
#define BETPOT_POT 3
#endif

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
class CWinApp;
typedef void*           HCURSOR;
typedef void*           HICON;
typedef void*           HMENU_EARLY;
typedef intptr_t        INT_PTR;
typedef uintptr_t       UINT_PTR;
typedef uintptr_t       DWORD_PTR;
typedef intptr_t        LONG_PTR;
typedef long            LRESULT;   // (also typedef'd later; identical)
typedef UINT_PTR        WPARAM;
typedef intptr_t        LPARAM;
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
struct SIZE  { long cx, cy; };
typedef SIZE* LPSIZE;
struct LOGFONT;  // fully defined later; CFont::CreateFontIndirect needs only the pointer
// MFC geometry helpers (defined early so GUI classes can return them by value)
struct CPoint : public POINT { CPoint(long X=0,long Y=0){x=X;y=Y;} };
struct CRect : public RECT { CRect(long l=0,long t=0,long r=0,long b=0){left=l;top=t;right=r;bottom=b;} CRect(const RECT& r){*(RECT*)this=r;} int Width()const{return right-left;} int Height()const{return bottom-top;} operator RECT*(){return this;} operator const RECT*()const{return this;} void DeflateRect(int x,int y){left+=x;top+=y;right-=x;bottom-=y;} void InflateRect(int x,int y){left-=x;top-=y;right+=x;bottom+=y;} void OffsetRect(int x,int y){left+=x;right+=x;top+=y;bottom+=y;} void SetRectEmpty(){left=top=right=bottom=0;} POINT TopLeft()const{return POINT{left,top};} POINT BottomRight()const{return POINT{right,bottom};} POINT CenterPoint()const{return POINT{(left+right)/2,(top+bottom)/2};} BOOL PtInRect(POINT p)const{return p.x>=left&&p.x<right&&p.y>=top&&p.y<bottom;} CRect operator+(POINT p)const{return CRect(left+p.x,top+p.y,right+p.x,bottom+p.y);} CRect operator-(POINT p)const{return CRect(left-p.x,top-p.y,right-p.x,bottom-p.y);} };
struct CSize { long cx=0, cy=0; CSize(long X=0,long Y=0):cx(X),cy(Y){} };

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
extern CWinApp* AfxGetAppPtr();
#ifndef AfxGetApp
#define AfxGetApp() (AfxGetAppPtr())
#endif

// ---- threading: MFC sync primitives → std ----
class CCriticalSection {
 public:
  void Lock() { m_.lock(); }
  void Unlock() { m_.unlock(); }
 private:
  std::recursive_mutex m_;
};
class CMutex {
 public:
  HANDLE m_hObject = (HANDLE)1;
  CMutex(BOOL = 0, const char* = nullptr, void* = nullptr) {}
  BOOL Lock(DWORD = 0) { m_.lock(); return 1; }
  BOOL Unlock() { m_.unlock(); return 1; }
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
  enum { modeRead = 1, modeWrite = 2, modeCreate = 4, modeReadWrite = 3, modeNoTruncate = 8,
         shareDenyNone = 0, shareDenyRead = 0x10, shareDenyWrite = 0x20, shareExclusive = 0x30,
         shareCompat = 0, typeText = 0x4000, typeBinary = 0x8000, modeNoInherit = 0x80 };
  CFile() : fp_(nullptr) {}
  CFile(const char* path, UINT mode) : fp_(nullptr) { Open(path, mode); }
  CFile(const CString& path, UINT mode) : fp_(nullptr) { Open(path.GetString(), mode); }
  ~CFile() { Close(); }
  BOOL Open(const char* path, UINT mode, void* = nullptr) {
    const char* m = (mode & modeWrite) ? "w" : "r";
    fp_ = std::fopen(path, m);
    path_ = path ? path : "";
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
  CString GetFilePath() const { return path_; }
  CString GetFileName() const {
    size_t p = path_.find_last_of("/\\");
    return CString(p == std::string::npos ? path_.c_str() : path_.c_str() + p + 1);
  }
 protected:
  FILE* fp_ = nullptr;
  std::string path_;
};

// ---- MFC object/serialization/runtime machinery (mostly inert on Linux) ----

class CWinThread { public: virtual ~CWinThread() {} virtual BOOL InitInstance() { return 1; } virtual int Run() { return 0; } HANDLE m_hThread = nullptr; DWORD m_nThreadID = 0; BOOL m_bAutoDelete = 1; BOOL SetThreadPriority(int) { return 1; } BOOL ResumeThread() { return 1; } BOOL SuspendThread() { return 1; } };
inline CWinThread* AfxBeginThread(void*, void*, int = 0, UINT = 0, DWORD = 0, void* = nullptr) { return nullptr; }

// ---- MFC GUI base classes → inert stubs (GUI is stripped on the server) ----
// forward decls so CWnd's accessors can name these (full defs follow below)
class CDataExchange; class CDC; class CFont; class CPen; class CBrush; class CBitmap; class CGdiObject;
class CFrameWnd;
struct MSG; struct CREATESTRUCT; struct CRuntimeClass;
class CCmdTarget { public: virtual ~CCmdTarget() {} };
class CWinApp : public CWinThread {
 public:
  virtual ~CWinApp() {}
  virtual BOOL InitInstance() { return 1; }
  virtual int  ExitInstance() { return 0; }
  const char* m_pszAppName = "hiss";
  const char* m_pszExeName = "hiss";
  class CWnd* m_pMainWnd = nullptr;
  HINSTANCE m_hInstance = nullptr;
  const char* m_lpCmdLine = "";
  HCURSOR LoadStandardCursor(const char*) { return nullptr; }
  HICON LoadStandardIcon(const char*) { return nullptr; }
  HCURSOR LoadCursor(const char*) { return nullptr; }
  HICON LoadIcon(const char*) { return nullptr; }
  int DoMessageBox(const char*, UINT, UINT) { return 0; }
};
class CWnd : public CCmdTarget {
 public:
  virtual ~CWnd() {}
  HWND m_hWnd = nullptr;
  BOOL ShowWindow(int = 0) { return 1; }
  BOOL UpdateWindow() { return 1; }
  BOOL DestroyWindow() { return 1; }
  void SetWindowText(const char*) {}
  HWND GetSafeHwnd() const { return m_hWnd; }
  void GetWindowRect(RECT* r) const { if (r) { r->left = r->top = r->right = r->bottom = 0; } }
  void GetClientRect(RECT* r) const { if (r) { r->left = r->top = r->right = r->bottom = 0; } }
  BOOL IsWindowVisible() const { return 0; }
  CWnd* GetParent() const { return nullptr; }
  CWnd* GetTopLevelParent() const { return nullptr; }
  BOOL  IsChild(const CWnd*) const { return 0; }
  BOOL  IsChild(HWND) const { return 0; }
  void  ClientToScreen(POINT*) const {}
  void  ClientToScreen(RECT*) const {}
  void  ScreenToClient(POINT*) const {}
  void  ScreenToClient(RECT*) const {}
  CWnd* GetDlgItem(int) const { return nullptr; }
  void  Invalidate(BOOL = 1) {}
  void  InvalidateRect(const RECT*, BOOL = 1) {}
  virtual int  OnCreate(CREATESTRUCT*) { return 0; }
  virtual BOOL PreTranslateMessage(MSG*) { return 0; }
  virtual BOOL PreTranslateInput(MSG*) { return 0; }
  virtual BOOL PreCreateWindow(CREATESTRUCT&) { return 1; }
  UINT_PTR SetTimer(UINT_PTR id, UINT, void* = nullptr) { return id; }
  BOOL  KillTimer(UINT_PTR) { return 1; }
  LRESULT SendMessage(UINT, WPARAM = 0, LPARAM = 0) { return 0; }
  BOOL  PostMessage(UINT, WPARAM = 0, LPARAM = 0) { return 1; }
  int   GetWindowText(CString&) const { return 0; }
  int   GetWindowText(char* buf, int n) const { if (buf && n) buf[0] = 0; return 0; }
  void  SetWindowText(const CString&) {}
  CFrameWnd* GetTopLevelFrame() const { return nullptr; }
  CWnd*      GetParentFrame() const { return nullptr; }
  CFont* GetFont() const { return nullptr; }
  void   SetFont(CFont*, BOOL = 1) {}
  DWORD  GetStyle() const { return 0; }
  DWORD  GetExStyle() const { return 0; }
  CDC*   GetDC() { return nullptr; }
  CDC*   GetWindowDC() { return nullptr; }
  int    ReleaseDC(CDC*) { return 1; }
  LONG   GetWindowLong(int) const { return 0; }
  LONG   GetWindowLong(HWND, int) const { return 0; }
  LONG   SetWindowLong(int, LONG) { return 0; }
  LONG   SetWindowLong(HWND, int, LONG) { return 0; }
  virtual void OnMouseMove(UINT, POINT) {}
  virtual void OnLButtonDown(UINT, POINT) {}
  virtual void OnRButtonDown(UINT, POINT) {}
  virtual void OnTimer(UINT_PTR) {}
  virtual void OnPaint() {}
  virtual BOOL OnEraseBkgnd(CDC*) { return 1; }
  virtual void OnSize(UINT, int, int) {}
  LRESULT DefWindowProc(UINT, WPARAM, LPARAM) { return 0; }
  LRESULT Default() { return 0; }
  CWnd* SetFocus() { return nullptr; }
  static CWnd* GetFocus() { return nullptr; }
  static CWnd* GetCapture() { return nullptr; }
  CWnd* SetCapture() { return nullptr; }
  static void ReleaseCapture() {}
  BOOL  EnableWindow(BOOL = 1) { return 1; }
  void  CenterWindow(CWnd* = nullptr) {}
  BOOL  UpdateData(BOOL = 1) { return 1; }
  BOOL  CreateEx(DWORD, const char*, const char*, DWORD, int, int, int, int, HWND, void*, void* = nullptr) { return 1; }
  BOOL  CreateEx(DWORD, const char*, const char*, DWORD, const RECT&, CWnd*, UINT, void* = nullptr) { return 1; }
  BOOL  Create(const char*, const char*, DWORD, const RECT&, CWnd*, UINT, void* = nullptr) { return 1; }
  HWND   GetSafeHwnd_() const { return m_hWnd; }
  void   MoveWindow(const RECT*, BOOL = 1) {}
  void   MoveWindow(int, int, int, int, BOOL = 1) {}
};
class CDialog : public CWnd { public: CDialog(unsigned = 0, CWnd* = nullptr) {} CDialog(const char*, CWnd* = nullptr) {} virtual BOOL OnInitDialog() { return 1; } virtual void OnOK() {} virtual void OnCancel() {} virtual void DoDataExchange(CDataExchange*) {} int DoModal() { return 0; } BOOL Create(unsigned, CWnd* = nullptr) { return 1; } BOOL Create(const char*, CWnd* = nullptr) { return 1; } };
class CFrameWnd : public CWnd { public: BOOL m_bHelpMode = 0; CWnd* GetActiveView() const { return nullptr; } void EnableDocking(DWORD) {} void DockControlBar(CWnd*, UINT = 0, RECT* = nullptr) {} void FloatControlBar(CWnd*, POINT, DWORD = 0) {} void RecalcLayout(BOOL = 1) {} void ShowControlBar(CWnd*, BOOL, BOOL) {} };
class CDocument : public CCmdTarget {};
class CView : public CWnd { public: CDocument* m_pDocument = nullptr; CDocument* GetDocument() const { return m_pDocument; } virtual void OnInitialUpdate() {} virtual void OnDraw(CDC*) {} virtual void OnUpdate(CView*, intptr_t, void*) {} };
class CStatic : public CWnd { public: BOOL Create(const char*, DWORD, const RECT&, CWnd*, UINT = 0) { return 1; } void SetWindowText(const char*) {} };
class CToolBarCtrl { public: BOOL SetButtonInfo(int, void*) { return 1; } int GetButtonCount() { return 0; } BOOL CheckButton(int, BOOL = 1) { return 1; } BOOL EnableButton(int, BOOL = 1) { return 1; } BOOL IsButtonChecked(int) { return 0; } BOOL IsButtonEnabled(int) { return 1; } };
class CToolBar : public CWnd { public: BOOL Create(CWnd*, DWORD = 0, UINT = 0) { return 1; } BOOL CreateEx(CWnd*, DWORD = 0, DWORD = 0) { return 1; } CToolBarCtrl& GetToolBarCtrl() { static CToolBarCtrl c; return c; } operator void*() { return this; } BOOL LoadToolBar(UINT) { return 1; } BOOL LoadToolBar(const char*) { return 1; } void EnableDocking(DWORD) {} void SetBarStyle(DWORD) {} DWORD GetBarStyle() { return 0; } BOOL SetButtons(const UINT*, int) { return 1; } void SetSizes(SIZE, SIZE) {} };
class CButton : public CWnd {};
class CEdit : public CWnd {};
class CComboBox : public CWnd {};
class CListBox : public CWnd {};
class CStatusBar : public CWnd { public: BOOL Create(CWnd*, DWORD = 0, UINT = 0) { return 1; } BOOL SetIndicators(const UINT*, int) { return 1; } void SetPaneText(int, const char*, BOOL = 1) {} void SetPaneInfo(int, UINT, UINT, int) {} void GetStatusBarCtrl() {} int CommandToIndex(UINT) const { return 0; } void GetItemRect(int, RECT*) const {} };
class CGdiObject; class CPen; class CBrush; class CFont; class CBitmap;
typedef DWORD COLORREF;  // (also typedef'd in the GDI colour section; identical)
struct PAINTSTRUCT { HDC hdc; BOOL fErase; RECT rcPaint; BOOL fRestore, fIncUpdate; BYTE rgbReserved[32]; };
class CDC {
 public:
  HDC m_hDC = nullptr;
  CGdiObject* SelectObject(CGdiObject*) { return nullptr; }
  CPen* SelectObject(CPen*) { return nullptr; }
  CBrush* SelectObject(CBrush*) { return nullptr; }
  CFont* SelectObject(CFont*) { return nullptr; }
  CBitmap* SelectObject(CBitmap*) { return nullptr; }
  CPen* SelectObject(CPen&) { return nullptr; }
  CBrush* SelectObject(CBrush&) { return nullptr; }
  CFont* SelectObject(CFont&) { return nullptr; }
  CBitmap* SelectObject(CBitmap&) { return nullptr; }
  POINT MoveTo(int, int) { return POINT{0, 0}; }
  POINT MoveTo(POINT) { return POINT{0, 0}; }
  BOOL LineTo(int, int) { return 1; }
  BOOL LineTo(POINT) { return 1; }
  BOOL Rectangle(int, int, int, int) { return 1; }
  BOOL Rectangle(const RECT*) { return 1; }
  BOOL Ellipse(int, int, int, int) { return 1; }
  BOOL Ellipse(const RECT*) { return 1; }
  BOOL RoundRect(int, int, int, int, int, int) { return 1; }
  BOOL Polygon(const POINT*, int) { return 1; }
  BOOL Polyline(const POINT*, int) { return 1; }
  BOOL Arc(int, int, int, int, int, int, int, int) { return 1; }
  BOOL TextOutA(int, int, const char*, int) { return 1; }
  BOOL TextOut(int, int, const char*, int) { return 1; }
  int  DrawText(const char*, int, RECT*, UINT) { return 0; }
  int  DrawText(const CString&, RECT*, UINT) { return 0; }
  int  FillRect(const RECT*, CBrush*) { return 1; }
  void FillSolidRect(const RECT*, COLORREF) {}
  void FillSolidRect(int, int, int, int, COLORREF) {}
  void Draw3dRect(const RECT*, COLORREF, COLORREF) {}
  BOOL DrawEdge(RECT*, UINT, UINT) { return 1; }
  int  FrameRect(const RECT*, CBrush*) { return 1; }
  COLORREF SetTextColor(COLORREF) { return 0; }
  COLORREF SetBkColor(COLORREF) { return 0; }
  int  SetBkMode(int) { return 0; }
  COLORREF GetPixel(int, int) { return 0; }
  COLORREF SetPixel(int, int, COLORREF) { return 0; }
  BOOL BitBlt(int, int, int, int, CDC*, int, int, DWORD) { return 1; }
  BOOL PatBlt(int, int, int, int, DWORD) { return 1; }
  BOOL StretchBlt(int, int, int, int, CDC*, int, int, int, int, DWORD) { return 1; }
  BOOL CreateCompatibleDC(CDC*) { return 1; }
  BOOL CreateCompatibleBitmap(CDC*, int, int) { return 1; }
  BOOL DeleteDC() { return 1; }
  int  SaveDC() { return 1; }
  BOOL RestoreDC(int) { return 1; }
  HDC  GetSafeHdc() const { return m_hDC; }
  CPen* GetCurrentPen() const { return nullptr; }
  CBrush* GetCurrentBrush() const { return nullptr; }
  CFont* GetCurrentFont() const { return nullptr; }
  CBitmap* GetCurrentBitmap() const { return nullptr; }
  int  GetDeviceCaps(int) const { return 0; }
  COLORREF GetTextColor() const { return 0; }
  COLORREF GetBkColor() const { return 0; }
  CSize GetTextExtent(const char*, int) const { return CSize(); }
  CSize GetTextExtent(const CString&) const { return CSize(); }
  BOOL GetTextMetrics(void*) const { return 1; }
};
class CClientDC : public CDC { public: CClientDC(CWnd* = nullptr) {} };
class CWindowDC : public CDC { public: CWindowDC(CWnd* = nullptr) {} };
class CPaintDC  : public CDC { public: CPaintDC(CWnd* = nullptr) {} PAINTSTRUCT m_ps{}; };
class CDataExchange { public: CWnd* m_pDlgWnd = nullptr; };

class CObject {
 public:
  virtual ~CObject() {}
  virtual CRuntimeClass* GetRuntimeClass() const { return nullptr; }
  BOOL IsKindOf(const CRuntimeClass*) const { return 1; }
};
class CArchive {
 public:
  enum Mode { load, store };
  CArchive() {}
  CArchive(CFile*, UINT, int = 0, void* = nullptr) {}
  bool IsStoring() const { return false; }
  bool IsLoading() const { return true; }
  CArchive& operator<<(int) { return *this; }
  CArchive& operator<<(const CString&) { return *this; }
  CArchive& operator>>(int&) { return *this; }
  CArchive& operator>>(CString&) { return *this; }
  BOOL ReadString(CString&) { return FALSE; }
  void WriteString(const char*) {}
  CFile* GetFile() const { return nullptr; }
};
struct CRuntimeClass { const char* m_lpszClassName; };
class CException : public CObject {
 public:
  virtual BOOL GetErrorMessage(char* buf, UINT n, UINT* = nullptr) { if (buf && n) buf[0] = 0; return FALSE; }
  void Delete() {}
};
class CFileException : public CException { public: int m_cause = 0; long m_lOsError = 0; CString m_strFileName; static int OsErrorToException(long) { return 0; } };
class CMemoryException : public CException {};
class CFileFind {
 public:
  BOOL FindFile(const char* = nullptr) { return FALSE; }
  BOOL FindNextFile() { return FALSE; }
  CString GetFileName() { return CString(); }
  CString GetFilePath() { return CString(); }
  BOOL IsDots() const { return FALSE; }
  BOOL IsDirectory() const { return FALSE; }
  void Close() {}
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
inline BOOL TryEnterCriticalSection(CRITICAL_SECTION*) { return 1; }
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
// GDI object handles + no-op management (engine touches these in stripped GUI paths)
typedef void* HGDIOBJ;
typedef void* HPEN;
typedef void* HBRUSH;
typedef void* HFONT;
typedef void* HRGN;
typedef void* HPALETTE;
typedef void* HMENU;
typedef void* HICON;
typedef void* HCURSOR;
inline BOOL DeleteObject(HGDIOBJ) { return 1; }
inline HGDIOBJ SelectObject(HDC, HGDIOBJ) { return nullptr; }
inline HGDIOBJ GetStockObject(int) { return nullptr; }
inline BOOL DeleteDC(HDC) { return 1; }
inline HDC CreateCompatibleDC(HDC) { return nullptr; }
inline HBITMAP CreateCompatibleBitmap(HDC, int, int) { return nullptr; }
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

// ---- GDI wrapper classes (depend on CObject, defined above) ----
class CFont : public CObject { public: HFONT m_hObject = nullptr; BOOL CreateFont(...) { return 1; } BOOL CreateFontIndirect(const LOGFONT*) { return 1; } HFONT GetSafeHandle() const { return m_hObject; } operator HFONT() const { return m_hObject; } static CFont* FromHandle(HFONT) { return nullptr; } BOOL DeleteObject() { return 1; } };
class CBitmap : public CObject { public: HBITMAP m_hObject = nullptr; HBITMAP GetSafeHandle() const { return m_hObject; } operator HBITMAP() const { return m_hObject; } BOOL CreateCompatibleBitmap(CDC*, int, int) { return 1; } BOOL CreateBitmap(int, int, UINT, UINT, const void*) { return 1; } BOOL LoadBitmap(UINT) { return 1; } BOOL LoadBitmap(const char*) { return 1; } BOOL Attach(HBITMAP h) { m_hObject = h; return 1; } HBITMAP Detach() { HBITMAP h = m_hObject; m_hObject = nullptr; return h; } static CBitmap* FromHandle(HBITMAP) { return nullptr; } BOOL DeleteObject() { return 1; } };
class CPen : public CObject { public: HPEN m_hObject = nullptr; CPen() {} CPen(int, int, COLORREF) {} BOOL CreatePen(int, int, COLORREF) { return 1; } HPEN GetSafeHandle() const { return m_hObject; } operator HPEN() const { return m_hObject; } static CPen* FromHandle(HPEN) { return nullptr; } BOOL DeleteObject() { return 1; } };
class CBrush : public CObject { public: HBRUSH m_hObject = nullptr; CBrush() {} CBrush(COLORREF) {} BOOL CreateSolidBrush(COLORREF) { return 1; } BOOL CreateStockObject(int) { return 1; } operator HBRUSH() const { return m_hObject; } HBRUSH GetSafeHandle() const { return m_hObject; } static CBrush* FromHandle(HBRUSH) { return nullptr; } BOOL DeleteObject() { return 1; } };
// (CDC + paint variants are defined earlier alongside the GUI base classes)

// ---- Win32 message/window structs + process API stubs ----
// (engine touches these in stripped GUI/autoplayer paths; inert here)
struct CREATESTRUCT { void* lpCreateParams; HINSTANCE hInstance; HMENU hMenu; HWND hwndParent; int cy, cx, y, x; long style; const char* lpszName; const char* lpszClass; DWORD dwExStyle; };
typedef CREATESTRUCT* LPCREATESTRUCT;
struct STARTUPINFOA { DWORD cb; char* lpReserved; char* lpDesktop; char* lpTitle; DWORD dwX, dwY, dwXSize, dwYSize, dwXCountChars, dwYCountChars, dwFillAttribute, dwFlags; WORD wShowWindow, cbReserved2; BYTE* lpReserved2; HANDLE hStdInput, hStdOutput, hStdError; };
typedef STARTUPINFOA STARTUPINFO;
typedef STARTUPINFOA* LPSTARTUPINFOA;
struct PROCESS_INFORMATION { HANDLE hProcess, hThread; DWORD dwProcessId, dwThreadId; };
typedef PROCESS_INFORMATION* LPPROCESS_INFORMATION;
#define PROCESS_TERMINATE 0x0001
#define PROCESS_QUERY_INFORMATION 0x0400
inline void PostQuitMessage(int) {}
inline BOOL PostMessage(HWND, UINT, WPARAM, LPARAM) { return 1; }
inline LRESULT SendMessage(HWND, UINT, WPARAM, LPARAM) { return 0; }
inline HANDLE OpenProcess(DWORD, BOOL, DWORD) { return nullptr; }
inline BOOL TerminateProcess(HANDLE, UINT) { return 1; }
inline BOOL CloseHandle(HANDLE) { return 1; }

// mouse/keyboard DLL function-pointer typedefs (autoplayer DLLs — stripped)
typedef int (*mouse_process_message_t)(void*, void*, void*);
typedef int (*keyboard_process_message_t)(void*, void*, void*);
typedef int (*process_message_t)(void*, void*, void*);
typedef int (*mouse_click_t)(void*, RECT, int, int);
typedef int (*keyboard_click_t)(void*, int, int, int);

// ---- more Win32 odds & ends the engine references ----
typedef RECT* LPRECT;
typedef POINT* LPPOINT;
struct LOGFONT { long lfHeight, lfWidth, lfEscapement, lfOrientation, lfWeight; BYTE lfItalic, lfUnderline, lfStrikeOut, lfCharSet, lfOutPrecision, lfClipPrecision, lfQuality, lfPitchAndFamily; char lfFaceName[32]; };
typedef LOGFONT* LPLOGFONT;
struct MSG { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; DWORD time; POINT pt; };
typedef MSG* LPMSG;

// MFC command-UI updater (menu/toolbar enable state) — inert
class CCmdUI { public: void Enable(BOOL=1){} void SetCheck(int=1){} void SetText(const char*){} UINT m_nID=0; };

// message-box flags → 0 (AfxMessageBox ignores them)
#ifndef MB_OK
#define MB_OK 0
#define MB_OKCANCEL 1
#define MB_YESNO 4
#define MB_ICONERROR 0
#define MB_ICONWARNING 0
#define MB_ICONINFORMATION 0
#define MB_ICONQUESTION 0
#define MB_ICONSTOP 0
#define MB_SYSTEMMODAL 0
#define MB_TOPMOST 0x40000
#define MB_SETFOREGROUND 0x10000
#define MB_TASKMODAL 0x2000
#define MB_APPLMODAL 0
#define IDOK 1
#define IDCANCEL 2
#define IDYES 6
#define IDNO 7
#endif

// case-insensitive memcmp (MSVC _memicmp/memicmp)
inline int _memicmp(const void* a, const void* b, size_t n) {
  const unsigned char *p=(const unsigned char*)a, *q=(const unsigned char*)b;
  for (size_t i=0;i<n;i++){ int d=tolower(p[i])-tolower(q[i]); if(d) return d; } return 0;
}
inline int memicmp(const void* a, const void* b, size_t n) { return _memicmp(a,b,n); }

// MSVC _O_* file-mode aliases (literal values — avoid pulling <fcntl.h>, whose
// F_OK macro would collide with MagicNumbers.h's `const int F_OK`)
#ifndef _O_RDONLY
#define _O_RDONLY 0x0000
#define _O_WRONLY 0x0001
#define _O_RDWR   0x0002
#define _O_CREAT  0x0100
#define _O_BINARY 0x0000
#define _O_TEXT   0x0000
#endif

inline DWORD GetCurrentProcessId() { return 0; }
inline HANDLE GetCurrentProcess() { return nullptr; }
inline HINSTANCE GetModuleHandle(const char*) { return nullptr; }

#ifndef VERSION_TEXT
#define VERSION_TEXT "Hiss-Linux"
#endif

#ifndef AFXAPI
#define AFXAPI
#endif
// MFC's CMap hash helper — primary template so engine specializations parse
template<class ARG_KEY>
UINT AFXAPI HashKey(ARG_KEY key) { return (UINT)(((uintptr_t)key) >> 4); }
// MFC's CompareElements / CopyElements — primary templates for the same reason
template<class TYPE, class ARG_TYPE>
BOOL AFXAPI CompareElements(const TYPE* e1, const ARG_TYPE* e2) { return *e1 == *e2; }
template<class TYPE, class ARG_TYPE>
void AFXAPI CopyElements(TYPE* dst, const ARG_TYPE* src, int n) { for (int i = 0; i < n; i++) dst[i] = src[i]; }
#ifndef AFX_EXT_CLASS
#define AFX_EXT_CLASS
#endif

// detected-table descriptor (autoconnector window list; window detection is
// replaced by the API feed, so members are inert here)
struct STableList {
  HWND  hwnd = nullptr;
  CString title;
  int   x = 0, y = 0, width = 0, height = 0;
  int   tablemap_index = 0;
  CString name;
};

// ---- CRT functions MSVC spells differently ----
inline int fopen_s(FILE** fp, const char* path, const char* mode) { return (*fp = std::fopen(path, mode)) ? 0 : 1; }
inline struct tm* localtime_s(struct tm* out, const time_t* t) { localtime_r(t, out); return out; }
inline int gmtime_s(struct tm* out, const time_t* t) { gmtime_r(t, out); return 0; }
extern "C" int access(const char*, int);  // declared without <unistd.h> (its F_OK macro clashes with MagicNumbers)
inline int _access(const char* p, int m) { return access(p, m); }
#ifndef _SH_DENYWR
#define _SH_DENYNO 0x40
#define _SH_DENYRD 0x30
#define _SH_DENYWR 0x20
#define _SH_DENYRW 0x10
#endif

// ---- Win32 kernel/user API stubs (inert; subsystems are stripped) ----
inline HANDLE CreateSemaphore(void*, long, long, const char*) { return nullptr; }
inline BOOL ReleaseSemaphore(HANDLE, long, long*) { return 1; }
inline HANDLE CreateEvent(void*, BOOL, BOOL, const char*) { return nullptr; }
inline HANDLE CreateMutex(void*, BOOL, const char*) { return nullptr; }
inline HANDLE CreateThread(void*, size_t, void*, void*, DWORD, DWORD*) { return nullptr; }
// CreateProcess: inert variadic stubs (callers pass MSVC-lax args like `false` for NULL)
inline BOOL CreateProcessA(const char*, char*, ...) { return 0; }
inline BOOL CreateProcess(const char*, char*, ...) { return 0; }
inline BOOL   SetEvent(HANDLE) { return 1; }
inline BOOL   ResetEvent(HANDLE) { return 1; }
inline DWORD  WaitForSingleObject(HANDLE, DWORD) { return 0; }
inline BOOL   ReleaseMutex(HANDLE) { return 1; }
inline BOOL   IsWindow(HWND) { return 0; }
inline int    MessageBoxA(HWND, const char* t, const char* c, UINT) { return AfxMessageBox(t ? t : ""); }
inline int    MessageBoxW(HWND, const wchar_t*, const wchar_t*, UINT) { return 1; }
#define MessageBox MessageBoxA
inline const char* GetCommandLineA() { return ""; }
inline const char* GetCommandLine()  { return ""; }
inline DWORD  GetSysColor(int) { return 0; }
#define COLOR_SCROLLBAR 0
#define COLOR_BACKGROUND 1
#define COLOR_ACTIVECAPTION 2
#define COLOR_INACTIVECAPTION 3
#define COLOR_MENU 4
#define COLOR_WINDOW 5
#define COLOR_WINDOWFRAME 6
#define COLOR_MENUTEXT 7
#define COLOR_WINDOWTEXT 8
#define COLOR_CAPTIONTEXT 9
#define COLOR_ACTIVEBORDER 10
#define COLOR_INACTIVEBORDER 11
#define COLOR_APPWORKSPACE 12
#define COLOR_HIGHLIGHT 13
#define COLOR_HIGHLIGHTTEXT 14
#define COLOR_BTNFACE 15
#define COLOR_3DFACE 15
#define COLOR_BTNSHADOW 16
#define COLOR_3DSHADOW 16
#define COLOR_GRAYTEXT 17
#define COLOR_BTNTEXT 18
#define COLOR_INACTIVECAPTIONTEXT 19
#define COLOR_BTNHIGHLIGHT 20
#define COLOR_3DHILIGHT 20
#define COLOR_3DHIGHLIGHT 20
#define COLOR_3DDKSHADOW 21
#define COLOR_3DLIGHT 22
#define COLOR_INFOTEXT 23
#define COLOR_INFOBK 24
// extended window styles + clip flags
#define WS_EX_DLGMODALFRAME 0x1
#define WS_EX_WINDOWEDGE 0x100
#define WS_EX_APPWINDOW 0x40000
#define WS_OVERLAPPED 0
#define WS_OVERLAPPEDWINDOW 0xcf0000
#define WS_MINIMIZEBOX 0x20000
#define WS_MAXIMIZEBOX 0x10000
#define WS_THICKFRAME 0x40000
#define WS_DLGFRAME 0x400000
#define WS_VSCROLL 0x200000
#define WS_HSCROLL 0x100000
#define ES_AUTOVSCROLL 0x40
#define ES_LEFT 0
#define ES_NOHIDESEL 0x100
#define ES_WANTRETURN 0x1000
#define WS_CLIPCHILDREN 0x02000000
#define WS_CLIPSIBLINGS 0x04000000
#define WS_POPUP 0x80000000
#define WS_CAPTION 0x00C00000
#define WS_SYSMENU 0x00080000
#define WS_DISABLED 0x08000000
#define WS_EX_TOPMOST 0x00000008
#define WS_EX_TOOLWINDOW 0x00000080
#define WS_EX_STATICEDGE 0x00020000
inline BOOL SystemParametersInfoA(UINT, UINT, void*, UINT) { return 1; }
#define SystemParametersInfo SystemParametersInfoA
#define SPI_GETWORKAREA 0x0030

// crypto (MD5/license) → inert stubs
typedef uintptr_t HCRYPTPROV;
typedef uintptr_t HCRYPTHASH;
typedef uintptr_t HCRYPTKEY;
#define PROV_RSA_FULL 1
#define CRYPT_VERIFYCONTEXT 0xF0000000
#define CRYPT_NEWKEYSET 0x00000008
#define CALG_MD5 0x8003
#define HP_HASHVAL 0x0002
inline BOOL CryptAcquireContextA(HCRYPTPROV*, const char*, const char*, DWORD, DWORD) { return 0; }
#define CryptAcquireContext CryptAcquireContextA
inline BOOL CryptCreateHash(HCRYPTPROV, DWORD, HCRYPTKEY, DWORD, HCRYPTHASH*) { return 0; }
inline BOOL CryptHashData(HCRYPTHASH, const BYTE*, DWORD, DWORD) { return 0; }
inline BOOL CryptGetHashParam(HCRYPTHASH, DWORD, BYTE*, DWORD*, DWORD) { return 0; }
inline BOOL CryptDestroyHash(HCRYPTHASH) { return 1; }
inline BOOL CryptReleaseContext(HCRYPTPROV, DWORD) { return 1; }

// process memory info (logging) → inert
struct PROCESS_MEMORY_COUNTERS { DWORD cb; DWORD PageFaultCount; size_t PeakWorkingSetSize, WorkingSetSize, QuotaPeakPagedPoolUsage, QuotaPagedPoolUsage, QuotaPeakNonPagedPoolUsage, QuotaNonPagedPoolUsage, PagefileUsage, PeakPagefileUsage; };
typedef PROCESS_MEMORY_COUNTERS* PPROCESS_MEMORY_COUNTERS;
inline BOOL GetProcessMemoryInfo(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD) { return 0; }

// ---- additional MFC GUI classes (depend on CWnd/CFile defined above) ----
class CStdioFile : public CFile {
 public:
  CStdioFile() {}
  CStdioFile(const char* path, UINT mode) { Open(path, mode); }
  BOOL ReadString(CString& s) {
    if (!fp_) return FALSE; char buf[8192];
    if (!std::fgets(buf, sizeof(buf), fp_)) return FALSE;
    size_t n = strlen(buf); while (n && (buf[n-1]=='\n'||buf[n-1]=='\r')) buf[--n]=0;
    s = buf; return TRUE;
  }
  void WriteString(const char* s) { if (s) Write(s, (UINT)strlen(s)); }
};
class CMenu : public CObject { public: BOOL CreateMenu() { return 1; } BOOL AppendMenuA(UINT, UINT_PTR = 0, const char* = nullptr) { return 1; } BOOL DestroyMenu() { return 1; } HMENU GetSafeHmenu() { return nullptr; } };
class CToolTipCtrl : public CWnd { public: BOOL Create(CWnd*, DWORD = 0) { return 1; } BOOL AddTool(CWnd*, const char*, RECT* = nullptr, UINT_PTR = 0) { return 1; } void Activate(BOOL) {} void RelayEvent(MSG*) {} void UpdateTipText(const char*, CWnd*, UINT = 0) {} BOOL SetMaxTipWidth(int) { return 1; } void SetDelayTime(DWORD, int = 0) {} };
// notify-message header (embedded by tree-view notification structs below)
struct NMHDR { HWND hwndFrom; UINT_PTR idFrom; UINT code; };
typedef NMHDR* LPNMHDR;
// tree-view item/insert structures (preferences dialog tree — inert)
#ifndef HISS_HTREEITEM_DEFINED
#define HISS_HTREEITEM_DEFINED
typedef void* HTREEITEM;  // (also typedef'd later; identical)
#endif
struct TVITEMA { UINT mask; HTREEITEM hItem; UINT state, stateMask; char* pszText; int cchTextMax, iImage, iSelectedImage, cChildren; LPARAM lParam; };
typedef TVITEMA TVITEM;
struct TV_INSERTSTRUCT { HTREEITEM hParent; HTREEITEM hInsertAfter; TVITEMA item; };
typedef TV_INSERTSTRUCT TVINSERTSTRUCTA;
typedef TV_INSERTSTRUCT* LPTV_INSERTSTRUCT;
#define TVIF_TEXT 0x0001
#define TVIF_PARAM 0x0004
#define TVIF_IMAGE 0x0002
#define TVI_ROOT ((HTREEITEM)0xFFFF0000)
#define TVI_LAST ((HTREEITEM)0xFFFF0002)
#define TVGN_CARET 0x0009
#define TVN_SELCHANGED (-402)
#define TVN_SELCHANGING (-401)
#define TVN_ITEMEXPANDED (-406)
struct NMTREEVIEW { NMHDR hdr; UINT action; TVITEMA itemOld; TVITEMA itemNew; POINT ptDrag; };
typedef NMTREEVIEW NM_TREEVIEW;
typedef NMTREEVIEW* LPNMTREEVIEW;
struct NMTVDISPINFO { NMHDR hdr; TVITEMA item; };
typedef NMTVDISPINFO TV_DISPINFO;
typedef NMTVDISPINFO* LPNMTVDISPINFO;
#define TVN_GETDISPINFO (-403)
#define TVN_SETDISPINFO (-404)
// property-sheet notifications
#define PSN_FIRST (-200)
#define PSN_SETACTIVE (-200)
#define PSN_KILLACTIVE (-201)
#define PSN_APPLY (-202)
#define PSN_RESET (-203)
#define PSN_HELP (-205)
#define PSN_WIZBACK (-206)
#define PSN_WIZNEXT (-207)
#define PSN_WIZFINISH (-208)
#define PSN_QUERYCANCEL (-209)
#define NM_CLICK (-2)
#define NM_DBLCLK (-3)
#define NM_RETURN (-4)
class CTreeCtrl : public CWnd {
 public:
  HTREEITEM InsertItem(TV_INSERTSTRUCT*) { return nullptr; }
  HTREEITEM InsertItem(const char*, HTREEITEM = nullptr, HTREEITEM = nullptr) { return nullptr; }
  BOOL  DeleteAllItems() { return 1; }
  BOOL  SelectItem(HTREEITEM) { return 1; }
  HTREEITEM GetSelectedItem() const { return nullptr; }
  LPARAM GetItemData(HTREEITEM) const { return 0; }
  BOOL  SetItemData(HTREEITEM, LPARAM) { return 1; }
};
class CProgressCtrl : public CWnd {};
class CImageList : public CObject {};

// ---- MFC message-map macros → no-ops ----
#ifndef BEGIN_MESSAGE_MAP
#define BEGIN_MESSAGE_MAP(a, b)
#define END_MESSAGE_MAP()
#define ON_COMMAND(a, b)
#define ON_COMMAND_RANGE(a, b, c)
#define ON_MESSAGE(a, b)
#define ON_WM_CREATE()
#define ON_WM_DESTROY()
#define ON_WM_CLOSE()
#define ON_WM_PAINT()
#define ON_WM_ERASEBKGND()
#define ON_WM_TIMER()
#define ON_WM_SIZE()
#define ON_WM_MOVING()
#define ON_WM_MOVE()
#define ON_WM_SIZING()
#define ON_WM_ACTIVATE()
#define ON_WM_SHOWWINDOW()
#define ON_WM_NCPAINT()
#define ON_WM_GETMINMAXINFO()
#define ON_WM_WINDOWPOSCHANGED()
#define ON_WM_WINDOWPOSCHANGING()
#define ON_WM_SYSCOMMAND()
#define ON_WM_DRAWITEM()
#define ON_WM_MEASUREITEM()
#define ON_WM_INITMENUPOPUP()
#define ON_WM_MENUSELECT()
#define ON_WM_QUERYDRAGICON()
#define ON_WM_HELPINFO()
#define ON_WM_KEYDOWN()
#define ON_WM_CHAR()
#define ON_WM_SETFOCUS()
#define ON_WM_KILLFOCUS()
#define ON_WM_HSCROLL()
#define ON_WM_VSCROLL()
#define ON_WM_CTLCOLOR()
#define ON_WM_LBUTTONDOWN()
#define ON_WM_LBUTTONUP()
#define ON_WM_RBUTTONUP()
#define ON_WM_MBUTTONDOWN()
#define ON_WM_LBUTTONDBLCLK()
#define ON_WM_MOUSEWHEEL()
#define ON_WM_VKEYTOITEM()
#define ON_WM_NCHITTEST()
#define ON_WM_RBUTTONDOWN()
#define ON_WM_MOUSEMOVE()
#define ON_WM_CONTEXTMENU()
#define ON_BN_CLICKED(a, b)
#define ON_EN_KILLFOCUS(a, b)
#define ON_EN_SETFOCUS(a, b)
#define ON_EN_CHANGE(a, b)
#define ON_EN_UPDATE(a, b)
#define ON_BN_DOUBLECLICKED(a, b)
#define ON_LBN_SELCHANGE(a, b)
#define ON_LBN_DBLCLK(a, b)
#define ON_UPDATE_COMMAND_UI(a, b)
#define ON_NOTIFY(a, b, c)
#define ON_CBN_SELCHANGE(a, b)
#endif

// mouse/keyboard autoplayer extra typedefs
typedef int (*mouse_clickdrag_t)(void*, RECT, int);
typedef int (*keyboard_sendstring_t)(void*, RECT, const char*, bool);
typedef int (*keyboard_sendkey_t)(void*, RECT, int);

// HRESULT + COM-ish bits (WindowCapture etc.)
typedef long HRESULT;
#define S_OK 0
#define S_FALSE 1
#define E_FAIL ((HRESULT)0x80004005L)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr)    (((HRESULT)(hr)) <  0)

// tree-control item + global window-text setter
typedef void* HTREEITEM;
inline BOOL SetWindowText(HWND, const char*) { return 1; }
inline BOOL SetWindowTextA(HWND, const char*) { return 1; }

// more Win32 constants the engine references
#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0 0
#define WAIT_TIMEOUT 0x102
#define WAIT_FAILED 0xFFFFFFFF
#define INFINITE 0xFFFFFFFF
#endif
#ifndef ERROR_ALREADY_EXISTS
#define ERROR_ALREADY_EXISTS 183
#define ERROR_FILE_NOT_FOUND 2
#endif
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (~0)
#define SOCKET_ERROR (-1)
#endif
#ifndef PROCESS_VM_READ
#define PROCESS_VM_READ 0x0010
#define PROCESS_QUERY_LIMITED_INFORMATION 0x1000
#define PROCESS_ALL_ACCESS 0x1F0FFF
#endif
#ifndef MONITOR_DEFAULTTONULL
#define MONITOR_DEFAULTTONULL 0
#define MONITOR_DEFAULTTOPRIMARY 1
#define MONITOR_DEFAULTTONEAREST 2
#endif
#ifndef MS_ENHANCED_PROV
#define MS_ENHANCED_PROV "Microsoft Enhanced Cryptographic Provider v1.0"
#define MS_DEF_PROV "Microsoft Base Cryptographic Provider v1.0"
#endif
#ifndef FW_NORMAL
#define FW_NORMAL 400
#define FW_BOLD 700
#endif
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#endif

extern "C" int open(const char*, int, ...);  // declared without <fcntl.h> (F_OK macro clash)
// _sopen_s (share-aware open) → plain open
inline int _sopen_s(int* fh, const char* path, int oflag, int /*shflag*/, int /*pmode*/) {
  *fh = open(path, oflag); return (*fh < 0) ? 1 : 0;
}

// ---- more Win32 handle/typedefs ----
typedef void* HMODULE;
typedef void* HGLOBAL;
typedef void* HLOCAL;
typedef void* PVOID;
typedef void* HMONITOR;
typedef void* HHOOK;
typedef void* HRGN;
typedef void* HKL;
typedef unsigned short USHORT;
typedef unsigned char  UCHAR;

// notify header / common-control structs
// (NMHDR is defined earlier, before the tree-view structures that embed it)

// winsock (chat/network subsystem — inert)
struct WSADATA { WORD wVersion; WORD wHighVersion; char szDescription[257]; char szSystemStatus[129]; unsigned short iMaxSockets, iMaxUdpDg; char* lpVendorInfo; };
inline int WSAStartup(WORD, WSADATA*) { return 0; }
inline int WSACleanup() { return 0; }
#define LOWORD(l) ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l) ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w) ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w) ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))
#define MAKELONG(a, b) ((LONG)(((WORD)(a)) | (((DWORD)((WORD)(b))) << 16)))
#define MAKEWORD(a, b) ((WORD)(((BYTE)(a)) | (((WORD)((BYTE)(b))) << 8)))

// crypto error codes (MD5_Checksum)
#ifndef NTE_EXISTS
#define NTE_EXISTS ((HRESULT)0x8009000FL)
#define NTE_BAD_KEYSET ((HRESULT)0x80090016L)
#endif

// Win32 function stubs the engine calls
inline BOOL  CreateDirectory(const char*, void*) { return 1; }
inline BOOL  CreateDirectoryA(const char*, void*) { return 1; }
inline BOOL  RemoveDirectory(const char*) { return 1; }
inline BOOL  EnumWindows(void*, LPARAM) { return 1; }
inline BOOL  EnumChildWindows(HWND, void*, LPARAM) { return 1; }
inline BOOL  GetExitCodeProcess(HANDLE, DWORD* code) { if (code) *code = 0; return 1; }
inline HMONITOR MonitorFromRect(const RECT*, DWORD) { return nullptr; }
inline HMONITOR MonitorFromWindow(HWND, DWORD) { return nullptr; }
inline BOOL  GetMonitorInfoA(HMONITOR, void*) { return 1; }
inline void  AfxEndThread(UINT, BOOL = 1) {}
inline BOOL  AfxWinInit(HINSTANCE, HINSTANCE, char*, int) { return 1; }
inline BOOL  EndDialog(HWND, INT_PTR = 0) { return 1; }
inline BOOL  EndDialog(INT_PTR) { return 1; }
inline BOOL  SetWindowPos(HWND, HWND, int, int, int, int, UINT) { return 1; }
inline BOOL  GetWindowThreadProcessId(HWND, DWORD* pid) { if (pid) *pid = 0; return 1; }
inline DWORD GetWindowThreadProcessId(HWND, void*) { return 0; }
inline BOOL  PathIsNetworkPath(const char*) { return 0; }
inline BOOL  PathIsNetworkPathA(const char*) { return 0; }

// LARGE_INTEGER / high-res timing
union LARGE_INTEGER { struct { DWORD LowPart; LONG HighPart; }; int64_t QuadPart; };
union ULARGE_INTEGER { struct { DWORD LowPart; DWORD HighPart; }; uint64_t QuadPart; };
typedef LARGE_INTEGER* PLARGE_INTEGER;
inline BOOL QueryPerformanceCounter(LARGE_INTEGER* p) { if (p) p->QuadPart = 0; return 1; }
inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* p) { if (p) p->QuadPart = 1; return 1; }

// WM_COPYDATA payload (inter-process; inert)
struct COPYDATASTRUCT { UINT_PTR dwData; DWORD cbData; void* lpData; };
typedef COPYDATASTRUCT* PCOPYDATASTRUCT;

// ShowWindow constants + process status
#ifndef SW_SHOWNORMAL
#define SW_HIDE 0
#define SW_SHOWNORMAL 1
#define CW_USEDEFAULT ((int)0x80000000)
#define SW_SHOW 5
#define SW_MINIMIZE 6
#define SW_RESTORE 9
#endif
#ifndef STILL_ACTIVE
#define STILL_ACTIVE 259
#endif

// winsock address family / socket-type constants
#ifndef AF_INET
#define AF_INET 2
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define IPPROTO_TCP 6
#define SOL_SOCKET 0xffff
#define SO_REUSEADDR 0x0004
#endif

// TEXT()/_T() string macros (ANSI build → identity)
#ifndef TEXT
#define TEXT(x) x
#endif
#ifndef _T
#define _T(x) x
#endif

// MSVC CRT spellings → POSIX/std
extern "C" long lseek(int, long, int);
inline long _lseek(int fd, long off, int whence) { return lseek(fd, off, whence); }
inline int  _access_s(const char* p, int m) { return _access(p, m); }
inline int  _finite(double x) { return std::isfinite(x) ? 1 : 0; }
inline int  _isnan(double x) { return std::isnan(x) ? 1 : 0; }
#define _tprintf printf
#define _stprintf sprintf
#define _ftprintf fprintf

// CPtrArray (MFC pointer array)
typedef CArray<void*, void*> CPtrArray;

// ---- final batch: misc Win32 API + constants the engine still references ----
inline BOOL  GetWindowText(HWND, char* buf, int n) { if (buf && n) buf[0] = 0; return 0; }
inline int   GetWindowTextLength(HWND) { return 0; }
inline HINSTANCE ShellExecuteA(HWND, const char*, const char*, const char*, const char*, int) { return nullptr; }
#define ShellExecute ShellExecuteA
inline BOOL  GetDiskFreeSpaceExA(const char*, ULARGE_INTEGER* a, ULARGE_INTEGER* b, ULARGE_INTEGER* c) { if(a)a->QuadPart=0; if(b)b->QuadPart=0; if(c)c->QuadPart=0; return 1; }
#define GetDiskFreeSpaceEx GetDiskFreeSpaceExA
inline BOOL  InvalidateRect(HWND, const RECT*, BOOL) { return 1; }
inline void* GetProcAddress(HMODULE, const char*) { return nullptr; }
inline HMODULE LoadLibraryA(const char*) { return nullptr; }
#define LoadLibrary LoadLibraryA
inline BOOL  FreeLibrary(HMODULE) { return 1; }
inline void* LocalAlloc(UINT, size_t n) { return std::calloc(1, n); }
inline void* LocalFree(void* p) { std::free(p); return nullptr; }
inline void* GlobalAlloc(UINT, size_t n) { return std::calloc(1, n); }
inline void* GlobalFree(void* p) { std::free(p); return nullptr; }
#ifndef LPTR
#define LPTR 0x0040
#define LMEM_ZEROINIT 0x0040
#define GPTR 0x0040
#endif
// directory-change-notification filter flags
#ifndef FILE_NOTIFY_CHANGE_FILE_NAME
#define FILE_NOTIFY_CHANGE_FILE_NAME 0x001
#define FILE_NOTIFY_CHANGE_DIR_NAME 0x002
#define FILE_NOTIFY_CHANGE_ATTRIBUTES 0x004
#define FILE_NOTIFY_CHANGE_SIZE 0x008
#define FILE_NOTIFY_CHANGE_LAST_WRITE 0x010
#define FILE_NOTIFY_CHANGE_SECURITY 0x100
#endif
// status-bar pane styles
#ifndef SBPS_NORMAL
#define SBPS_NORMAL 0x0000
#define SBPS_NOBORDERS 0x0100
#define SBPS_POPOUT 0x0200
#define SBPS_OWNERDRAW 0x1000
#define SBPS_STRETCH 0x8000
#define SBPS_DISABLED 0x04
#endif
// SetWindowPos flags
#ifndef SWP_NOZORDER
#define SWP_NOSIZE 0x0001
#define SWP_NOMOVE 0x0002
#define SWP_NOZORDER 0x0004
#define SWP_NOACTIVATE 0x0010
#define SWP_SHOWWINDOW 0x0040
#define HWND_TOP ((HWND)0)
#define HWND_TOPMOST ((HWND)-1)
#define HWND_NOTOPMOST ((HWND)-2)
#endif
// window messages
#ifndef WM_KEYDOWN
#define WM_KEYDOWN 0x0100
#define WM_SIZE 0x0005
#define WM_MOVE 0x0003
#define WM_ACTIVATE 0x0006
#define WM_KEYUP 0x0101
#define WM_CHAR 0x0102
#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONUP 0x0202
#define WM_COMMAND 0x0111
#define WM_TIMER 0x0113
#define WM_CLOSE 0x0010
#define WM_DESTROY 0x0002
#define WM_COPYDATA 0x004A
#define WM_SETTEXT 0x000C
#define WM_GETTEXT 0x000D
#define WM_SETFONT 0x0030
#define WM_NOTIFY 0x004E
#define WM_ERASEBKGND 0x0014
#endif
#define LPSTR_TEXTCALLBACK ((char*)-1L)
#define LPSTR_TEXTCALLBACKA ((char*)-1L)
inline const char* AfxRegisterWndClass(UINT, HCURSOR = nullptr, HBRUSH = nullptr, HICON = nullptr) { return "HissWndClass"; }
inline HINSTANCE AfxGetInstanceHandle() { return nullptr; }
inline BOOL AfxGetResourceHandle() { return 0; }
inline LRESULT DefWindowProcA(HWND, UINT, WPARAM, LPARAM) { return 0; }
inline LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM) { return 0; }
inline BOOL DestroyWindow(HWND) { return 1; }
inline HWND CreateWindowExA(DWORD,const char*,const char*,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,void*){return nullptr;}
#define CreateWindowEx CreateWindowExA
inline ATOM RegisterClassExA(const void*){return 0;}
#define RegisterClassEx RegisterClassExA
inline LRESULT CallWindowProcA(void*, HWND, UINT, WPARAM, LPARAM) { return 0; }
inline LRESULT CallWindowProc(void*, HWND, UINT, WPARAM, LPARAM) { return 0; }
// low-level CRT IO
extern "C" long read(int, void*, unsigned long);
extern "C" long write(int, const void*, unsigned long);
extern "C" int  close(int);
inline long _read(int fd, void* b, unsigned n) { return read(fd, b, n); }
inline long _write(int fd, const void* b, unsigned n) { return write(fd, b, n); }
inline int  _close(int fd) { return close(fd); }

// MFC substring helper
inline BOOL AfxExtractSubString(CString& out, const char* full, int index, char sep = '\n') {
  if (!full) return FALSE; const char* p = full; int cur = 0;
  while (cur < index && *p) { if (*p == sep) cur++; p++; }
  if (cur != index) { out = ""; return FALSE; }
  std::string s; while (*p && *p != sep) s += *p++;
  out = s.c_str(); return TRUE;
}

// ---- memory + remaining Win32/CRT odds & ends ----
#define ZeroMemory(p, n) std::memset((p), 0, (n))
#define CopyMemory(d, s, n) std::memcpy((d), (s), (n))
#define MoveMemory(d, s, n) std::memmove((d), (s), (n))
#define FillMemory(p, n, v) std::memset((p), (v), (n))
inline HANDLE FindFirstChangeNotification(const char*, BOOL, DWORD) { return INVALID_HANDLE_VALUE; }
inline BOOL   FindNextChangeNotification(HANDLE) { return 0; }
inline BOOL   FindCloseChangeNotification(HANDLE) { return 1; }
inline DWORD  GetFileAttributesA(const char*) { return 0xFFFFFFFF; }
#define GetFileAttributes GetFileAttributesA
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#define FILE_ATTRIBUTE_DIRECTORY 0x10
inline int    GetClassNameA(HWND, char* buf, int n) { if (buf && n) buf[0] = 0; return 0; }
#define GetClassName GetClassNameA
#define WM_QUIT 0x0012

// virtual-key codes
#ifndef VK_ESCAPE
#define VK_ESCAPE 0x1B
#define VK_RETURN 0x0D
#define VK_TAB 0x09
#define VK_SPACE 0x20
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#endif

// version symbol
#ifndef VERSION_NUMBER
#define VERSION_NUMBER 1
#endif

// minimal BSD-socket surface (ChatTerminalServer — networking is inert here)
typedef uintptr_t SOCKET_T;
struct in_addr_stub { unsigned long s_addr; };
struct sockaddr_stub { unsigned short sa_family; char sa_data[14]; };
struct sockaddr_in_stub { short sin_family; unsigned short sin_port; in_addr_stub sin_addr; char sin_zero[8]; };
#ifndef HISS_HAVE_SOCKADDR
#define sockaddr     sockaddr_stub
#define sockaddr_in  sockaddr_in_stub
#define in_addr      in_addr_stub
#endif
inline SOCKET socket(int, int, int) { return (SOCKET)~0; }
inline int  bind(SOCKET, const sockaddr_stub*, int) { return -1; }
inline int  listen(SOCKET, int) { return -1; }
inline SOCKET accept(SOCKET, sockaddr_stub*, int*) { return (SOCKET)~0; }
inline int  connect(SOCKET, const sockaddr_stub*, int) { return -1; }
inline int  send(SOCKET, const char*, int, int) { return -1; }
inline int  recv(SOCKET, char*, int, int) { return -1; }
inline int  closesocket(SOCKET) { return 0; }
inline unsigned short htons(unsigned short x) { return (unsigned short)((x << 8) | (x >> 8)); }
inline unsigned long  inet_addr(const char*) { return 0; }
inline int  setsockopt(SOCKET, int, int, const char*, int) { return 0; }
inline int  ioctlsocket(SOCKET, long, unsigned long*) { return 0; }

// global window helpers (engine calls the ::-qualified Win32 forms)
inline BOOL GetWindowRect(HWND, RECT* r) { if (r) { r->left = r->top = r->right = r->bottom = 0; } return 1; }
inline BOOL GetClientRect(HWND, RECT* r) { if (r) { r->left = r->top = r->right = r->bottom = 0; } return 1; }
inline BOOL IsWindowVisible(HWND) { return 0; }
inline HWND GetFocus() { return nullptr; }
inline HWND GetForegroundWindow() { return nullptr; }
inline HWND SetFocus(HWND) { return nullptr; }
inline void ExitProcess(UINT) {}
inline HRESULT SHCreateDirectoryExA(HWND, const char*, void*) { return 0; }
#define SHCreateDirectoryEx SHCreateDirectoryExA
#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK 0x7f000001
#define INADDR_ANY 0
#endif

inline DWORD WaitForMultipleObjects(DWORD, const HANDLE*, BOOL, DWORD) { return 0; }
inline unsigned long  htonl(unsigned long x) { return ((x & 0xff) << 24) | ((x & 0xff00) << 8) | ((x >> 8) & 0xff00) | ((x >> 24) & 0xff); }
inline unsigned long  ntohl(unsigned long x) { return htonl(x); }
inline unsigned short ntohs(unsigned short x) { return htons(x); }
inline BOOL CloseWindow(HWND) { return 1; }
inline BOOL MoveWindow(HWND, int, int, int, int, BOOL = 1) { return 1; }
inline BOOL IsChild(HWND, HWND) { return 0; }
inline BOOL ClientToScreen(HWND, POINT*) { return 1; }
inline BOOL ScreenToClient(HWND, POINT*) { return 1; }
inline HDC  CreateDCA(const char*, const char*, const char*, const void*) { return nullptr; }
#define CreateDC CreateDCA
inline HDC  GetDC(HWND) { return nullptr; }
inline int  ReleaseDC(HWND, HDC) { return 1; }

// COM error wrapper (CPokerTrackerThread try/catch) — inert
class _com_error { public: _com_error(HRESULT = 0) {} HRESULT Error() const { return 0; } const char* ErrorMessage() const { return ""; } CString Description() const { return CString(); } CString Source() const { return CString(); } };

// MFC dialog data-exchange helpers → no-ops (no dialogs headless)
#define DDX_Control(...) ((void)0)
#define DDX_Text(...) ((void)0)
#define DDX_Check(...) ((void)0)
#define DDX_Radio(...) ((void)0)
#define DDX_CBString(...) ((void)0)
#define DDX_CBIndex(...) ((void)0)
#define DDV_MaxChars(...) ((void)0)
#define DDV_MinMaxInt(...) ((void)0)

#ifndef SOMAXCONN
#define SOMAXCONN 128
#endif
// pen styles
#ifndef PS_SOLID
#define PS_SOLID 0
#define PS_DASH 1
#define PS_DOT 2
#define PS_NULL 5
#endif
// DIB bitmap structures (screen-capture paths — inert)
struct RGBQUAD { BYTE rgbBlue, rgbGreen, rgbRed, rgbReserved; };
struct BITMAPINFOHEADER { DWORD biSize; LONG biWidth, biHeight; WORD biPlanes, biBitCount; DWORD biCompression, biSizeImage; LONG biXPelsPerMeter, biYPelsPerMeter; DWORD biClrUsed, biClrImportant; };
struct BITMAPINFO { BITMAPINFOHEADER bmiHeader; RGBQUAD bmiColors[1]; };
typedef BITMAPINFO* PBITMAPINFO;
typedef BITMAPINFO* LPBITMAPINFO;
typedef BITMAPINFOHEADER* PBITMAPINFOHEADER;
#pragma pack(push, 2)
struct BITMAPFILEHEADER { WORD bfType; DWORD bfSize; WORD bfReserved1, bfReserved2; DWORD bfOffBits; };
#pragma pack(pop)
typedef BITMAPFILEHEADER* PBITMAPFILEHEADER;
#define BI_RGB 0
#define DIB_RGB_COLORS 0
#define DIB_PAL_COLORS 1
// thread priorities + font quality
#define THREAD_PRIORITY_NORMAL 0
#define THREAD_PRIORITY_IDLE (-15)
#define THREAD_PRIORITY_LOWEST (-2)
#define THREAD_PRIORITY_BELOW_NORMAL (-1)
#define THREAD_PRIORITY_ABOVE_NORMAL 1
#define THREAD_PRIORITY_HIGHEST 2
#define THREAD_PRIORITY_TIME_CRITICAL 15
#define DEFAULT_QUALITY 0
#define DRAFT_QUALITY 1
#define PROOF_QUALITY 2
#define ANTIALIASED_QUALITY 4
#define DEFAULT_CHARSET 1
#define OUT_DEFAULT_PRECIS 0
#define CLIP_DEFAULT_PRECIS 0
#define DEFAULT_PITCH 0
#define FF_DONTCARE 0
#define ANSI_CHARSET 0
#define OEM_CHARSET 255
#define SYMBOL_CHARSET 2
#define FF_DECORATIVE 80
#define FF_MODERN 48
#define FF_ROMAN 16
#define FF_SCRIPT 64
#define FF_SWISS 32
#define VARIABLE_PITCH 2
#define FIXED_PITCH 1
// raster-operation codes (BitBlt)
#define SRCCOPY 0x00CC0020
#define SRCAND  0x008800C6
#define SRCPAINT 0x00EE0086
#define SRCINVERT 0x00660046
#define BLACKNESS 0x00000042
#define WHITENESS 0x00FF0062
#define PATCOPY 0x00F00021
#define PATINVERT 0x005A0049
#define DSTINVERT 0x00550009
#define NOTSRCCOPY 0x00330008
// window-class styles
#define CS_DBLCLKS 0x0008
#define CS_HREDRAW 0x0002
#define CS_VREDRAW 0x0001
#define CS_OWNDC 0x0020
// GDI BITMAP descriptor
struct BITMAP { LONG bmType, bmWidth, bmHeight, bmWidthBytes; WORD bmPlanes, bmBitsPixel; void* bmBits; };
typedef BITMAP* PBITMAP;
inline int lstrcmpi(const char* a, const char* b) { return strcasecmp(a ? a : "", b ? b : ""); }
inline int lstrcmpiA(const char* a, const char* b) { return strcasecmp(a ? a : "", b ? b : ""); }
inline int lstrcmp(const char* a, const char* b) { return strcmp(a ? a : "", b ? b : ""); }
inline int lstrlen(const char* a) { return a ? (int)strlen(a) : 0; }
inline char* lstrcpy(char* d, const char* s) { return strcpy(d, s ? s : ""); }
// process-creation flags
#define CREATE_SUSPENDED 0x00000004
#define CREATE_NEW_CONSOLE 0x00000010
#define CREATE_NO_WINDOW 0x08000000
#define DETACHED_PROCESS 0x00000008
#define NORMAL_PRIORITY_CLASS 0x00000020
// pointer typedefs
typedef BYTE*  LPBYTE;
typedef WORD*  LPWORD;
typedef LONG*  LPLONG;
typedef int*   LPINT;
typedef float* LPFLOAT;
// window / control styles (dialog GUI — inert)
#define WS_CHILD 0x40000000
#define WS_VISIBLE 0x10000000
#define WS_BORDER 0x00800000
#define WS_TABSTOP 0x00010000
#define WS_GROUP 0x00020000
#define WS_VSCROLL 0x00200000
#define WS_HSCROLL 0x00100000
#define WS_EX_CLIENTEDGE 0x00000200
#define ES_WANTRETURN 0x1000
#define ES_MULTILINE 0x0004
#define ES_AUTOHSCROLL 0x0080
#define ES_READONLY 0x0800
#define BS_AUTOCHECKBOX 0x0003
#define SS_LEFT 0x0000
#define DS_SETFONT 0x40
// DIB section GDI funcs (screen capture — inert)
inline HBITMAP CreateDIBSection(HDC, const BITMAPINFO*, UINT, void** bits, HANDLE, DWORD) { if (bits) *bits = nullptr; return nullptr; }
inline int GetDIBits(HDC, HBITMAP, UINT, UINT, void*, BITMAPINFO*, UINT) { return 0; }
inline int SetDIBits(HDC, HBITMAP, UINT, UINT, const void*, const BITMAPINFO*, UINT) { return 0; }
inline int StretchDIBits(HDC, int, int, int, int, int, int, int, int, const void*, const BITMAPINFO*, UINT, DWORD) { return 0; }
inline int GetObjectA(HGDIOBJ, int, void*) { return 0; }
#define GetObject GetObjectA
inline BOOL BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD) { return 1; }
inline BOOL StretchBlt(HDC, int, int, int, int, HDC, int, int, int, int, DWORD) { return 1; }
inline HBITMAP CreateBitmap(int, int, UINT, UINT, const void*) { return nullptr; }
inline int SetStretchBltMode(HDC, int) { return 0; }
// CreateFile access / share / disposition flags
#define GENERIC_READ  0x80000000
#define GENERIC_WRITE 0x40000000
#define FILE_SHARE_READ  0x1
#define FILE_SHARE_WRITE 0x2
#define OPEN_EXISTING 3
#define CREATE_ALWAYS 2
#define FILE_ATTRIBUTE_NORMAL 0x80
inline HANDLE CreateFileA(const char*, DWORD, DWORD, void*, DWORD, DWORD, HANDLE) { return INVALID_HANDLE_VALUE; }
#define CreateFile CreateFileA
inline BOOL ReadFile(HANDLE, void*, DWORD, DWORD*, void*) { return 0; }
inline BOOL WriteFile(HANDLE, const void*, DWORD, DWORD*, void*) { return 0; }
#ifndef _MAX_PATH
#define _MAX_PATH 4096
#define _MAX_DRIVE 8
#define _MAX_DIR 4096
#define _MAX_FNAME 1024
#define _MAX_EXT 1024
#endif
// file-find (Win32) — inert; engine also uses CFileFind
struct WIN32_FIND_DATAA { DWORD dwFileAttributes; DWORD ftCreationTime[2], ftLastAccessTime[2], ftLastWriteTime[2]; DWORD nFileSizeHigh, nFileSizeLow, dwReserved0, dwReserved1; char cFileName[260]; char cAlternateFileName[14]; };
typedef WIN32_FIND_DATAA WIN32_FIND_DATA;
inline HANDLE FindFirstFileA(const char*, WIN32_FIND_DATAA*) { return INVALID_HANDLE_VALUE; }
inline HANDLE FindFirstFile(const char*, WIN32_FIND_DATAA*) { return INVALID_HANDLE_VALUE; }
inline BOOL FindNextFileA(HANDLE, WIN32_FIND_DATAA*) { return 0; }
inline BOOL FindNextFile(HANDLE, WIN32_FIND_DATAA*) { return 0; }
inline BOOL FindClose(HANDLE) { return 1; }
inline char* ctime_s(char* buf, size_t /*n*/, const time_t* t) { if (buf) ctime_r(t, buf); return buf; }
// global window APIs the window-functions lib calls (inert headless)
inline BOOL ShowWindow(HWND, int) { return 1; }
inline HWND GetDesktopWindow() { return nullptr; }
inline HWND FindWindowA(const char*, const char*) { return nullptr; }
#define FindWindow FindWindowA
inline BOOL IsIconic(HWND) { return 0; }
inline BOOL IsZoomed(HWND) { return 0; }
inline BOOL SetCurrentDirectoryA(const char*) { return 1; }
#define SetCurrentDirectory SetCurrentDirectoryA
inline DWORD GetCurrentDirectoryA(DWORD, char* buf) { if (buf) buf[0] = 0; return 0; }
#define GetCurrentDirectory GetCurrentDirectoryA
inline HANDLE GetProcessHeap() { return nullptr; }
inline void* HeapAlloc(HANDLE, DWORD, size_t n) { return std::calloc(1, n); }
inline void* HeapReAlloc(HANDLE, DWORD, void* p, size_t n) { return std::realloc(p, n); }
inline BOOL HeapFree(HANDLE, DWORD, void* p) { std::free(p); return 1; }
#define HEAP_ZERO_MEMORY 0x8
inline HGDIOBJ GetCurrentObject(HDC, UINT) { return nullptr; }
#define OBJ_BITMAP 7
#define OBJ_PEN 1
#define OBJ_BRUSH 2
#define OBJ_FONT 6
inline HWND GetDlgItem(HWND, int) { return nullptr; }
inline COLORREF GetPixel(HDC, int, int) { return 0; }
inline COLORREF SetPixel(HDC, int, int, COLORREF) { return 0; }
inline HWND GetActiveWindow() { return nullptr; }
inline BOOL OpenClipboard(HWND) { return 1; }
inline BOOL EmptyClipboard() { return 1; }
inline BOOL CloseClipboard() { return 1; }
inline HANDLE SetClipboardData(UINT, HANDLE) { return nullptr; }
inline HANDLE GetClipboardData(UINT) { return nullptr; }
inline void* GlobalLock(HANDLE) { return nullptr; }
inline BOOL GlobalUnlock(HANDLE) { return 1; }
inline HANDLE GlobalAllocHandle(UINT, size_t n) { return std::calloc(1, n); }
#define CF_TEXT 1
#define GMEM_MOVEABLE 0x2
#define GMEM_DDESHARE 0x2000
typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);
struct WNDCLASSA { UINT style; WNDPROC lpfnWndProc; int cbClsExtra, cbWndExtra; HINSTANCE hInstance; HICON hIcon; HCURSOR hCursor; HBRUSH hbrBackground; const char* lpszMenuName; const char* lpszClassName; };
typedef WNDCLASSA WNDCLASS;
inline ATOM RegisterClassA(const WNDCLASSA*) { return 1; }
#define RegisterClass RegisterClassA
inline BOOL UnregisterClassA(const char*, HINSTANCE) { return 1; }
#define UnregisterClass UnregisterClassA
inline HCURSOR LoadCursorA(HINSTANCE, const char*) { return nullptr; }
#define LoadCursor LoadCursorA
inline HICON LoadIconA(HINSTANCE, const char*) { return nullptr; }
#define LoadIcon LoadIconA
#define IDC_ARROW ((const char*)32512)
#define IDI_APPLICATION ((const char*)32512)
inline BOOL EnableWindow(HWND, BOOL) { return 1; }
inline BOOL SetForegroundWindow(HWND) { return 1; }
#define KL_NAMELENGTH 9
#define DEFAULT_GUI_FONT 17
#define SYSTEM_FONT 13
#define ANSI_VAR_FONT 12
#define NULL_BRUSH 5
#define WHITE_BRUSH 0
#define EM_SETSEL 0x00B1
#define EM_REPLACESEL 0x00C2
#define EM_GETSEL 0x00B0
inline BOOL GetMessageA(MSG*, HWND, UINT, UINT) { return 0; }
#define GetMessage GetMessageA
inline BOOL TranslateMessage(const MSG*) { return 1; }
inline LRESULT DispatchMessageA(const MSG*) { return 0; }
#define DispatchMessage DispatchMessageA
inline BOOL IsDialogMessageA(HWND, MSG*) { return 0; }
#define IsDialogMessage IsDialogMessageA
inline BOOL PeekMessageA(MSG*, HWND, UINT, UINT, UINT) { return 0; }
#define PeekMessage PeekMessageA
inline BOOL UpdateWindow(HWND) { return 1; }
inline int GetSystemMetrics(int) { return 0; }
inline BOOL GetCursorPos(POINT* p) { if (p) { p->x = p->y = 0; } return 1; }
inline BOOL SetCursorPos(int, int) { return 1; }
inline void mouse_event(DWORD, DWORD, DWORD, DWORD, UINT_PTR) {}
inline void keybd_event(BYTE, BYTE, DWORD, UINT_PTR) {}
#define KEY_QUERY_VALUE 0x1
#define KEY_SET_VALUE 0x2
#define KEY_ALL_ACCESS 0xF003F
#define KEY_ENUMERATE_SUB_KEYS 0x8
#define WAIT_FOR_CONDITION(x) while (!(x)) { Sleep(50); }
enum MouseButton { MouseLeft = 0, MouseRight = 1, MouseMiddle = 2 };
inline BOOL Beep(DWORD, DWORD) { return 1; }
#define RegOpenKeyEx RegOpenKeyExA
#define RegQueryValueEx RegQueryValueExA
#define RegSetValueEx RegSetValueExA
#define RegCreateKeyEx RegCreateKeyExA
#define PLANES 14
#define BITSPIXEL 12
#define HORZRES 8
#define VERTRES 10
inline bool IsChatAllowed() { return false; }
#define ID_FILE_NEW 0xE100
#define ID_FILE_OPEN 0xE101
#define ID_FILE_SAVE 0xE103
#define ID_FILE_SAVE_AS 0xE104
#define ID_APP_ABOUT 0xE140
#define ID_APP_EXIT 0xE141
#define ID_EDIT_COPY 0xE122
#define ID_EDIT_CUT 0xE123
#define ID_EDIT_PASTE 0xE125
#define ID_VIEW_TOOLBAR 0xE800
#define ID_VIEW_STATUS_BAR 0xE801
#define ID_SEPARATOR 0
inline void RegisterChatMessage(...) {}
inline LRESULT SendMessageTimeoutA(HWND, UINT, WPARAM, LPARAM, UINT, UINT, DWORD_PTR*) { return 0; }
#define SendMessageTimeout SendMessageTimeoutA
#define SMTO_NOTIMEOUTIFNOTHUNG 0x8
#define SMTO_ABORTIFHUNG 0x2
#define SMTO_BLOCK 0x1
inline int GetKeyboardLayoutName(char* buf) { if (buf) buf[0]=0; return 1; }
#define GetKeyboardLayoutNameA GetKeyboardLayoutName
#define _tcscmp strcmp
#define _tcsicmp strcasecmp
#define _tcscpy strcpy
#define _tcslen strlen
#define _tcscat strcat
#define _tcsstr strstr
#define _tcschr strchr
#define _stprintf_s snprintf
#define _vstprintf_s vsnprintf
struct TBBUTTONINFOA { UINT cbSize; DWORD dwMask; int idCommand; int iImage; BYTE fsState, fsStyle; WORD cx; UINT_PTR lParam; char* pszText; int cchText; };
typedef TBBUTTONINFOA TBBUTTONINFO;
struct TBBUTTON { int iBitmap; int idCommand; BYTE fsState, fsStyle; UINT_PTR dwData; INT_PTR iString; };
#define TBIF_TEXT 0x2
#define TBIF_STATE 0x4
#define TBIF_STYLE 0x8
#define TBIF_IMAGE 0x1
#define TBSTATE_ENABLED 0x4
#define TBSTATE_CHECKED 0x1
#define TBSTYLE_CHECK 0x2
#define TBSTYLE_BUTTON 0x0
#define TBSTYLE_SEP 0x1
#define CBRS_TOP 0x1
#define CBRS_BOTTOM 0x2
#define CBRS_LEFT 0x4
#define CBRS_RIGHT 0x8
#define CBRS_ALIGN_TOP CBRS_TOP
#define CBRS_TOOLTIPS 0x10
#define CBRS_FLYBY 0x20
#define CBRS_SIZE_DYNAMIC 0x40
#define CBRS_GRIPPER 0x400000
inline DWORD GetPrivateProfileStringA(const char*, const char*, const char* def, char* buf, DWORD n, const char*) { if (buf && n) { std::strncpy(buf, def?def:"", n); buf[n-1]=0; return (DWORD)std::strlen(buf); } return 0; }
#define GetPrivateProfileString GetPrivateProfileStringA
inline UINT GetPrivateProfileIntA(const char*, const char*, int def, const char*) { return def; }
#define GetPrivateProfileInt GetPrivateProfileIntA
inline BOOL WritePrivateProfileStringA(const char*, const char*, const char*, const char*) { return 1; }
#define WritePrivateProfileString WritePrivateProfileStringA
inline BOOL EqualRect(const RECT* a, const RECT* b) { return a&&b&&a->left==b->left&&a->top==b->top&&a->right==b->right&&a->bottom==b->bottom; }
inline HWND SetActiveWindow(HWND) { return nullptr; }
#define VK_CONTROL 0x11
#define VK_SHIFT 0x10
#define VK_MENU 0x12
#define VK_LWIN 0x5B
#define VK_BACK 0x08
#define VK_DELETE 0x2E
#define VK_LEFT 0x25
#define VK_RIGHT 0x27
#define VK_UP 0x26
#define VK_DOWN 0x28
#define VK_HOME 0x24
#define VK_END 0x23
inline short VkKeyScanA(char) { return 0; }
#define VkKeyScan VkKeyScanA
inline UINT MapVirtualKeyA(UINT, UINT) { return 0; }
#define MapVirtualKey MapVirtualKeyA
inline short GetKeyState(int) { return 0; }
inline short GetAsyncKeyState(int) { return 0; }
inline long RegOpenKeyExW(HKEY, const wchar_t*, DWORD, DWORD, HKEY*) { return 1; }
inline long RegQueryValueExW(HKEY, const wchar_t*, DWORD*, DWORD*, BYTE*, DWORD*) { return 1; }
inline long RegCreateKeyExW(HKEY, const wchar_t*, DWORD, wchar_t*, DWORD, DWORD, void*, HKEY*, DWORD*) { return 1; }
inline long RegSetValueExW(HKEY, const wchar_t*, DWORD, DWORD, const BYTE*, DWORD) { return 1; }
inline HWND GetShellWindow() { return nullptr; }
inline DWORD GetProcessImageFileNameA(HANDLE, char* buf, DWORD) { if (buf) buf[0] = 0; return 0; }
#define GetProcessImageFileName GetProcessImageFileNameA
inline FILE* _fsopen(const char* path, const char* mode, int) { return std::fopen(path, mode); }
inline DWORD GetModuleFileNameA(HMODULE, char* buf, DWORD n) { if (buf && n) buf[0] = 0; return 0; }
#define GetModuleFileName GetModuleFileNameA
#define CAPTUREBLT 0x40000000
#define TRANSPARENT 1
#define OPAQUE 2
#define DT_LEFT 0x0000
#define DT_CENTER 0x0001
#define DT_RIGHT 0x0002
#define DT_TOP 0x0000
#define DT_VCENTER 0x0004
#define DT_SINGLELINE 0x0020
#define DT_NOPREFIX 0x0800
#define DT_WORDBREAK 0x0010
#define DT_CALCRECT 0x0400
#define DT_END_ELLIPSIS 0x8000
#define GWL_STYLE (-16)
#define GWL_EXSTYLE (-20)
#define GWL_WNDPROC (-4)
#define GWL_ID (-12)
#define GWLP_USERDATA (-21)
inline BOOL PatBlt(HDC, int, int, int, int, DWORD) { return 1; }
inline COLORREF SetBkColor(HDC, COLORREF) { return 0; }
inline int SetBkMode(HDC, int) { return 0; }
// COM BSTR wrapper → std::string-backed
class _bstr_t {
 public:
  _bstr_t() {}
  _bstr_t(const char* s) : s_(s ? s : "") {}
  operator const char*() const { return s_.c_str(); }
  const char* operator+() const { return s_.c_str(); }
  size_t length() const { return s_.size(); }
 private:
  std::string s_;
};
typedef wchar_t* BSTR;
// MFC debug assertions → no-ops
#ifndef ASSERT_VALID
#define ASSERT_VALID(p) ((void)0)
#define ASSERT_KINDOF(c, p) ((void)0)
#define ASSERT_POINTER(p, t) ((void)0)
#endif

// k_undefined (underscore spelling) → the engine's kUndefined sentinel
#ifndef k_undefined
#define k_undefined kUndefined
#endif

#endif  // HISS_COMPAT_MFC_COMPAT_H
