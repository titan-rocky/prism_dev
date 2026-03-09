
import subprocess
import time
import requests
import json
import os
import shutil

API_URL = "http://localhost:9000"

SCENARIOS = [
    {
        "id": 1,
        "name": "Scenario 1: Normal Operations",
        "pre_cmd": [
             {"endpoint": "/api/modbus/request", "data": {"fc": 5, "count": 5, "delay": 0.5}, "sleep": 2}
        ],
        "cmd": {"fc": 3, "count": 15, "delay": 0.5},
        "endpoint": "/api/modbus/request",
        "sleep": 15
    },
    {
        "id": 2,
        "name": "Scenario 2: Reconnaissance Scan",
        "cmd": {"fc": 8, "count": 20, "delay": 0.2},
        "endpoint": "/api/modbus/request",
        "sleep": 10
    },
    {
        "id": 3,
        "name": "Scenario 3: Write Flood Attack",
        "cmd": {"fc": 5, "count": 50, "delay": 0.05},
        "endpoint": "/api/modbus/request",
        "sleep": 10
    },
    {
        "id": 4,
        "name": "Scenario 4: DNP3 Protocol Violation",
        "cmd": {"fc": 130, "count": 10, "delay": 0.5},
        "endpoint": "/api/dnp3/request",
        "sleep": 10
    },
    {
        "id": 5,
        "name": "Scenario 5: Post-Lockout Write Attempt",
        "pre_cmd": [
             # First Trigger Lockout (Scenario 3)
             {"endpoint": "/api/modbus/request", "data": {"fc": 5, "count": 10, "delay": 0.05}, "sleep": 5}
        ],
        "cmd": {"fc": 5, "count": 5, "delay": 0.5},
        "endpoint": "/api/modbus/request",
        "sleep": 5
    }
]

def run_command(cmd, shell=False):
    print(f"CMD: {cmd}")
    subprocess.run(cmd, shell=shell, check=True)

def capture_logs(scenario_id):
    for service in ["prism", "simulator", "hmi"]:
        try:
            container_name = service
            if service == "simulator":
                container_name = "prism_simulator"
            
            res = subprocess.run(
                ["docker", "logs", container_name],
                capture_output=True, text=True
            )
            
            if not res.stdout:
                print(f"Warning: Empty logs for {service}")

            filename = f"logs/scenario_{scenario_id}_{service}.log"
            with open(filename, "w") as f:
                f.write(res.stdout)
                if res.stderr:
                    f.write("\n--- STDERR ---\n")
                    f.write(res.stderr)
        except Exception as e:
            print(f"Error capturing logs for {service}: {e}")

def wait_for_health():
    print("Waiting for HMI API health...")
    for _ in range(30):
        try:
            r = requests.get(f"{API_URL}/health")
            if r.status_code == 200:
                print("Health check passed.")
                return True
        except:
            pass
        time.sleep(1)
    print("Health check failed.")
    return False



def wait_for_prism_ready():
    print("Waiting for Prism to initialize capture...")
    for i in range(30):
        try:
            res = subprocess.run(
                ["docker", "logs", "prism"],
                capture_output=True, text=True
            )
            # Check for the specific log line indicating packet capture has started
            if "PRISM starting on interface" in res.stdout or "PRISM starting on interface" in res.stderr:
                print(f"Prism ready after {i}s. Waiting 3s for pcap init...")
                time.sleep(3)
                return True
        except Exception as e:
            print(f"Error checking Prism logs: {e}")
        time.sleep(1)
    print("Prism failed to initialize capture in time.")
    return False

def run_scenario(sc):
    print(f"\n=== Running {sc['name']} ===")
    
    run_command(["docker", "compose", "down", "-v"])
    run_command(["docker", "compose", "up", "-d", "--build"]) # Build to pick up changes!
    
    if not wait_for_health():
        print("Skipping due to health check failure")
        return
    
    # Wait for Prism to actually start capturing
    if not wait_for_prism_ready():
        print("Skipping scenario due to Prism startup failure")
        return

    if "pre_cmd" in sc:
        print("Executing pre-commands...")
        for pre in sc["pre_cmd"]:
            requests.post(f"{API_URL}{pre['endpoint']}", json=pre['data'])
            time.sleep(pre.get("sleep", 2))

    print(f"Triggering: {sc['cmd']}")
    r = requests.post(f"{API_URL}{sc['endpoint']}", json=sc['cmd'])
    print(f"Response: {r.status_code}")
    
    print(f"Sleeping {sc['sleep']}s...")
    time.sleep(sc['sleep'])

    capture_logs(sc['id'])
    
    print(f"Logs captured for Scenario {sc['id']}")

def main():
    if not os.path.exists("logs"):
        os.makedirs("logs")
        
    for sc in SCENARIOS:
        run_scenario(sc)

if __name__ == "__main__":
    main()
