#!/bin/bash

# Definition of Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

SIM_BIN="./src/simulator/build/scada-sim"

echo -e "${BLUE}=== PRISM DEMO EVIDENCE GENERATOR ===${NC}"
echo "This script will generate traffic for 4 distinct scenarios to create evidence logs."
echo -e "${RED}IMPORTANT: You must have PRISM running in another terminal!${NC}"
echo "Run: sudo ./build/prism lo | tee prism_output.log"
echo ""
read -p "Press ENTER when PRISM is running..."

# Function to run Modbus Scenario
run_modbus_scenario() {
    NAME=$1
    DESC=$2
    FC=$3
    COUNT=$4
    DELAY=$5
    
    echo -e "\n${GREEN}[SCENARIO] $NAME${NC}"
    echo "Description: $DESC"
    
    # Start Server in BG
    $SIM_BIN start-server modbus 502 > /dev/null 2>&1 &
    SERVER_PID=$!
    sleep 1 # Wait for server bind

    echo "[$(date '+%H:%M:%S')] Starting Traffic Injection..."
    
    for i in $(seq 1 $COUNT); do
        $SIM_BIN request modbus 502 $FC 10 1
        sleep $DELAY
    done

    echo "[$(date '+%H:%M:%S')] Traffic Complete."
    
    # Kill Server
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null
    sleep 1
}

# Function to run DNP3 Scenario
run_dnp3_scenario() {
    NAME=$1
    DESC=$2
    FC=$3 # Function Code
    COUNT=$4
    DELAY=$5

    echo -e "\n${GREEN}[SCENARIO] $NAME${NC}"
    echo "Description: $DESC"

    # Start Server in BG
    $SIM_BIN start-server dnp3 20000 > /dev/null 2>&1 &
    SERVER_PID=$!
    sleep 1

    echo "[$(date '+%H:%M:%S')] Starting Traffic Injection..."

    for i in $(seq 1 $COUNT); do
        $SIM_BIN request dnp3 20000 $FC
        sleep $DELAY
    done

    echo "[$(date '+%H:%M:%S')] Traffic Complete."

    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null
    sleep 1
}

# --- SCENARIO 1: BASELINE ---
# Modbus Read Holding Registers (FC 3)
run_modbus_scenario "1. Normal Operations" "Periodic HMI Polling (FC 3)" 3 5 1.0

echo -e "${BLUE}Waiting 10s for PRISM State Decay...${NC}"
sleep 10

# --- SCENARIO 2: RECONNAISSANCE ---
# Modbus Diagnostics (FC 8)
run_modbus_scenario "2. Reconnaissance Scan" "Diagnostic Function Code Injection (FC 8)" 8 5 0.5

echo -e "${BLUE}Waiting 10s for PRISM State Decay...${NC}"
sleep 10

# --- SCENARIO 3: FLOODING ATTACK ---
# Modbus Write Single Coil (FC 5)
run_modbus_scenario "3. Actuator Stress Test" "High-Velocity Write Injection (FC 5)" 5 20 0.05

echo -e "${BLUE}Waiting 10s for PRISM State Decay...${NC}"
sleep 10

# --- SCENARIO 4: DNP3 ANOMALY ---
# DNP3 Unsolicited Response (FC 130)
run_dnp3_scenario "4. DNP3 Protocol Violation" "Unsolicited Response Injection (FC 130)" 130 5 0.5

echo -e "\n${BLUE}=== GENERATION COMPLETE ===${NC}"
echo "Please stop PRISM (Ctrl+C) and check 'prism_output.log'."
