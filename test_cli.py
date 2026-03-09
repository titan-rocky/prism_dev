
import argparse
import requests
import subprocess
import time
import os
import json
import sys

API_URL = "http://localhost:9000"

SCENARIOS = [
    {
        "id": 1,
        "name": "Scenario 1: Baseline Operations (Normal Polling)",
        # No pre-commands for baseline
        "cmd": {"fc": 3, "count": 6, "delay": 1.1}, # 6 requests over 8s
        "endpoint": "/api/modbus/request",
        "sleep": 10
    },
    {
        "id": 2,
        "name": "Scenario 2: Reconnaissance via Diagnostics Probing",
        "cmd": {"fc": 8, "count": 5, "delay": 0.8}, # ~5 requests over 7s
        "endpoint": "/api/modbus/request",
        "post_cmd": [
            {"endpoint": "/api/modbus/request", "data": {"fc": 3, "count": 1, "delay": 1.0}, "sleep": 6}
        ],
        "sleep": 10
    },
    {
        "id": 3,
        "name": "Scenario 3: Actuator Flood / Write Storm",
        "cmd": {"fc": 5, "count": 9, "delay": 0.05}, # 9 requests in burst
        "endpoint": "/api/modbus/request",
        "post_cmd": [
            {"endpoint": "/api/modbus/request", "data": {"fc": 3, "count": 1, "delay": 1.0}, "sleep": 6}
        ],
        "sleep": 10
    },
    {
        "id": 4,
        "name": "Scenario 4: DNP3 Unsolicited Response Abuse",
        "cmd": {"fc": 130, "count": 5, "delay": 0.6},
        "endpoint": "/api/dnp3/request",
        "post_cmd": [
            {"endpoint": "/api/modbus/request", "data": {"fc": 3, "count": 1, "delay": 1.0}, "sleep": 6}
        ],
        "sleep": 10
    },
    {
        "id": 5,
        "name": "Scenario 5: Post-Lockout Write Attempt",
        "pre_cmd": [
             # First Trigger Lockout (Scenario 3 - reduced count slightly but still bursty)
             {"endpoint": "/api/modbus/request", "data": {"fc": 5, "count": 10, "delay": 0.05}, "sleep": 5},
             # Attempt blocked writes
             {"endpoint": "/api/modbus/request", "data": {"fc": 5, "count": 2, "delay": 0.6}, "sleep": 2}
        ],
        # Finally, valid reads
        "cmd": {"fc": 3, "count": 2, "delay": 1.0},
        "endpoint": "/api/modbus/request",
        "sleep": 10
    }
]

def wait_for_health():
    print("Checking HMI API health...")
    try:
        r = requests.get(f"{API_URL}/health", timeout=2)
        if r.status_code == 200:
            print("Health check passed.")
            return True
    except:
        pass
    print("Health check failed. Is the environment running? (docker compose up -d)")
    return False

def check_prism_ready():
    # In manual mode, we just check if the container is running and has printed the start message
    # We generally assume the user has waited enough, but a quick check helps.
    try:
        res = subprocess.run(
            ["docker", "logs", "prism"],
            capture_output=True, text=True
        )
        if "PRISM starting on interface" in res.stdout or "PRISM starting on interface" in res.stderr:
            return True
    except:
        pass
    return False

def capture_logs(scenario_id):
    if not os.path.exists("logs"):
        os.makedirs("logs")
        
    for service in ["prism", "simulator", "hmi"]:
        try:
            container_name = service
            if service == "simulator":
                container_name = "prism_simulator"
            
            res = subprocess.run(
                ["docker", "logs", container_name],
                capture_output=True, text=True
            )
            
            filename = f"logs/scenario_{scenario_id}_{service}.log"
            print(f"Writing {filename}...")
            with open(filename, "w") as f:
                f.write(res.stdout)
                if res.stderr:
                    f.write("\n--- STDERR ---\n")
                    f.write(res.stderr)
        except Exception as e:
            print(f"Error capturing logs for {service}: {e}")

def run_scenario(sc_id):
    sc = next((s for s in SCENARIOS if s["id"] == sc_id), None)
    if not sc:
        print(f"Scenario {sc_id} not found.")
        return

    if not wait_for_health():
        return

    print(f"\n=== Executing {sc['name']} ===")
    
    if "pre_cmd" in sc:
        print("Executing pre-commands...")
        for pre in sc["pre_cmd"]:
            try:
                requests.post(f"{API_URL}{pre['endpoint']}", json=pre['data'])
                time.sleep(pre.get("sleep", 2))
            except Exception as e:
                print(f"Error in pre-command: {e}")

    print(f"Triggering: {sc['cmd']}")
    try:
        r = requests.post(f"{API_URL}{sc['endpoint']}", json=sc['cmd'])
        print(f"Response: {r.status_code}")
    except Exception as e:
        print(f"Error triggering command: {e}")
        return

    print(f"Waiting {sc['sleep']}s for processing...")
    time.sleep(sc['sleep'])

    if "post_cmd" in sc:
        print("Executing post-commands (flush)...")
        for post in sc["post_cmd"]:
            try:
                time.sleep(post.get("sleep", 1))
                requests.post(f"{API_URL}{post['endpoint']}", json=post['data'])
            except Exception as e:
                print(f"Error in post-command: {e}")

    capture_logs(sc['id'])
    print("Done.")


def capture_full_log():
    print("\nCapturing FULL CONTINUOUS LOGS...")
    if not os.path.exists("logs"):
        os.makedirs("logs")
        
    for service in ["prism", "simulator", "hmi"]:
        try:
            container_name = service
            if service == "simulator":
                container_name = "prism_simulator"
            
            res = subprocess.run(
                ["docker", "logs", container_name],
                capture_output=True, text=True
            )
            
            filename = f"logs/continuous_{service}.log"
            print(f"Writing {filename}...")
            with open(filename, "w") as f:
                f.write(res.stdout)
                if res.stderr:
                    f.write("\n--- STDERR ---\n")
                    f.write(res.stderr)
        except Exception as e:
            print(f"Error capturing logs for {service}: {e}")


def reset_lock():
    print("Resetting safety coil (write-lock off)...")
    try:
        # Use docker exec to run scada-sim directly from the hmi container to reset the coil
        # Target: modbus server (prism_simulator) on port 502, Unit 1 (implied), FC 5, Addr 65535, Val 0
        subprocess.run(
            ["docker", "exec", "hmi", "./scada-sim", "request", "modbus", "502", "5", "65535", "0"],
            capture_output=True
        )
        time.sleep(1)
    except Exception as e:
        print(f"Error resetting lock: {e}")

def run_all():
    print("\n" + "="*60)
    print("STARTING CONTINUOUS FORENSIC EVALUATION (TC1 -> TC5)")
    print("ALL LOGS WILL BE CAPTURED AT THE END")
    print("="*60 + "\n")
    
    # Warmup to stabilize history/burst detector
    print("Executing Warmup (2 benign packets)...")
    wait_for_health()
    try:
        requests.post(f"{API_URL}/api/modbus/request", json={"fc": 3, "count": 2, "delay": 1.0})
    except:
        pass
    time.sleep(5)
    
    # TC1: Baseline
    reset_lock()
    run_scenario(1)
    
    # TC2: Reconnaissance
    reset_lock()
    run_scenario(2)
    
    # TC3: Write Flood
    reset_lock()
    run_scenario(3)
    
    # TC4: DNP3 Unsolicited
    reset_lock()
    run_scenario(4)
    
    # TC5: Post-Lockout (It re-triggers lock internally)
    reset_lock()
    run_scenario(5)
    
    print("\n" + "="*60)
    print("CONTINUOUS EVALUATION COMPLETE - DUMPING LOGS")
    print("="*60 + "\n")
    
    capture_full_log()

def list_scenarios():
    print("Available Scenarios:")
    for sc in SCENARIOS:
        print(f"  {sc['id']}: {sc['name']}")

def main():
    parser = argparse.ArgumentParser(description="PRISM Test CLI")
    subparsers = parser.add_subparsers(dest="command")

    subparsers.add_parser("list", help="List available scenarios")
    
    run_parser = subparsers.add_parser("run", help="Run a specific scenario (or 0 for ALL)")
    run_parser.add_argument("id", type=int, help="Scenario ID to run (0 for ALL)")

    args = parser.parse_args()

    if args.command == "list":
        list_scenarios()
    elif args.command == "run":
        if args.id == 0:
            run_all()
        else:
            run_scenario(args.id)
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
