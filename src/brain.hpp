// Hiss's decision core. A disciplined, conservative baseline policy over the
// REST seat view's `legal` object — pot-odds aware, never tilts. Swap this out
// for a stronger model; the daemon only needs decide() -> {action, amount}.
#pragma once
#include "http.hpp"
#include <string>

namespace hiss {

struct Decision { std::string action; long amount = 0; };

class Brain {
public:
    // `seat` is the JSON from GET /api/v1/tables/{id} — your view, with hole
    // cards and a `legal` object describing exactly what you may do.
    Decision decide(const json& seat) {
        Decision d{"check", 0};
        if (!seat.is_object() || !seat.contains("hand") || !seat["hand"].is_object()) return {"fold", 0};
        const auto& hand = seat["hand"];
        const json legal = hand.contains("legal") && hand["legal"].is_object() ? hand["legal"] : json::object();

        // Prefer the free option.
        if (legal.value("check", false)) return {"check", 0};

        // Facing a bet: call only when it's cheap relative to the pot (>= 4:1).
        long toCall = legal.value("to_call", 0L);
        long pot = hand.value("pot", 0L);
        if (legal.value("call", false)) {
            if (toCall == 0) return {"call", 0};
            double odds = pot > 0 ? double(pot) / double(toCall) : 0.0;
            if (odds >= 4.0) return {"call", 0};
        }
        // Otherwise fold and live to fight the next hand.
        if (legal.value("fold", false)) return {"fold", 0};
        if (legal.value("call", false)) return {"call", 0};
        return {"check", 0};
    }
};

} // namespace hiss
