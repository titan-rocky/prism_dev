#!/usr/bin/env bash
set -eu

# Simple demo: call the exposed HMI client API (host:9000)
API=${API:-http://localhost:9000}

ts() { date '+[%Y-%m-%d %H:%M:%S]'; }
log() { echo "$(ts) $*"; }

wait_for_api() {
  until curl -sSf "${API}/health" >/dev/null 2>&1; do sleep 0.5; done
}

trigger_modbus() {
  fc=$1; count=$2; delay=$3
  curl -sS -X POST "${API}/api/modbus/request" \
    -H 'Content-Type: application/json' \
    -d "{\"fc\":${fc},\"count\":${count},\"delay\":${delay}}" >/dev/null
}

trigger_dnp3() {
  fc=$1; count=$2; delay=$3
  curl -sS -X POST "${API}/api/dnp3/request" \
    -H 'Content-Type: application/json' \
    -d "{\"fc\":${fc},\"count\":${count},\"delay\":${delay}}" >/dev/null
}

wait_for_api

# Reset safety coil at start (allows reruns without container restart)
log "--- Resetting safety coil (write-lock off) ---"
docker exec hmi scada-sim request modbus 502 5 65535 0 2>/dev/null || true
sleep 1

echo ""
log "=== Scenario 1: Normal Operations (Modbus FC3 — Read Holding Registers) ==="
log "    Sending 5 read requests at 1.0s intervals (baseline traffic)"
trigger_modbus 3 5 1.0
sleep 5
log "    Done. Check 'docker logs prism' — this should NOT trigger an alert."
echo ""
read -rp "Press ENTER to continue to Scenario 2..."

echo ""
log "=== Scenario 2: Reconnaissance (Modbus FC8 — Diagnostics) ==="
log "    Sending 5 diagnostic probes at 0.5s intervals"
trigger_modbus 8 5 0.5
sleep 5
log "    Done. Check 'docker logs prism' — may trigger a low/medium alert."
echo ""
read -rp "Press ENTER to continue to Scenario 3..."

echo ""
log "=== Scenario 3: Flooding Attack (Modbus FC5 — Write Single Coil) ==="
log "    Sending 20 rapid write commands at 0.05s intervals"
log "    PRISM should detect this and engage the safety coil (write-lock)"
trigger_modbus 5 20 0.05
sleep 5
log "    Done. Check 'docker logs prism' — should trigger CRITICAL + ENFORCEMENT."
log "    Check 'docker logs prism_simulator' — should show SAFETY COIL ENGAGED."
echo ""
read -rp "Press ENTER to continue to Scenario 4..."

echo ""
log "=== Scenario 4: Protocol Anomaly (DNP3 FC130 — Unsolicited Response) ==="
log "    Sending 5 unsolicited response frames at 0.5s intervals"
trigger_dnp3 130 5 0.5
sleep 5
log "    Done. Check 'docker logs prism' — should trigger a CRITICAL alert."
echo ""
read -rp "Press ENTER to continue to Scenario 5..."

echo ""
log "=== Scenario 5: Post-Lockout Write Attempt ==="
log "    Attempting Modbus FC5 write AFTER PRISM engaged safety coil..."
log "    Expected: Write REJECTED (Modbus Exception 0x01 — Illegal Function)"
trigger_modbus 5 3 0.5
sleep 3
log "    Done. Check 'docker logs prism_simulator' — should show BLOCKED messages."
echo ""
read -rp "Press ENTER to continue to Scenario 6..."

echo ""
log "=== Scenario 6: Read Still Works During Lockout ==="
log "    Sending Modbus FC3 read during write-lock..."
log "    Expected: Read succeeds normally (safety coil only blocks writes)"
trigger_modbus 3 3 0.5
sleep 3
log "    Done. Check 'docker logs prism_simulator' — reads should NOT be blocked."
echo ""

log "=== Demo complete ==="
