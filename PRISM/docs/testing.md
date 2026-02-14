# Testing Commands

I configured the `scada-sim` tool to validate these specific behaviors. You must run the `prism` binary first.

### Start the Simulator Server
```bash
./scada-sim start-server iec104 2404
```

### Scenario A: Normal Polling (Low Risk)
Generates slow, read-only traffic that stays below the risk threshold.
```bash
for i in {1..5}; do 
  ./scada-sim request iec104 2404 1
  sleep 2
done
```

### Scenario B: Write Flood Attack (High Risk)
Generates rapid "Write" commands that trigger the Burst and Write-Risk detectors.
```bash
for i in {1..20}; do 
  ./scada-sim request iec104 2404 45
  sleep 0.1
done
```
