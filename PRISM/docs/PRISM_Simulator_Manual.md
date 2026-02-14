# PRISM Simulator — Usage & Testing Manual

## Overview

`PRISM_simulator` is a lightweight SCADA/ICS traffic generator designed to produce **controlled, protocol‑accurate request traffic** for evaluation and validation of the PRISM intrusion detection framework.

The simulator implements:

- Modbus TCP
- DNP3
- IEC‑60870‑5‑104 (IEC‑104)

Its purpose is to:

- Generate deterministic traffic sequences
- Support repeatable burst & stress scenarios
- Align payload formats with PRISM protocol parsers
- Enable risk‑detection and anomaly research experiments

The simulator intentionally models **minimal functional behaviour** instead of full protocol stacks, ensuring traffic remains:

- structurally valid  
- semantically meaningful  
- research‑reproducible  


## Build & Run

### Build

```
mkdir build
cd build
cmake ..
make
```

Executable:

```
./scada-sim
```


## Command Structure

### Start a protocol server

```
./scada-sim start-server <protocol> <port>
```

### Send a request

```
./scada-sim request <protocol> <port> <fn> [addr val]
```

Protocols:

| Protocol | Keyword | Notes |
|--------|--------|------|
| Modbus TCP | modbus | supports addr + value |
| DNP3 | dnp3 | sends master‑request FC |
| IEC‑104 | iec104 | sends ASDU type‑ID |


## Protocol Behaviour Alignment

PRISM acts as the **traffic consumer**.  
Therefore the simulator is aligned to the **payload structures expected by PRISM parsers**.

This ensures:

- deterministic feature generation
- protocol‑stable semantics
- reproducible risk‑evaluation experiments

The simulator does not attempt to implement:

- multi‑frame sessions
- field‑device state
- vendor‑variant behaviour

Only payload fields required for:

- functionCode
- address
- isRequest
- isWrite

are generated.


## Modbus Mode

### Start Modbus Server

```
./scada-sim start-server modbus 502
```

### Send a Modbus Request

```
./scada-sim request modbus 502 <fn> <addr> <val>
```

Example

```
./scada-sim request modbus 502 6 16 1
```

Semantics:

- Function Code = write request indicator
- Address = holding register index

The server runs persistent TCP and echoes responses.


## DNP3 Mode

### Start DNP3 Server

```
./scada-sim start-server dnp3 20000
```

### Send a DNP3 Request

```
./scada-sim request dnp3 20000 <fc>
```

Example

```
./scada-sim request dnp3 20000 2
```

Behaviour:

- Master‑to‑Outstation direction flag set
- Minimal single‑frame request
- Response echo preserved
- FC exposed to PRISM parser

Used for:

- HST concentration testing
- Burst attack modelling
- Function‑code abuse scenarios


## IEC‑104 Mode

### Start IEC‑104 Server

```
./scada-sim start-server iec104 2404
```

### Send IEC‑104 Requests

```
./scada-sim request iec104 2404 <typeId>
```

Example sequence

```
./scada-sim request iec104 2404 45
./scada-sim request iec104 2404 1
./scada-sim request iec104 2404 100
```

Payload properties:

- APCI start byte = 0x68
- ASDU position aligned to PRISM parser
- Type‑ID placed at byte offset 6
- COT encoded as activation
- IOA = 3‑byte address field

Write‑class is mapped to:

Type IDs 45–50.


## Stress / Burst Testing

### Repeated Request Loop

```
for i in {1..40}; do ./scada-sim request <protocol> <port> <fn>; done
```

Used for:

- High‑frequency burst analysis
- Repeated‑function concentration
- Write‑dominant workload simulation


## Integration with PRISM

Fields consumed by PRISM:

- protocol
- functionCode
- address
- isRequest
- isWrite

These values feed:

- sliding‑window aggregation
- burst metrics
- HST profile models
- risk scoring

Simulator is designed to:

- generate clean traffic traces
- isolate protocol behaviour
- avoid noise / ambiguity
- support research reproducibility


## Known Design Constraints

By design:

- No persistent session state
- No full protocol negotiation
- No vendor‑dependent behaviour
- Only single‑frame exchanges

This guarantees:

- deterministic packet semantics
- stable feature outputs
- consistent risk evaluation


## Recommended Research Workflow

1) Validate protocol parser mappings  
2) Generate controlled traffic sequences  
3) Run sliding‑window feature extraction  
4) Analyse burst + HST behaviour  
5) Tune thresholds post‑observation


The simulator is a **research‑grade traffic instrument**, not a production emulator.
