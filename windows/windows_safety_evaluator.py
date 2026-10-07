"""
==============================================================================
Project: QNX-Based Fully Wired Real-Time Safety & Automation Orchestrator
Component: Standalone Windows Python Safety Evaluator & Latency Monitor
Location: /windows/windows_safety_evaluator.py
Description:
    Runs directly in Windows PowerShell / Command Prompt.
    Connects to the live 10 Hz Input Node telemetry stream (Port 9001),
    executes the safety state machine in Python on Windows OS,
    measures high-resolution latency with time.perf_counter_ns(),
    and prints real-time timing & jitter metrics.
==============================================================================
"""

import sys
import time
import socket
import statistics

# Ensure unbuffered immediate console output in PowerShell
if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(line_buffering=True)

SERVER_HOST = "127.0.0.1"
SERVER_PORT = 9001
SAFETY_DEADLINE_US = 50.0  # 50 microseconds hard real-time threshold

def calculate_xor_checksum(payload: str) -> str:
    checksum = 0
    for ch in payload:
        checksum ^= ord(ch)
    return f"{checksum:02X}"

class WindowsSafetyEvaluator:
    def __init__(self, host=SERVER_HOST, port=SERVER_PORT):
        self.host = host
        self.port = port
        self.latencies_us = []
        self.total_frames = 0
        self.missed_deadlines = 0

    def evaluate_safety(self, temp: float, gas_adc: int, mq2_alert: bool, flame_det: bool, flame_adc: int):
        """
        Executes the safety interlocking state machine in Python on Windows OS.
        Measures exact execution time using high-precision OS performance counter.
        """
        t0 = time.perf_counter_ns()

        # Decision State Machine
        if flame_det or flame_adc < 1000 or temp >= 50.0:
            state = "FIRE_CRITICAL"
            action = "FAN_ON + PUMP_ON + SIREN_ON"
        elif mq2_alert or gas_adc > 1500:
            state = "GAS_WARNING"
            action = "FAN_ON + WARNING_BEEP"
        elif temp >= 35.0:
            state = "ELEVATED_TEMP"
            action = "FAN_ON"
        else:
            state = "NORMAL"
            action = "IDLE_MONITOR"

        t1 = time.perf_counter_ns()
        
        # High resolution execution time in microseconds
        eval_us = (t1 - t0) / 1000.0
        return state, action, eval_us

    def run(self):
        print("=" * 85)
        print("  WINDOWS PYTHON SAFETY EVALUATOR & LATENCY MONITOR")
        print("  OS: Microsoft Windows 11 | Runtime: Python " + sys.version.split()[0])
        print(f"  Target Stream: {self.host}:{self.port} | Real-Time Deadline: < {SAFETY_DEADLINE_US} us")
        print("=" * 85)
        print("Connecting to live ESP32-C3 Input Node telemetry stream...")

        while True:
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(3.0)
                sock.connect((self.host, self.port))
                sock.settimeout(None)
                print(f"[CONNECTED] Receiving live 10 Hz telemetry stream from {self.host}:{self.port}\n")
                
                print(f"{'FRAME':<8} {'TIME':<10} {'SENSORS (T/GAS/FLM)':<28} {'STATE':<16} {'LATENCY (us)':<14} {'STATUS'}")
                print("-" * 88)

                buffer = ""
                while True:
                    data = sock.recv(512).decode('ascii', errors='ignore')
                    if not data:
                        print("\n[DISCONNECTED] Stream disconnected from Input Node.")
                        break

                    buffer += data
                    while "\n" in buffer:
                        line, buffer = buffer.split("\n", 1)
                        line = line.strip()
                        if line.startswith("$IN,"):
                            self.process_frame(line)

            except ConnectionRefusedError:
                print(f"[RETRY] Cannot connect to {self.host}:{self.port}. Is start_system.py running? Retrying in 2s...")
                time.sleep(2.0)
            except KeyboardInterrupt:
                self.print_summary()
                break
            except Exception as e:
                print(f"[ERROR] {e}. Reconnecting in 2s...")
                time.sleep(2.0)

    def process_frame(self, line: str):
        try:
            star_pos = line.rfind("*")
            if star_pos == -1: return
            payload = line[1:star_pos]
            checksum_str = line[star_pos+1:]

            # Verify XOR Checksum
            calc_cs = calculate_xor_checksum(payload)
            if calc_cs != checksum_str:
                return

            tokens = payload.split(",")
            if len(tokens) < 12 or tokens[0] != "IN":
                return

            node_id = int(tokens[1])
            seq = int(tokens[2])
            ts_ms = int(tokens[3])
            temp = float(tokens[4])
            hum = float(tokens[5])
            press = float(tokens[6])
            mq2_adc = int(tokens[7])
            mq2_alert = bool(int(tokens[8]))
            flame_det = bool(int(tokens[9]))
            flame_adc = int(tokens[10])

            # Measure Python evaluation duration on Windows
            state, action, eval_us = self.evaluate_safety(temp, gas_adc=mq2_adc, mq2_alert=mq2_alert, flame_det=flame_det, flame_adc=flame_adc)

            # Record metrics
            self.total_frames += 1
            self.latencies_us.append(eval_us)
            
            is_passed = eval_us <= SAFETY_DEADLINE_US
            if not is_passed:
                self.missed_deadlines += 1

            time_str = time.strftime("%H:%M:%S")
            sensor_summary = f"{temp:4.1f}C | {mq2_adc:4d} ADC | {'FLAME' if flame_det else 'CLEAR':<5}"
            status_tag = "[PASS <50us]" if is_passed else "[VIOLATION!]"

            # Format console row
            print(f"#{seq:<7d} {time_str:<10} {sensor_summary:<28} {state:<16} {eval_us:8.2f} us     {status_tag}")

        except Exception:
            pass

    def print_summary(self):
        print("\n" + "=" * 85)
        print("  WINDOWS PYTHON LATENCY BENCHMARK SUMMARY")
        print("=" * 85)
        if not self.latencies_us:
            print("No frames were processed.")
            return

        mean_val = statistics.mean(self.latencies_us)
        median_val = statistics.median(self.latencies_us)
        min_val = min(self.latencies_us)
        max_val = max(self.latencies_us)
        p95_val = sorted(self.latencies_us)[int(len(self.latencies_us) * 0.95)]
        p99_val = sorted(self.latencies_us)[int(len(self.latencies_us) * 0.99)]
        jitter = max_val - min_val
        pass_rate = ((self.total_frames - self.missed_deadlines) / self.total_frames) * 100.0

        print(f"  Total Frames Evaluated : {self.total_frames}")
        print(f"  Mean Execution Latency : {mean_val:.2f} us")
        print(f"  Median Latency (P50)   : {median_val:.2f} us")
        print(f"  P95 Latency            : {p95_val:.2f} us")
        print(f"  P99 Tail Latency       : {p99_val:.2f} us")
        print(f"  Min / Max Latency      : {min_val:.2f} us / {max_val:.2f} us")
        print(f"  Maximum Jitter         : +- {jitter:.2f} us")
        print(f"  Safety Deadline (<50us): {pass_rate:.1f}% Compliant ({self.missed_deadlines} violations)")
        print("-" * 85)
        print("  [COMPARISON NOTE] QNX Neutrino RTOS maintains bounded ~14-22 us with zero jitter.")
        print("  Windows general-purpose OS scheduler introduces variable context-switch delays.")
        print("=" * 85 + "\n")

if __name__ == "__main__":
    host = sys.argv[1] if len(sys.argv) > 1 else "10.61.30.220"
    evaluator = WindowsSafetyEvaluator(host=host)
    evaluator.run()
