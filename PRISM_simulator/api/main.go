// dev: minimal Go HTTP API for triggering simulator requests
package main

import (
    "encoding/json"
    "log"
    "net/http"
    "os"
    "os/exec"
    "strconv"
    "time"
)

type Req struct {
    FC    int     `json:"fc"`
    Count int     `json:"count,omitempty"`
    Delay float64 `json:"delay,omitempty"`
}

var scadaPath = "scada-sim"

func findScada() string {
    candidates := []string{
        "/usr/local/bin/scada-sim",
        "/src/build/scada-sim",
        "/src/build/scada-sim/scada-sim",
        "./build/scada-sim",
        "scada-sim",
    }
    for _, p := range candidates {
        if _, err := os.Stat(p); err == nil {
            return p
        }
    }
    if p, err := exec.LookPath("scada-sim"); err == nil {
        return p
    }
    return "scada-sim"
}

func runCmd(cmd []string) {
    if len(cmd) == 0 {
        return
    }
    // normalize command to use discovered scadaPath when caller used "scada-sim"
    if cmd[0] == "scada-sim" {
        cmd[0] = scadaPath
    }
    c := exec.Command(cmd[0], cmd[1:]...)
    _ = c.Start()
}

func modbusHandler(w http.ResponseWriter, r *http.Request) {
    var req Req
    _ = json.NewDecoder(r.Body).Decode(&req)
    if req.Count <= 0 {
        req.Count = 1
    }
    if req.Delay <= 0 {
        req.Delay = 1.0
    }
    fc := req.FC
    go func() {
        for i := 0; i < req.Count; i++ {
            runCmd([]string{"scada-sim", "request", "modbus", "502", strconv.Itoa(fc), "10", "1"})
            time.Sleep(time.Duration(req.Delay*1000) * time.Millisecond)
        }
    }()
    w.WriteHeader(http.StatusAccepted)
}

func dnp3Handler(w http.ResponseWriter, r *http.Request) {
    var req Req
    _ = json.NewDecoder(r.Body).Decode(&req)
    if req.Count <= 0 {
        req.Count = 1
    }
    if req.Delay <= 0 {
        req.Delay = 1.0
    }
    fc := req.FC
    go func() {
        for i := 0; i < req.Count; i++ {
            runCmd([]string{"scada-sim", "request", "dnp3", "20000", strconv.Itoa(fc)})
            time.Sleep(time.Duration(req.Delay*1000) * time.Millisecond)
        }
    }()
    w.WriteHeader(http.StatusAccepted)
}

func generateHandler(w http.ResponseWriter, r *http.Request) {
    seq := [][]string{
        {"scada-sim", "request", "modbus", "502", "3", "10", "1"},
        {"scada-sim", "request", "modbus", "502", "8", "10", "1"},
        {"scada-sim", "request", "modbus", "502", "5", "20", "1"},
        {"scada-sim", "request", "dnp3", "20000", "130", "5"},
    }
    go func() {
        for _, c := range seq {
            runCmd(c)
            time.Sleep(500 * time.Millisecond)
        }
    }()
    w.WriteHeader(http.StatusAccepted)
}

func startScadaServer() {
    scadaPath = findScada()
    if _, err := os.Stat(scadaPath); err == nil {
        _ = exec.Command(scadaPath, "start-server", "modbus", "502").Start()
        _ = exec.Command(scadaPath, "start-server", "dnp3", "20000").Start()
    } else {
        log.Println("scada-sim binary not found")
    }
}

func main() {
    // Start scada servers only when running as simulator role.
    // HMI should not start servers; it only exposes the client API.
    if os.Getenv("ROLE") == "simulator" {
        startScadaServer()
    }
    http.HandleFunc("/api/modbus/request", modbusHandler)
    http.HandleFunc("/api/dnp3/request", dnp3Handler)
    http.HandleFunc("/api/generate", generateHandler)
    http.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
        w.WriteHeader(http.StatusOK)
        _, _ = w.Write([]byte("ok"))
    })
    log.Fatal(http.ListenAndServe(":8080", nil))
}
