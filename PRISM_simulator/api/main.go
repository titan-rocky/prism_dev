package main

import (
    "bytes"
    "bufio"
    "encoding/json"
    "log"
    "math/rand"
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

type LogWriter struct {
    prefix string
}

func (w LogWriter) Write(p []byte) (n int, err error) {
    scanner := bufio.NewScanner(bytes.NewReader(p))
    for scanner.Scan() {
        log.Printf("%s%s", w.prefix, scanner.Text())
    }
    return len(p), nil
}

func runCmd(cmd []string) {
	if len(cmd) == 0 {
		return
	}
	if cmd[0] == "scada-sim" {
		cmd[0] = scadaPath
	}
    
    finalCmd := append([]string{"stdbuf", "-oL", "-eL"}, cmd...)
    
	log.Printf("Executing: %v", finalCmd)
	c := exec.Command(finalCmd[0], finalCmd[1:]...)
    
    c.Stdout = LogWriter{prefix: "[scada-sim] "}
    c.Stderr = LogWriter{prefix: "[scada-sim] "}

	if err := c.Start(); err != nil {
		log.Printf("Error starting command: %v", err)
	}
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
			jitter := time.Duration(rand.Intn(200)-100) * time.Millisecond
			time.Sleep(time.Duration(req.Delay*1000)*time.Millisecond + jitter)
		}
	}()
	w.WriteHeader(http.StatusAccepted)
	w.Write([]byte("Accepted"))
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
			jitter := time.Duration(rand.Intn(200)-100) * time.Millisecond
			time.Sleep(time.Duration(req.Delay*1000)*time.Millisecond + jitter)
		}
	}()
	w.WriteHeader(http.StatusAccepted)
	w.Write([]byte("Accepted"))
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
        // Use stdbuf for servers too
		cmd1 := exec.Command("stdbuf", "-oL", "-eL", scadaPath, "start-server", "modbus", "502")
        cmd1.Stdout = LogWriter{prefix: "[modbus-srv] "}
        cmd1.Stderr = LogWriter{prefix: "[modbus-srv] "}
		if err := cmd1.Start(); err != nil {
			log.Printf("Failed to start Modbus server: %v", err)
		}

		cmd2 := exec.Command("stdbuf", "-oL", "-eL", scadaPath, "start-server", "dnp3", "20000")
        cmd2.Stdout = LogWriter{prefix: "[dnp3-srv] "}
        cmd2.Stderr = LogWriter{prefix: "[dnp3-srv] "}
		if err := cmd2.Start(); err != nil {
			log.Printf("Failed to start DNP3 server: %v", err)
		}

		cmd3 := exec.Command("stdbuf", "-oL", "-eL", scadaPath, "start-server", "iec104", "2404")
        cmd3.Stdout = LogWriter{prefix: "[iec104-srv] "}
        cmd3.Stderr = LogWriter{prefix: "[iec104-srv] "}
		if err := cmd3.Start(); err != nil {
			log.Printf("Failed to start IEC104 server: %v", err)
		}
	} else {
		log.Println("scada-sim binary not found")
	}
}

func main() {
	log.SetFlags(log.LstdFlags | log.Lmicroseconds)
	
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
	log.Println("Starting API server on :8080")
	log.Fatal(http.ListenAndServe(":8080", nil))
}
