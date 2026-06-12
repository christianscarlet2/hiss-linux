// server.cpp — HTTP socket layer for the Hiss-Linux daemon.
//
// Pure POSIX (no MFC shim); calls the engine via the extern "C" interface in
// src/engined.cpp. GET /health, POST/GET /decide.
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern "C" void hiss_boot();
extern "C" const char* hiss_decide();

static void Send(int fd, const char* status, const char* body) {
  char hdr[256];
  size_t blen = std::strlen(body);
  int n = std::snprintf(hdr, sizeof(hdr),
    "HTTP/1.1 %s\r\nContent-Type: application/json\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n",
    status, blen);
  if (::write(fd, hdr, n) < 0) {}
  if (::write(fd, body, blen) < 0) {}
}

int main(int argc, char** argv) {
  std::signal(SIGPIPE, SIG_IGN);
  int port = (argc > 1) ? atoi(argv[1]) : 8087;
  hiss_boot();

  int srv = ::socket(AF_INET, SOCK_STREAM, 0);
  int one = 1; ::setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons((unsigned short)port);
  if (::bind(srv, (sockaddr*)&addr, sizeof(addr)) < 0) { std::perror("bind"); return 1; }
  ::listen(srv, 16);
  std::fprintf(stderr, "[hiss] HTTP decision service on :%d  (GET /health, GET|POST /decide)\n", port);

  for (;;) {
    int fd = ::accept(srv, nullptr, nullptr);
    if (fd < 0) continue;
    char req[8192]; ssize_t r = ::read(fd, req, sizeof(req) - 1);
    if (r <= 0) { ::close(fd); continue; }
    req[r] = 0;
    char method[8] = {0}, path[256] = {0};
    std::sscanf(req, "%7s %255s", method, path);
    if (std::strcmp(path, "/health") == 0)
      Send(fd, "200 OK", "{\"status\":\"ok\",\"engine\":\"booted\",\"service\":\"hiss-linux\"}");
    else if (std::strncmp(path, "/decide", 7) == 0)
      Send(fd, "200 OK", hiss_decide());
    else
      Send(fd, "404 Not Found", "{\"error\":\"not found\"}");
    ::close(fd);
  }
  return 0;
}
