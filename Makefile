CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
LDLIBS   := -lcurl
PREFIX   ?= /usr/local

hiss: src/main.cpp src/http.hpp src/api.hpp src/brain.hpp
	$(CXX) $(CXXFLAGS) src/main.cpp -o hiss $(LDLIBS)

install: hiss
	install -Dm755 hiss $(DESTDIR)$(PREFIX)/bin/hiss
	install -Dm644 systemd/hiss.service $(DESTDIR)/etc/systemd/system/hiss.service
	install -dm755 $(DESTDIR)/etc/hiss
	[ -f $(DESTDIR)/etc/hiss/hiss.conf ] || install -Dm640 hiss.conf.example $(DESTDIR)/etc/hiss/hiss.conf
	@echo "Edit /etc/hiss/hiss.conf, then: systemctl enable --now hiss"

clean:
	rm -f hiss
.PHONY: install clean
