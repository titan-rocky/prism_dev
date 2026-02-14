# PRISM Documentation

Welcome to the PRISM documentation. This repository hosts the source code and design details for the Passive Real-time Industrial Security Monitor (PRISM).

## Documentation Index

I have organized the detailed documentation into the following semantically ordered sections:

### 1. [Architecture & Role](docs/architecture.md)
**Theory & Design**: Explains PRISM's design as a passive, out-of-band IDS and its operational theory in SCADA networks.

### 2. [Feature Taxonomy](docs/taxonomy.md)
**Detection Theory**: Maps the implemented C++ features to the theoretical feature taxonomy, detailing exactly which metrics (Timing, Entropy, Function Codes) are active in the codebase.

### 3. [Heuristic Parameters](docs/heuristics.md)
**Configuration & Tuning**: Lists the specific tuning values (Window Size, Risk Coefficients, Sigmoid settings) used in the prototype, along with their justifications.

### 4. [Testing Guide](docs/testing.md)
**Verification**: Provides the exact `scada-sim` commands to replicate the "Normal Polling" and "Write Flood" scenarios used for validation.

## Output Interpretation
To assist with manual evaluation, I have documented the specific log formats and linguistic fuzzy sets used by the system.

### 1. Risk Updates
The system outputs a visual risk indicator every time a window is processed.
`RISK [ASCII_BAR] FUZZY_LABEL (p=PROBABILITY)`

**Linguistic Fuzzy Sets**:
The continuous probability `p` is mapped to valid-graded sets for instant operator awareness:
*   **SAFE**: `p < 0.2` (Green zone, normal operations)
*   **SUSPICIOUS**: `0.2 <= p < 0.5` (Elevated stats, but below hysteresis trigger)
*   **THREAT**: `0.5 <= p < 0.8` (Active anomaly detection)
*   **CRITICAL**: `p >= 0.8` (Confirmed high-confidence attack signature)

### 2. Feature Vector (FV) Logs
The evidence behind the risk score is detailed in the `FV` line:
`FV | Cnt:80.00 | Wr:1.00 | Func:1.00 | ...`

*   **Cnt**: Window Volume (packets)
*   **Wr**: Write Ratio (0.0-1.0)
*   **Func**: Function Consistency (0.0-1.0)
*   **Ent**: Address Entropy
*   **IA_m**: Inter-arrival Mean (seconds)