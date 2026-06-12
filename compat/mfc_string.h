// Minimal MFC CString on top of std::string, for the Linux OpenHoldem port.
// Covers the CString surface the engine actually uses. Built out as the
// compiler demands more; keep additions alphabetical-ish and documented.
#ifndef HISS_COMPAT_MFC_STRING_H
#define HISS_COMPAT_MFC_STRING_H

#include <string>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cctype>
#include <vector>
#include <algorithm>

typedef char TCHAR;
#ifndef _T
#define _T(x) x
#endif
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef const char* LPCTSTR;
typedef const char* LPCSTR;
typedef char* LPTSTR;
typedef char* LPSTR;

class CString {
 public:
  // _c is the FIRST member and always points at s_'s data. MFC's CString is a
  // thin char* wrapper, so the engine freely passes CString objects to printf
  // %s (CParseErrors::Error, write_log, …). std::string isn't layout-compatible
  // with that, so we mirror the MFC trick: reading the object as a char* yields
  // _c. Every mutation calls _u() to re-point it (c_str() can reallocate).
  CString() { _u(); }
  CString(const char* s) : s_(s ? s : "") { _u(); }
  CString(const std::string& s) : s_(s) { _u(); }
  explicit CString(char c, int n = 1) : s_(n, c) { _u(); }
  CString(const CString& o) : s_(o.s_) { _u(); }

  CString& operator=(const CString& o) { s_ = o.s_; return _u(); }
  CString& operator=(const char* s) { s_ = s ? s : ""; return _u(); }
  CString& operator=(char c) { s_ = std::string(1, c); return _u(); }
  CString& operator=(int v) { if (v == 0) s_.clear(); else s_ = std::string(1, (char)v); return _u(); }  // handles `= NULL`
  CString& operator=(long v) { if (v == 0) s_.clear(); return _u(); }
  CString& operator=(std::nullptr_t) { s_.clear(); return _u(); }

  // implicit conversions used throughout the engine
  operator const char*() const { return s_.c_str(); }
  const char* GetString() const { return s_.c_str(); }

  int GetLength() const { return (int)s_.size(); }
  bool IsEmpty() const { return s_.empty(); }
  void Empty() { s_.clear(); _u(); }

  char GetAt(int i) const { return (i >= 0 && i < (int)s_.size()) ? s_[i] : 0; }
  char operator[](int i) const { return GetAt(i); }

  // formatting
  void Format(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    char buf[4096];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    s_ = buf; _u();
  }
  void AppendFormat(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    char buf[4096];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    s_ += buf; _u();
  }

  // search
  int Find(char c, int start = 0) const {
    size_t p = s_.find(c, start < 0 ? 0 : start);
    return p == std::string::npos ? -1 : (int)p;
  }
  int Find(const char* sub, int start = 0) const {
    size_t p = s_.find(sub, start < 0 ? 0 : start);
    return p == std::string::npos ? -1 : (int)p;
  }
  int ReverseFind(char c) const {
    size_t p = s_.rfind(c);
    return p == std::string::npos ? -1 : (int)p;
  }
  int FindOneOf(const char* set) const {
    size_t p = s_.find_first_of(set);
    return p == std::string::npos ? -1 : (int)p;
  }

  // substrings
  CString Mid(int start, int len) const {
    if (start < 0) start = 0;
    if (start >= (int)s_.size()) return CString();
    return CString(s_.substr(start, len));
  }
  CString Mid(int start) const {
    if (start < 0) start = 0;
    if (start >= (int)s_.size()) return CString();
    return CString(s_.substr(start));
  }
  CString Left(int n) const { return CString(s_.substr(0, n < 0 ? 0 : n)); }
  CString Right(int n) const {
    if (n < 0) n = 0;
    if (n > (int)s_.size()) n = (int)s_.size();
    return CString(s_.substr(s_.size() - n));
  }

  // case + trim — MFC returns CString& (mutate in place, allow chaining/assignment)
  CString& MakeLower() { for (auto& c : s_) c = (char)tolower((unsigned char)c); return _u(); }
  CString& MakeUpper() { for (auto& c : s_) c = (char)toupper((unsigned char)c); return _u(); }
  CString& Trim() { TrimLeft(); TrimRight(); return _u(); }
  CString& TrimLeft() { size_t p = s_.find_first_not_of(" \t\r\n"); s_ = (p == std::string::npos) ? "" : s_.substr(p); return _u(); }
  CString& TrimRight() { size_t p = s_.find_last_not_of(" \t\r\n"); s_ = (p == std::string::npos) ? "" : s_.substr(0, p + 1); return _u(); }
  CString& TrimLeft(const char* set) { size_t p = s_.find_first_not_of(set); s_ = (p == std::string::npos) ? "" : s_.substr(p); return _u(); }
  CString& TrimRight(const char* set) { size_t p = s_.find_last_not_of(set); s_ = (p == std::string::npos) ? "" : s_.substr(0, p + 1); return _u(); }
  CString& TrimLeft(char c) { char set[2] = {c, 0}; return TrimLeft(set); }
  CString& TrimRight(char c) { char set[2] = {c, 0}; return TrimRight(set); }
  // Remove all occurrences of a char; returns count removed (MFC semantics)
  int Remove(char c) { int n = 0; std::string o; for (char ch : s_) { if (ch == c) n++; else o += ch; } s_ = o; _u(); return n; }
  void Truncate(int n) { if (n >= 0 && n < (int)s_.size()) s_.resize(n); _u(); }
  void Append(const char* p) { if (p) s_ += p; _u(); }
  void Append(const CString& o) { s_ += o.s_; _u(); }
  void AppendChar(char c) { s_ += c; _u(); }

  int Replace(const char* from, const char* to) {
    int n = 0; size_t p = 0; size_t flen = strlen(from);
    if (flen == 0) return 0;
    while ((p = s_.find(from, p)) != std::string::npos) { s_.replace(p, flen, to); p += strlen(to); n++; }
    _u(); return n;
  }
  int Replace(char from, char to) {
    int n = 0; for (auto& c : s_) if (c == from) { c = to; n++; } _u(); return n;
  }
  void Insert(int i, const char* sub) { if (i < 0) i = 0; if (i > (int)s_.size()) i = (int)s_.size(); s_.insert(i, sub); _u(); }
  void Insert(int i, char c) { if (i < 0) i = 0; if (i > (int)s_.size()) i = (int)s_.size(); s_.insert(i, 1, c); _u(); }
  int Delete(int i, int n = 1) { if (i < 0 || i >= (int)s_.size()) return (int)s_.size(); s_.erase(i, n); _u(); return (int)s_.size(); }

  // comparison
  int Compare(const char* o) const { return strcmp(s_.c_str(), o); }
  int CompareNoCase(const char* o) const { return strcasecmp(s_.c_str(), o); }

  // buffer access (engine pokes raw buffers occasionally)
  char* GetBuffer(int minlen = 0) { if ((int)s_.size() < minlen) s_.resize(minlen); _u(); return &s_[0]; }
  void ReleaseBuffer(int newlen = -1) { if (newlen >= 0) s_.resize(newlen); else s_.resize(strlen(s_.c_str())); _u(); }
  void SetAt(int i, char c) { if (i >= 0 && i < (int)s_.size()) s_[i] = c; _u(); }

  // tokenize (MFC-style)
  CString Tokenize(const char* delims, int& pos) const {
    if (pos < 0) return CString();
    size_t start = s_.find_first_not_of(delims, pos);
    if (start == std::string::npos) { pos = -1; return CString(); }
    size_t end = s_.find_first_of(delims, start);
    if (end == std::string::npos) { pos = (int)s_.size(); return CString(s_.substr(start)); }
    pos = (int)end + 1;
    return CString(s_.substr(start, end - start));
  }

  // operators
  CString& operator+=(const CString& o) { s_ += o.s_; return _u(); }
  CString& operator+=(const char* o) { s_ += o; return _u(); }
  CString& operator+=(char c) { s_ += c; return _u(); }

  friend CString operator+(const CString& a, const CString& b) { return CString(a.s_ + b.s_); }
  friend CString operator+(const CString& a, const char* b) { return CString(a.s_ + b); }
  friend CString operator+(const char* a, const CString& b) { return CString(std::string(a) + b.s_); }
  friend CString operator+(const CString& a, char b) { return CString(a.s_ + b); }

  bool operator==(const CString& o) const { return s_ == o.s_; }
  bool operator==(const char* o) const { return s_ == (o ? o : ""); }
  bool operator!=(const CString& o) const { return s_ != o.s_; }
  bool operator!=(const char* o) const { return s_ != (o ? o : ""); }
  bool operator<(const CString& o) const { return s_ < o.s_; }

  const std::string& str() const { return s_; }

 private:
  const char* _c = "";   // MUST be first: passing CString to %s/varargs reads this as the char*
  std::string s_;
  CString& _u() { _c = s_.c_str(); return *this; }  // re-point after any mutation
};

typedef CString CStringA;

#endif  // HISS_COMPAT_MFC_STRING_H
