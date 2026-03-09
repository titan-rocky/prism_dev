# PRISM — PASSIVE INTRUSION DETECTION SYSTEM FOR SCADA-OT NETWORKS

An intrusion detection system for Industrial Control Systems (ICS) that passively sniffs SCADA network traffic, detects anomalies using streaming machine learning, and enforces write-lock protection using native Modbus safety coils.

## What It Does

1. **Passively captures** Modbus TCP, DNP3, and IEC 60870-5-104 traffic via libpcap
2. **Detects anomalies** using Half-Space Trees and burst analysis on sliding windows
3. **Enforces write-lock** by sending a standard Modbus FC5 command to safety coil `0xFFFF` when a critical threat is detected

## Quick Start

```bash
# Start all containers
docker compose up -d --build

# Run the interactive demo (6 scenarios, keypress-paced)
./exec_demo.sh
```

## Testing

```bash
# Install Python dependency
pip install requests

# List available test scenarios
python test_cli.py list

# Run a single scenario (containers must already be up)
python test_cli.py run 3

# Run the full forensic evaluation (all 5 scenarios, logs captured to logs/)
python test_cli.py run 0
```

Logs are written to `logs/scenario_<id>_<service>.log`. See [Quickstart](docs/quickstart.md) for the full testing reference.

## Documentation

| Document | Contents |
|----------|----------|
| [Quickstart](docs/quickstart.md) | Build, run, demo walkthrough, API reference |
| [Theory](docs/theory.md) | Detection pipeline, feature extraction, anomaly detection, enforcement, protocol reference |

## License

This project is licensed under the **MIT License**. See [LICENSE](LICENSE) for the full text.

---

### Courtesy

This project's documentation has been generated with the help of **Antigravity**, an AI coding agent powered by **Claude Opus 4.6** from **Google DeepMind**.