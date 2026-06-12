// Hiss — Linux server daemon.
//
// A headless, API-native port of the Hiss poker bot: no Windows, no screen
// scraping, no OCR. It talks to poker.scarletbeast.com over the documented
// machine gate — REST for the felt loop, GraphQL for the model/market graph —
// and runs as a systemd background service.
//
// The original Windows OpenHoldem code is NOT touched; this is a fresh build.
//
// Config (env or /etc/hiss/hiss.conf as KEY=VALUE):
//   HISS_BASE   = https://poker.scarletbeast.com
//   HISS_TOKEN  = sbp_xxxxxxxx          (machine key from /wallet)
//   HISS_TABLE  = 1                     (table id to sit at)
//   HISS_BUYIN  = 5000                  (cents; 5000 = $50.00)
//   HISS_HANDLE = HAL_9000              (this model's marketplace handle; optional)
//   HISS_POLL   = 1500                  (poll interval, ms)
//   HISS_DRYRUN = 0                     (1 = observe + decide but never act)

#include "api.hpp"
#include "brain.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

using namespace hiss;

static std::atomic<bool> g_run{true};
static void on_signal(int) { g_run = false; }

static std::string env_or(const char* k, const std::string& def) {
    const char* v = std::getenv(k);
    return v && *v ? std::string(v) : def;
}

// Load KEY=VALUE lines from a config file into the environment (env wins).
static void load_conf(const std::string& path) {
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq), v = line.substr(eq + 1);
        if (!std::getenv(k.c_str())) setenv(k.c_str(), v.c_str(), 1);
    }
}

static void log(const std::string& s) {
    std::time_t t = std::time(nullptr);
    char ts[32];
    std::strftime(ts, sizeof ts, "%Y-%m-%dT%H:%M:%S", std::gmtime(&t));
    std::cout << "[" << ts << "Z] " << s << std::endl;
}

int main(int argc, char** argv) {
    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    std::string conf = (argc > 1) ? argv[1] : "/etc/hiss/hiss.conf";
    load_conf(conf);

    std::string base = env_or("HISS_BASE", "https://poker.scarletbeast.com");
    std::string token = env_or("HISS_TOKEN", "");
    long table = std::stol(env_or("HISS_TABLE", "1"));
    long buyin = std::stol(env_or("HISS_BUYIN", "5000"));
    std::string handle = env_or("HISS_HANDLE", "");
    int poll = std::stoi(env_or("HISS_POLL", "1500"));
    bool dry = env_or("HISS_DRYRUN", "0") == "1";

    if (token.empty()) {
        log("FATAL: HISS_TOKEN is required (machine key from /wallet). Set it in env or hiss.conf.");
        return 2;
    }

    PokerApi api(base, token);
    Brain brain;

    log("Hiss-Linux waking. base=" + base + " table=" + std::to_string(table) +
        (dry ? " [DRY-RUN]" : ""));

    // Identity + a GraphQL probe so the bot "knows itself" in the marketplace.
    try {
        json me = api.me();
        if (me.is_object()) log("Signed in as @" + me.value("username", std::string("?")) +
                                 " chips=" + std::to_string(me.value("chips", 0L)));
        json stats = api.marketplaceStats();
        if (stats.is_object() && stats.contains("data"))
            log("Marketplace: " + stats["data"]["marketplaceStats"].dump());
        if (!handle.empty()) {
            json m = api.modelByHandle(handle);
            if (m.is_object()) log("Self (GraphQL): " + m.dump());
        }
    } catch (const std::exception& e) {
        log(std::string("warn: probe failed: ") + e.what());
    }

    // Take a seat.
    if (!dry) {
        try {
            json s = api.sit(table, buyin);
            log("Sit: HTTP " + std::to_string(api.lastStatus()) + " " + (s.is_null() ? "" : s.dump()));
        } catch (const std::exception& e) { log(std::string("sit failed: ") + e.what()); }
    }

    long mySeat = -1;
    while (g_run) {
        try {
            json view = dry ? api.observe(table) : api.seat(table);
            if (view.is_object()) {
                if (view.contains("you") && view["you"].is_object()) mySeat = view["you"].value("seat_no", -1L);
                long toAct = (view.contains("hand") && view["hand"].is_object()) ? view["hand"].value("to_act", -1L) : -1L;

                if (!dry && mySeat >= 0 && toAct == mySeat) {
                    Decision d = brain.decide(view);
                    json r = api.act(table, d.action, d.amount);
                    log("ACT " + d.action + (d.amount ? "/" + std::to_string(d.amount) : "") +
                        " -> HTTP " + std::to_string(api.lastStatus()));
                } else if (dry && view.contains("hand")) {
                    Decision d = brain.decide(view);
                    log("[dry] would " + d.action + " (to_act=" + std::to_string(toAct) + ")");
                }
            }
        } catch (const std::exception& e) {
            log(std::string("loop error: ") + e.what());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(poll));
    }

    log("Hiss-Linux standing up from the table.");
    if (!dry) {
        try { api.leave(table); } catch (...) {}
    }
    return 0;
}
