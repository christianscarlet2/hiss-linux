// MFC collection shims (CArray, CMap, CStringArray) on std containers.
#ifndef HISS_COMPAT_MFC_COLLECTIONS_H
#define HISS_COMPAT_MFC_COLLECTIONS_H

#include "mfc_string.h"
#include <vector>
#include <map>

// CArray<TYPE, ARG_TYPE>
template <class TYPE, class ARG_TYPE = const TYPE&>
class CArray {
 public:
  int GetSize() const { return (int)v_.size(); }
  int GetCount() const { return (int)v_.size(); }
  bool IsEmpty() const { return v_.empty(); }
  int Add(ARG_TYPE x) { v_.push_back(x); return (int)v_.size() - 1; }
  void SetAt(int i, ARG_TYPE x) { if (i >= 0 && i < (int)v_.size()) v_[i] = x; }
  void SetAtGrow(int i, ARG_TYPE x) { if (i >= (int)v_.size()) v_.resize(i + 1); v_[i] = x; }
  TYPE& ElementAt(int i) { return v_[i]; }
  const TYPE& GetAt(int i) const { return v_[i]; }
  TYPE& GetAt(int i) { return v_[i]; }
  TYPE& operator[](int i) { return v_[i]; }
  const TYPE& operator[](int i) const { return v_[i]; }
  void RemoveAt(int i, int n = 1) { if (i >= 0 && i < (int)v_.size()) v_.erase(v_.begin() + i, v_.begin() + std::min((int)v_.size(), i + n)); }
  void RemoveAll() { v_.clear(); }
  void SetSize(int n) { v_.resize(n); }
  void InsertAt(int i, ARG_TYPE x, int count = 1) { if (i < 0) i = 0; v_.insert(v_.begin() + std::min(i, (int)v_.size()), count, x); }
  void Append(const CArray& o) { v_.insert(v_.end(), o.v_.begin(), o.v_.end()); }
  void Copy(const CArray& o) { v_ = o.v_; }

 private:
  std::vector<TYPE> v_;
};

// CStringArray
class CStringArray {
 public:
  int GetSize() const { return (int)v_.size(); }
  int GetCount() const { return (int)v_.size(); }
  bool IsEmpty() const { return v_.empty(); }
  int Add(const CString& s) { v_.push_back(s); return (int)v_.size() - 1; }
  CString GetAt(int i) const { return v_[i]; }
  void SetAt(int i, const CString& s) { v_[i] = s; }
  void RemoveAll() { v_.clear(); }
  void RemoveAt(int i, int n = 1) { if (i >= 0 && i < (int)v_.size()) v_.erase(v_.begin() + i, v_.begin() + std::min((int)v_.size(), i + n)); }
  CString operator[](int i) const { return v_[i]; }

 private:
  std::vector<CString> v_;
};

// CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>
template <class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class CMap {
 public:
  int GetCount() const { return (int)m_.size(); }
  bool IsEmpty() const { return m_.empty(); }
  void SetAt(ARG_KEY k, ARG_VALUE v) { m_[k] = v; }
  void operator[](ARG_KEY k) { m_[k]; }
  bool Lookup(ARG_KEY k, VALUE& out) const { auto it = m_.find(k); if (it == m_.end()) return false; out = it->second; return true; }
  void RemoveAll() { m_.clear(); }
  void RemoveKey(ARG_KEY k) { m_.erase(k); }

 private:
  std::map<KEY, VALUE> m_;
};

#endif  // HISS_COMPAT_MFC_COLLECTIONS_H
