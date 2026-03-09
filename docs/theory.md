# PRISM — Theoretical Implementation

## Overview

PRISM (Passive Real-time ICS Security Monitor) is an intrusion detection system for Industrial Control Systems (ICS) that operates entirely through passive network sniffing. It monitors SCADA protocol traffic — Modbus TCP, DNP3, and IEC 60870-5-104 — without imposing any latency or load on the operational network.

## Architecture

```
┌──────────────┐       ┌──────────────┐
│   HMI / RTU  │◄─────►│  PLC / RTU   │
│  (Clients)   │ SCADA │  (Servers)   │
└──────────────┘ proto └──────┬───────┘
                              │
                     ┌────────┴────────┐
                     │  Network (TAP)  │
                     └────────┬────────┘
                              │ passive copy
                     ┌────────▼────────┐
                     │     PRISM       │
                     │  (IDS Engine)   │
                     └─────────────────┘
```

In the Docker deployment, PRISM shares the simulator's network namespace (`network_mode: service:simulator`) to receive a passive copy of all traffic — equivalent to a network TAP or SPAN port in a physical deployment.

## Detection Pipeline

PRISM processes packets through a six-stage pipeline:

### 1. Packet Capture (`capture/pcap.cpp`)

Uses libpcap in promiscuous mode to capture raw IP packets. A BPF filter (`"ip"`) pre-filters at the kernel level. The capture layer extracts:
- Source/destination IP and port
- TCP payload (stripped of L2/L3/L4 headers)

### 2. Protocol Classification (`parser/proto_classifier.cpp`)

Classifies packets by destination port:

| Port | Protocol |
|------|----------|
| 502 | Modbus TCP |
| 20000 | DNP3 |
| 2404 | IEC 60870-5-104 |

### 3. Protocol Parsing

Each parser extracts a `ParsedRecord` from the raw payload:

- **Modbus** (`parser/modbus_parser.cpp`) — Decodes MBAP header + PDU: function code, register address, read/write classification
- **DNP3** (`parser/dnp3_parser.cpp`) — Decodes transport + application layer: function code, object headers
- **IEC-104** (`parser/iec104_parser.cpp`) — Decodes APCI + ASDU: type ID, cause of transmission, information object address

A record contains:
```
{protocol, timestamp, functionCode, address, isRequest, isWrite}
```

### 4. Sliding Window (`window/window_manager.cpp`)

Records are grouped into time-based sliding windows:
- **Window size**: 5 seconds
- **Slide interval**: 1 second

Each window contains all `ParsedRecord` entries from the time interval. The window slides forward on every new record, enabling continuous feature computation.

### 5. Feature Extraction (`feature/feat_miner.cpp`)

From each window, an 8-dimensional feature vector is computed:

| Index | Feature | Description |
|-------|---------|-------------|
| 0 | Packet Count | Total records in window |
| 1 | Write Ratio | Fraction of write-class operations (FC 5, 6, 15, 16 for Modbus) |
| 2 | Function Spread | Normalized count of distinct function codes |
| 3 | Entropy | Shannon entropy of function code distribution |
| 4 | Protocol Mix | Fraction of dominant protocol (1.0 = single protocol) |
| 5 | Inter-arrival Mean | Mean time between consecutive packets (seconds) |
| 6 | Inter-arrival StdDev | Standard deviation of inter-arrival times |
| 7 | Density | Ratio of active time to window duration |

### 6. Anomaly Detection

Two complementary detectors score each feature vector:

#### Half-Space Trees (`detection/hst.cpp`)

An unsupervised streaming anomaly detector. HST partitions the feature space using axis-aligned half-spaces and scores points based on how isolated they are from the learned distribution. High scores indicate anomalous feature vectors.

#### Burst Detector (`detection/burst.cpp`)

Detects sudden spikes in packet rate that deviate from the baseline. Captures flooding and denial-of-service patterns that may not register as spatially anomalous in HST.

### Risk Fusion (`risk/risk_model.cpp`)

The HST score and burst score are fused into a single risk probability using a weighted combination. The probability maps to a risk level:

| Probability | Level | Label |
|-------------|-------|-------|
| < 0.2 | Low | SAFE |
| 0.2 – 0.5 | Low | SUSPICIOUS |
| 0.5 – 0.8 | Medium | THREAT |
| > 0.8 | High | CRITICAL |

## Enforcement — Safety Coil Write-Lock

When risk reaches **HIGH** with `restrictWrites = true`, PRISM transitions from passive monitoring to active enforcement using native SCADA protocols.

### Mechanism

PRISM sends a standard **Modbus FC5 (Write Single Coil)** command to safety coil address `0xFFFF` on the PLC:

```
Modbus FC5 → Coil 0xFFFF → Value 0xFF00 (LOCK)
```

This mirrors how real Safety Instrumented Systems (SIS) communicate with PLCs — using the same industrial protocol, not out-of-band channels.

### PLC Behavior When Locked

| Incoming FC | Type | Response |
|-------------|------|----------|
| 1, 2, 3, 4, 8 | Read / Diagnostics | **Normal response** |
| 5, 6, 15, 16 | Write | **Exception Response** (FC \| 0x80, code 0x01 — Illegal Function) |

The PLC continues to serve read requests, maintaining observability while preventing malicious writes.

### Reset

The safety coil is released by writing value `0x0000` to coil `0xFFFF`:

```
Modbus FC5 → Coil 0xFFFF → Value 0x0000 (UNLOCK)
```

### Debouncing

The enforcement engine tracks lock state internally (`locked_` flag). The FC5 signal is sent **exactly once** per attack engagement — subsequent HIGH alerts from the same attack context report "Write-lock active (already engaged)" without re-sending the signal.

## Protocol Reference

### Modbus TCP Function Codes

| FC | Name | Class |
|----|------|-------|
| 1 | Read Coils | Read |
| 2 | Read Discrete Inputs | Read |
| 3 | Read Holding Registers | Read |
| 4 | Read Input Registers | Read |
| 5 | Write Single Coil | Write |
| 6 | Write Single Register | Write |
| 8 | Diagnostics | Read |
| 15 | Write Multiple Coils | Write |
| 16 | Write Multiple Registers | Write |

### DNP3 Function Codes

| FC | Name |
|----|------|
| 1 | Read |
| 2 | Write |
| 3 | Select |
| 4 | Operate / Direct Operate |
| 130 | Unsolicited Response |

### IEC 60870-5-104 Type IDs

| Type | Name |
|------|------|
| 1 | Single Point Information |
| 13 | Measured Value, Short Floating Point |
| 30 | Single Point with Timestamp CP56 |
| 45 | Single Command |
| 46 | Double Command |
| 100 | Interrogation Command |

## Docker Deployment

```
┌──────────────────────────────────────┐
│            sim_net (bridge)          │
│                                      │
│  ┌──────────────┐  ┌──────────────┐  │
│  │  simulator   │  │     hmi      │  │
│  │  (PLC)       │  │  (client)    │  │
│  │  Modbus:502  │  │  API:8080    │  │
│  │  DNP3:20000  │  │  :9000→host  │  │
│  │  IEC104:2404 │  │              │  │
│  └──────┬───────┘  └──────────────┘  │
│         │ shared network namespace   │
│  ┌──────┴───────┐                    │
│  │    prism     │                    │
│  │  (IDS, eth0) │                    │
│  └──────────────┘                    │
└──────────────────────────────────────┘
```

PRISM uses `network_mode: service:simulator` to share the simulator's network namespace. This provides passive read access to all traffic via libpcap — equivalent to a network TAP — without being in the data path.
