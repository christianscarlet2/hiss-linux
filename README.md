# Hiss · Linux Daemon

A headless, **API-native** port of the Hiss poker bot. No Windows, no screen
scraping, no OCR — it talks to **poker.scarletbeast.com** over the documented
machine gate and runs as a `systemd` background service.

- **REST** (`/api/v1/*`) — the felt loop: sit, poll your seat view (hole cards +
  legal actions), act, leave.
- **GraphQL** (`/console/graphql`) — the model/market graph: marketplace stats
  and this model's own marketplace record, so the bot "knows itself."

> The original Windows OpenHoldem code (`c:\www\openholdembot_old`) is **not**
> touched. This is a clean, separate build that lives only on the Linux server.

## Build

```bash
sudo apt-get install -y libcurl4-openssl-dev nlohmann-json3-dev
make
```

## Configure

Copy `hiss.conf.example` → `/etc/hiss/hiss.conf` and set your machine key
(minted in the Vault at `/wallet` → "Machine Key"):

```ini
HISS_BASE=https://poker.scarletbeast.com
HISS_TOKEN=sbp_xxxxxxxxxxxxxxxx
HISS_TABLE=1
HISS_BUYIN=5000      # cents (5000 = $50.00)
HISS_HANDLE=HAL_9000 # marketplace handle (optional)
HISS_POLL=1500       # ms
HISS_DRYRUN=1        # 1 = observe + decide, never act
```

## Run as a service

```bash
sudo make install
sudo systemctl enable --now hiss
journalctl -u hiss -f
```

## Layout

| File | Role |
|------|------|
| `src/http.hpp`  | libcurl JSON client (Bearer auth, GET/POST, GraphQL). |
| `src/api.hpp`   | Scarlet Beast poker client — REST felt loop + GraphQL graph. |
| `src/brain.hpp` | Decision core over the `legal` object. Swap for a stronger model. |
| `src/main.cpp`  | The daemon: probe → sit → poll → act → leave, with signal handling. |

The decision policy in `brain.hpp` is a conservative pot-odds baseline — replace
`Brain::decide()` with your own model; the daemon only needs `{action, amount}`.
