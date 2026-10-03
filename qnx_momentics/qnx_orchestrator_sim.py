"""
==============================================================================
Project: QNX-Based Fully Wired Real-Time Safety & Automation Orchestrator
Component: QNX Master Real-Time Safety Engine (Host Simulation Runner)
Description:
    Mirrors the exact QNX C/C++ Real-Time Engine on host environments:
    - Microkernel-style decoupled event loop
    - Monotonic nanosecond timing & latency profiling
    - Checksum validation & sequence tracking
    - Deterministic 6-state safety machine (Normal -> Gas -> Fire -> Faults)
    - Direct disk logging & Backend synchronization queue
==============================================================================
"""

import sys
import time
import socket
import argparse

NODE_ID_INPUT = 1
NODE_ID_OUTPUT = 2

# Safety Threshold Defaults
TEMP_WARN_CELSIUS = 35.0
TEMP_CRIT_CELSIUS = 50.0
MQ2_RAW_WARN = 1000
MQ2_RAW_CRIT = 1800
FLAME_ADC_THRESH = 1000
COMM_TIMEOUT_SEC = 1.0

def calculate_xor_checksum(payload: str) -> str:
    checksum = 0
    for ch in payload:
        checksum ^= ord(ch)
    return f"{checksum:02X}"

class QnxSafetyOrchestrator:
    def __init__(self, in_port=9001, out_port=9002, backend_port=8000):
        self.in_port = in_port
        self.out_port = out_port
        self.backend_port = backend_port

        self.last_seq = 0
        self.total_dropped = 0
        self.total_corrupt = 0
        self.last_valid_packet_time = time.time()
        self.current_state = "NORMAL"
        self.cmd_seq = 1

        self.in_sock = None
        self.out_sock = None
        self.running = True

        # Open local log file
        self.log_file = open("qnx_safety_events.log", "a", encoding="utf-8")
        self.log_event("INFO", "QNX_CORE", "QNX Master Orchestrator Engine Started", 0.0, 0.0)

    def log_event(self, level: str, source: str, desc: str, v1: float, v2: float):
        ts_ns = time.time_ns()
        entry = f"[{ts_ns}] [{level}] [{source}] {desc} | V1={v1:.2f} V2={v2:.2f}\n"
        self.log_file.write(entry)
        self.log_file.flush()

    def connect_nodes(self):
        # Connect to Input Node
        if self.in_sock is None:
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.settimeout(0.2)
                s.connect(("127.0.0.1", self.in_port))
                self.in_sock = s
            except Exception:
                self.in_sock = None

        # Connect to Output Node
        if self.out_sock is None:
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.settimeout(0.2)
                s.connect(("127.0.0.1", self.out_port))
                self.out_sock = s
            except Exception:
                self.out_sock = None

    def evaluate_safety(self, tel: dict, is_comm_healthy: bool) -> tuple:
        """
        Deterministic State Machine:
        Returns: (state_name, b1_alarm, b2_warn, fan, pump, y_led, r_led, status_str)
        """
        # 1. Comm Fault
        if not is_comm_healthy or tel is None:
            if self.current_state != "COMM_FAULT":
                self.log_event("CRITICAL", "SAFETY_ENG", "Communication link failure with Input Node", 0.0, 0.0)
            return "COMM_FAULT", True, False, True, False, True, True, "COMM_LINK_DOWN"

        # 2. Hardware Sensor Fault
        if tel.get("fault_flags", 0) != 0 or tel.get("temp", 0) < -50.0:
            if self.current_state != "SENSOR_FAULT":
                self.log_event("FAULT", "SAFETY_ENG", "Sensor hardware fault flag detected", tel.get("fault_flags", 0), 0.0)
            return "SENSOR_FAULT", False, True, True, False, True, False, "SENSOR_HW_FAULT"

        # 3. Fire Critical
        if tel.get("flame_det", 0) == 1 or tel.get("flame_adc", 4095) < FLAME_ADC_THRESH or tel.get("temp", 0) >= TEMP_CRIT_CELSIUS:
            if self.current_state != "FIRE_CRITICAL":
                self.log_event("CRITICAL", "SAFETY_ENG", "FIRE ALARM TRIPPED - SUPPRESSION ACTIVE", tel.get("temp", 0), tel.get("flame_adc", 0))
            return "FIRE_CRITICAL", True, False, True, True, False, True, "FIRE_CRITICAL"

        # 4. Gas Warning
        if tel.get("mq2_alert", 0) == 1 or tel.get("mq2_adc", 0) >= MQ2_RAW_CRIT:
            if self.current_state != "GAS_WARNING":
                self.log_event("WARN", "SAFETY_ENG", "Gas threshold trip - Exhaust purging", tel.get("mq2_adc", 0), 0.0)
            return "GAS_WARNING", False, True, True, False, True, False, "GAS_LEAK_WARN"

        # 5. Elevated Temperature
        if tel.get("temp", 0) >= TEMP_WARN_CELSIUS:
            return "ELEVATED_TEMP", False, False, True, False, True, False, "ELEVATED_TEMP"

        # 6. Normal
        return "NORMAL", False, False, False, False, False, False, "SYSTEM_NORMAL"

    def run(self):
        print(f"[QNX MASTER] Connecting to Input Node (Port {self.in_port}) & Output Node (Port {self.out_port})...")
        buffer = ""

        while self.running:
            t_start = time.perf_counter_ns()
            self.connect_nodes()

            tel = None
            if self.in_sock:
                try:
                    data = self.in_sock.recv(256).decode('ascii', errors='ignore')
                    if data:
                        buffer += data
                        if "\n" in buffer:
                            line, buffer = buffer.split("\n", 1)
                            line = line.strip()
                            if line.startswith("$") and "*" in line:
                                star_pos = line.rfind("*")
                                payload = line[1:star_pos]
                                chk = line[star_pos+1:]
                                if calculate_xor_checksum(payload) == chk.upper():
                                    tokens = payload.split(",")
                                    if len(tokens) >= 12 and tokens[0] == "IN":
                                        seq = int(tokens[2])
                                        if self.last_seq > 0 and seq > self.last_seq + 1:
                                            self.total_dropped += (seq - self.last_seq - 1)
                                        self.last_seq = seq
                                        self.last_valid_packet_time = time.time()

                                        tel = {
                                            "node_id": int(tokens[1]),
                                            "seq": seq,
                                            "ts_ms": int(tokens[3]),
                                            "temp": float(tokens[4]),
                                            "hum": float(tokens[5]),
                                            "press": float(tokens[6]),
                                            "mq2_adc": int(tokens[7]),
                                            "mq2_alert": int(tokens[8]),
                                            "flame_det": int(tokens[9]),
                                            "flame_adc": int(tokens[10]),
                                            "fault_flags": int(tokens[11])
                                        }
                                else:
                                    self.total_corrupt += 1
                except (socket.timeout, ConnectionResetError, BrokenPipeError):
                    self.in_sock = None

            # Check communication health
            is_comm_healthy = (time.time() - self.last_valid_packet_time) <= COMM_TIMEOUT_SEC

            # Safety Decision
            state, b1, b2, fan, pump, y_led, r_led, status_str = self.evaluate_safety(tel, is_comm_healthy)
            self.current_state = state

            # Transmit Actuator Command
            if self.out_sock:
                try:
                    cmd_payload = f"CMD,{NODE_ID_OUTPUT},{self.cmd_seq},{int(b1)},{int(b2)},{int(fan)},{int(pump)},{int(y_led)},{int(r_led)},{status_str}"
                    chk = calculate_xor_checksum(cmd_payload)
                    packet = f"${cmd_payload}*{chk}\r\n"
                    self.out_sock.sendall(packet.encode('ascii'))
                    self.cmd_seq += 1
                except (socket.timeout, ConnectionResetError, BrokenPipeError):
                    self.out_sock = None

            t_end = time.perf_counter_ns()
            latency_us = (t_end - t_start) / 1000.0

            # Real-Time Dashboard Output
            temp_val = tel.get("temp", 0.0) if tel else 0.0
            gas_val = tel.get("mq2_adc", 0) if tel else 0
            flame_val = tel.get("flame_det", 0) if tel else 0

            sys.stdout.write(
                f"\r[QNX MASTER] State:{state:<14} | T:{temp_val:4.1f}C Gas:{gas_val:4d} Flame:{flame_val} | "
                f"Fan:{int(fan)} Pump:{int(pump)} Buz:{int(b1)} | Latency:{latency_us:5.1f}us Dropped:{self.total_dropped:<3d} "
            )
            sys.stdout.flush()
            time.sleep(0.05) # 20 Hz orchestration loop

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="QNX Master Orchestrator")
    parser.add_argument("--in-port", type=int, default=9001)
    parser.add_argument("--out-port", type=int, default=9002)
    args = parser.parse_args()

    orchestrator = QnxSafetyOrchestrator(in_port=args.in_port, out_port=args.out_port)
    try:
        orchestrator.run()
    except KeyboardInterrupt:
        print("\n[QNX MASTER] Shutting down.")
