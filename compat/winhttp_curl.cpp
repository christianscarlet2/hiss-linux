// winhttp_curl.cpp — WinHTTP API implemented over libcurl.
//
// Maps the WinHTTP handle/call sequence CScarletBeast.cpp uses onto a single
// libcurl easy request. State accumulates across Open/Connect/OpenRequest/
// SendRequest; the transfer runs inside WinHttpSendRequest; the response is then
// drained by QueryHeaders(status) + QueryDataAvailable/ReadData.
#define HISS_NO_SOCKET_STUBS   // we use real sockets via libcurl; skip the shim's inert socket stubs
#include <curl/curl.h>
#include <string>
#include <vector>
#include <mutex>
#include "winhttp.h"

namespace {

std::once_flag g_curl_once;
void EnsureCurl() { std::call_once(g_curl_once, [] { curl_global_init(CURL_GLOBAL_DEFAULT); }); }

// wide (the engine's wchar_t strings are plain ASCII codepoints) -> narrow
std::string W2S(const wchar_t* w) {
  std::string s;
  if (w) for (; *w; ++w) s += (char)*w;
  return s;
}

struct SbHandle { virtual ~SbHandle() {} };  // common base so CloseHandle deletes the right type
struct SbSession : SbHandle { std::string user_agent; };
struct SbConnect : SbHandle { std::string host; int port = 443; };
struct SbRequest : SbHandle {
  SbConnect* conn = nullptr;
  std::string method = "GET";
  std::string path;
  bool secure = true;
  std::vector<std::string> headers;
  std::string response;   // body captured by SendRequest
  long status = 0;
  size_t read_off = 0;    // ReadData cursor
};

size_t WriteCb(char* ptr, size_t size, size_t nmemb, void* userdata) {
  auto* out = static_cast<std::string*>(userdata);
  out->append(ptr, size * nmemb);
  return size * nmemb;
}

}  // namespace

HINTERNET WinHttpOpen(LPCWSTR user_agent, DWORD, LPCWSTR, LPCWSTR, DWORD) {
  EnsureCurl();
  auto* s = new SbSession();
  s->user_agent = W2S(user_agent);
  return s;
}

HINTERNET WinHttpConnect(HINTERNET /*session*/, LPCWSTR host, INTERNET_PORT port, DWORD) {
  auto* c = new SbConnect();
  c->host = W2S(host);
  c->port = port ? port : 443;
  return c;
}

HINTERNET WinHttpOpenRequest(HINTERNET connect, LPCWSTR method, LPCWSTR path, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD flags) {
  auto* r = new SbRequest();
  r->conn = static_cast<SbConnect*>(connect);
  r->method = method ? W2S(method) : "GET";
  r->path = W2S(path);
  r->secure = (flags & WINHTTP_FLAG_SECURE) != 0;
  return r;
}

BOOL WinHttpAddRequestHeaders(HINTERNET request, LPCWSTR headers, DWORD, DWORD) {
  auto* r = static_cast<SbRequest*>(request);
  if (r && headers) r->headers.push_back(W2S(headers));
  return 1;
}

// Splits a CRLF-joined header blob into individual "Name: value" lines.
static void AppendHeaderBlob(SbRequest* r, const std::string& blob) {
  size_t start = 0;
  while (start < blob.size()) {
    size_t end = blob.find("\r\n", start);
    std::string line = blob.substr(start, end == std::string::npos ? std::string::npos : end - start);
    if (!line.empty()) r->headers.push_back(line);
    if (end == std::string::npos) break;
    start = end + 2;
  }
}

BOOL WinHttpSendRequest(HINTERNET request, LPCWSTR headers, DWORD /*headers_len*/,
                        void* body, DWORD body_len, DWORD, uintptr_t) {
  auto* r = static_cast<SbRequest*>(request);
  if (!r || !r->conn) return 0;
  if (headers) AppendHeaderBlob(r, W2S(headers));

  CURL* curl = curl_easy_init();
  if (!curl) return 0;

  std::string url = (r->secure ? "https://" : "http://") + r->conn->host;
  if ((r->secure && r->conn->port != 443) || (!r->secure && r->conn->port != 80))
    url += ":" + std::to_string(r->conn->port);
  url += r->path;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, r->method.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r->response);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Hiss-ScarletBeast/1.0");
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");  // allow gzip

  if (body && body_len) {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (const char*)body);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body_len);
  }

  struct curl_slist* hlist = nullptr;
  for (const auto& h : r->headers) hlist = curl_slist_append(hlist, h.c_str());
  if (hlist) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hlist);

  CURLcode rc = curl_easy_perform(curl);
  if (rc == CURLE_OK)
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &r->status);

  if (hlist) curl_slist_free_all(hlist);
  curl_easy_cleanup(curl);
  return rc == CURLE_OK ? 1 : 0;
}

BOOL WinHttpReceiveResponse(HINTERNET /*request*/, void*) { return 1; }  // done in SendRequest

BOOL WinHttpQueryHeaders(HINTERNET request, DWORD flags, LPCWSTR, void* out, DWORD* out_len, DWORD*) {
  auto* r = static_cast<SbRequest*>(request);
  if (!r) return 0;
  if ((flags & WINHTTP_QUERY_STATUS_CODE) && (flags & WINHTTP_QUERY_FLAG_NUMBER)) {
    if (out && out_len && *out_len >= sizeof(DWORD)) { *(DWORD*)out = (DWORD)r->status; return 1; }
  }
  return 0;
}

BOOL WinHttpQueryDataAvailable(HINTERNET request, DWORD* available) {
  auto* r = static_cast<SbRequest*>(request);
  if (!r || !available) return 0;
  *available = (DWORD)(r->response.size() - r->read_off);
  return 1;
}

BOOL WinHttpReadData(HINTERNET request, void* buf, DWORD buf_len, DWORD* read) {
  auto* r = static_cast<SbRequest*>(request);
  if (!r || !buf) return 0;
  size_t remaining = r->response.size() - r->read_off;
  size_t n = remaining < buf_len ? remaining : buf_len;
  if (n) std::memcpy(buf, r->response.data() + r->read_off, n);
  r->read_off += n;
  if (read) *read = (DWORD)n;
  return 1;
}

BOOL WinHttpCloseHandle(HINTERNET h) {
  delete static_cast<SbHandle*>(h);  // virtual dtor -> correct concrete type freed
  return 1;
}
