// Scarlet Beast poker API client — REST for the felt loop, GraphQL for the
// model/marketplace graph. This is the API-native replacement for Windows
// screen-scraping: no pixels, no OCR, just the documented machine gate.
#pragma once
#include "http.hpp"
#include <string>

namespace hiss {

class PokerApi {
public:
    PokerApi(const std::string& base, const std::string& token)
        : http_(base, token), base_(base) {}

    // ---- REST (the documented bot API at /api/v1) ----------------------------
    json lobby()                       { return http_.get("/api/v1/tables"); }
    json me()                          { return http_.get("/api/v1/me"); }
    json observe(long table)           { return http_.get("/api/v1/tables/" + std::to_string(table) + "/observe"); }
    // Your seat view: includes your hole cards + the `legal` actions right now.
    json seat(long table)              { return http_.get("/api/v1/tables/" + std::to_string(table)); }
    json sit(long table, long cents)   { return http_.post("/api/v1/tables/" + std::to_string(table) + "/sit", {{"amount", cents}}); }
    json leave(long table)             { return http_.post("/api/v1/tables/" + std::to_string(table) + "/leave", json::object()); }
    json act(long table, const std::string& action, long amount = 0) {
        json b = {{"action", action}}; if (amount) b["amount"] = amount;
        return http_.post("/api/v1/tables/" + std::to_string(table) + "/act", b);
    }

    // ---- GraphQL (the console model graph at /console/graphql) ----------------
    // Used to populate "symbols" the bot reasons over — e.g. how this model
    // ranks in the marketplace, market KPIs — alongside the live table state.
    json marketplaceStats() {
        static const char* Q = "{ marketplaceStats { count totalHands avgBbPer100 } }";
        return http_.graphql(base_ + "/console/graphql", Q);
    }
    json modelByHandle(const std::string& handle) {
        // models() is ranked; we filter client-side by handle.
        static const char* Q =
            "{ models(first: 50) { sku name handle bbPer100 winRate hands rating pricePer100 } }";
        json res = http_.graphql(base_ + "/console/graphql", Q);
        if (res.is_object() && res.contains("data") && res["data"].contains("models")) {
            for (auto& m : res["data"]["models"]) {
                if (m.value("handle", "") == handle) return m;
            }
        }
        return json(nullptr);
    }

    long lastStatus() { return http_.lastStatus(); }

private:
    Http http_;
    std::string base_;
};

} // namespace hiss
