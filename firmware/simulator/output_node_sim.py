"""
==============================================================================
Project: QNX-Based Fully Wired Real-Time Safety & Automation Orchestrator
Component: Simulated ESP32-C3 Output Actuator & OLED Node
Description:
    Simulates the physical ESP32-C3 Output Node. Listens for validated $CMD
    frames over TCP/Virtual Serial, simulates dual buzzers, 2-channel relays
    (fan and water pump), status LEDs, live ASCII OLED display, fail-safe
    timeout watchdog, and ACK packet transmission.
==============================================================================
"""

import os
import sys
import time
import socket
import argparse
import threading

NODE_ID = 2
FAILSAFE_TIMEOUT_SEC = 2.0

def calculate_xor_checksum(payload: str) -> str:
    """Computes XOR-8 checksum formatted as 2-character hex."""
    checksum = 0
    for ch in payload:
        checksum ^= ord(ch)
    return f"{checksum:02X}"

class OutputNodeSimulator:
    def __init__(self):
        self.buzzer1 = False
        self.buzzer2 = False
        self.relay_fan = False
        self.relay_pump = False
        self.led_yellow = False
        self.led_red = False
        self.status_text = "INITIALIZING"
        self.last_seq = 0
        self.failsafe_active = False
        self.last_cmd_time = time.time()
        self.running = True

    def apply_failsafe(self):
        if not self.failsafe_active:
            self.failsafe_active = True
            self.buzzer1 = True
            self.buzzer2 = False
            self.relay_fan = True      # Exhaust Fan ON
            self.relay_pump = False
            self.led_yellow = True
            self.led_red = True
            self.status_text = "COMM FAILSAFE!"

    def parse_command(self, raw_packet: str) -> tuple:
        """Parses and validates $CMD frame, returns (is_valid, ack_packet_str)."""
        raw_packet = raw_packet.strip()
        if not raw_packet.startswith("$") or "*" not in raw_packet:
            return False, ""

        star_idx = raw_packet.rfind("*")
        payload = raw_packet[1:star_idx]
        checksum_hex = raw_packet[star_idx + 1:]

        expected_chk = calculate_xor_checksum(payload)
        if checksum_hex.upper() != expected_chk.upper():
            return False, "" # Corrupted packet

        tokens = payload.split(",")
        if len(tokens) < 10 or tokens[0] != "CMD":
            return False, ""

        try:
            target_node = int(tokens[1])
            if target_node != NODE_ID:
                return False, ""

            seq = int(tokens[2])
            b1 = bool(int(tokens[3]))
            b2 = bool(int(tokens[4]))
            r_fan = bool(int(tokens[5]))
            r_pump = bool(int(tokens[6]))
            led_y = bool(int(tokens[7]))
            led_r = bool(int(tokens[8]))
            status_msg = tokens[9]

            # Update state
            self.buzzer1 = b1
            self.buzzer2 = b2
            self.relay_fan = r_fan
            self.relay_pump = r_pump
            self.led_yellow = led_y
            self.led_red = led_r
            self.status_text = status_msg
            self.last_seq = seq
            self.last_cmd_time = time.time()
            self.failsafe_active = False

            # Update SQLite Database for Backend/Frontend sync
            try:
                import sqlite3
                db_path = os.path.join(os.path.dirname(__file__), "..", "..", "backend", "qnx_safety.db")
                if os.path.exists(os.path.dirname(db_path)):
                    conn = sqlite3.connect(db_path, timeout=1.0)
                    cur = conn.cursor()
                    cur.execute("""
                        INSERT INTO actuator_states (node_id, sequence_number, updated_at, buzzer1_alarm, buzzer2_warning, relay_fan, relay_pump, led_yellow, led_red, status_text, failsafe_active)
                        VALUES (?, ?, datetime('now'), ?, ?, ?, ?, ?, ?, ?, ?)
                    """, (NODE_ID, seq, int(b1), int(b2), int(r_fan), int(r_pump), int(led_y), int(led_r), status_msg, 0))
                    conn.commit()
                    conn.close()
            except Exception:
                pass

            # Formulate ACK frame
            ack_payload = f"ACK,{NODE_ID},{seq},{int(b1)},{int(b2)},{int(r_fan)},{int(r_pump)},{int(led_y)},{int(led_r)},0"
            ack_chk = calculate_xor_checksum(ack_payload)
            ack_packet = f"${ack_payload}*{ack_chk}\r\n"

            return True, ack_packet

        except (ValueError, IndexError):
            return False, ""

    def render_oled_ui(self):
        """Renders an interactive visual ASCII OLED representation in console."""
        b1_str = "ON " if self.buzzer1 else "OFF"
        b2_str = "ON " if self.buzzer2 else "OFF"
        fan_str = "RUN " if self.relay_fan else "STOP"
        pump_str = "RUN " if self.relay_pump else "STOP"
        y_led_str = "[ON ]" if self.led_yellow else "[OFF]"
        r_led_str = "[ON ]" if self.led_red else "[OFF]"
        link_str = "DOWN (FAILSAFE)" if self.failsafe_active else "LIVE (HEALTHY)"

        try:
            print("\n" + "=" * 70)
            print("                 VIRTUAL ESP32-C3 OUTPUT & OLED NODE")
            print("=" * 70)
            print("  +--------------------------------------------------------------+")
            print("  |                QNX SAFETY & AUTOMATION OLED                  |")
            print("  | ------------------------------------------------------------ |")
            print(f"  |  SYSTEM STATUS : {self.status_text:<44}|")
            print("  |                                                              |")
            print(f"  |  FAN RELAY     : {fan_str:<12} WATER PUMP RELAY : {pump_str:<12}|")
            print(f"  |  PRIMARY ALARM : {b1_str:<12} WARNING BUZZER   : {b2_str:<12}|")
            print(f"  |  YELLOW LED    : {y_led_str:<12} RED LED CRITICAL : {r_led_str:<12}|")
            print("  | ------------------------------------------------------------ |")
            print(f"  |  SEQ: {self.last_seq:<7} LINK STATUS: {link_str:<33}|")
            print("  +--------------------------------------------------------------+")
            print("  [Watchdog Timer]: Fail-safe triggers after 2.0s link inactivity")
            print("=" * 70)
        except Exception:
            pass

def handle_client(client, sim):
    buffer = ""
    try:
        while sim.running:
            data = client.recv(1024).decode('ascii', errors='ignore')
            if not data:
                break
            buffer += data
            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                if line.strip():
                    valid, ack = sim.parse_command(line)
                    if valid and ack:
                        client.sendall(ack.encode('ascii'))
    except Exception:
        pass
    finally:
        try:
            client.close()
        except Exception:
            pass

def run_server(port=9002):
    sim = OutputNodeSimulator()

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("0.0.0.0", port))
    server.listen(10)

    # UI render loop thread
    def ui_thread():
        while sim.running:
            # Check fail-safe watchdog
            if (time.time() - sim.last_cmd_time) > FAILSAFE_TIMEOUT_SEC:
                sim.apply_failsafe()
            sim.render_oled_ui()
            time.sleep(0.5) # 2 Hz UI update

    # Background auto-connector to QNX Master
    def qnx_output_auto_connector():
        while sim.running:
            try:
                qs = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                qs.settimeout(1.0)
                qs.connect((qnx_ip, port))
                qs.setblocking(True)
                print(f"\n[OUTPUT NODE SIM] Connected directly to QNX Master Command Channel ({qnx_ip}:{port})")
                handle_client(qs, sim)
            except Exception:
                pass
            time.sleep(2.0)

    qot = threading.Thread(target=qnx_output_auto_connector, daemon=True)
    qot.start()

    try:
        while sim.running:
            client, addr = server.accept()
            client.setblocking(True)
            ct = threading.Thread(target=handle_client, args=(client, sim), daemon=True)
            ct.start()
    except KeyboardInterrupt:
        sim.running = False
    finally:
        server.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Simulated ESP32-C3 Output Node")
    parser.add_argument("--port", type=int, default=9002, help="TCP port for command receiver (default: 9002)")
    parser.add_argument("--qnx-ip", type=str, default="10.61.30.60", help="QNX Master IP (default: 10.61.30.60)")
    args = parser.parse_args()

    run_server(port=args.port, qnx_ip=args.qnx_ip)
