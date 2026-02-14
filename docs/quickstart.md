# Quickstart

## Prerequisites

- Docker & Docker Compose (v2+)
- Bash shell

## Build & Run

```bash
# From the repo root
docker compose up -d --build
```

This starts three containers:

| Container | Role |
|---|---|
| `prism_simulator` | PLC simulator — runs Modbus (502), DNP3 (20000), IEC-104 (2404) servers |
| `hmi` | HMI client — exposes REST API on `localhost:9000` for triggering SCADA requests |
| `prism` | IDS — passively sniffs traffic on the simulator's network interface |

## Run the Demo

In one terminal, watch PRISM's output:

```bash
docker logs -f prism
```

In another terminal, run the demo script:

```bash
./exec_demo.sh
```

The script walks through 6 scenarios with keypress pauses between each:

1. **Normal Operations** — Modbus FC3 reads (baseline, no alert expected)
2. **Reconnaissance** — Modbus FC8 diagnostics (SUSPICIOUS ~27%)
3. **Flooding Attack** — Modbus FC5 rapid writes (CRITICAL 99%+, triggers safety coil)
4. **Protocol Anomaly** — DNP3 FC130 unsolicited response (CRITICAL 99%+)
5. **Post-Lockout Write** — FC5 write attempt after lockout (BLOCKED by simulator)
6. **Read During Lockout** — FC3 read during lockout (succeeds normally)

## Checking Logs

```bash
# PRISM detection and enforcement output
docker logs prism

# Simulator server logs (safety coil status, blocked writes)
docker logs prism_simulator

# HMI logs
docker logs hmi
```

## Re-running the Demo

The demo script automatically resets the safety coil at the start, so you can re-run it without restarting containers:

```bash
./exec_demo.sh
```

## Teardown

```bash
docker compose down -v
```

## HMI API Reference

The HMI exposes a REST API on `localhost:9000`:

| Endpoint | Method | Body | Description |
|---|---|---|---|
| `/health` | GET | — | Health check |
| `/api/modbus/request` | POST | `{"fc":3, "count":5, "delay":1.0}` | Send Modbus requests |
| `/api/dnp3/request` | POST | `{"fc":130, "count":5, "delay":0.5}` | Send DNP3 requests |
| `/api/iec104/request` | POST | `{"fc":45, "count":5, "delay":0.5}` | Send IEC-104 requests |
| `/api/generate` | POST | — | Run default traffic sequence |
