"""
==============================================================================
Project: QNX-Based Fully Wired Real-Time Safety & Automation Orchestrator
Component: Multi-Client Simulated ESP32-C3 Input Node
Description:
    Streams 10 Hz checksum-verified telemetry to ALL connected clients
    concurrently (QNX Master, FastAPI Backend, Test Benches).
==============================================================================
"""

import sys
import time
import math
import random
import socket
import argparse
import threading

NODE_ID = 1

def calculate_xor_checksum(payload: str) -> str:
    checksum = 0
    for ch in payload:
        checksum ^= ord(ch)
    return f"{checksum:02X}"

class InputNodeSimulator:
    def __init__(self, mode="fluctuating"):
        self.seq = 0
        self.start_time = time.time()
        self.mode = mode
        self.running = True
        self.clients = []
        self.lock = threading.Lock()

        self.temp_base = 26.5
        self.hum_base = 52.0
        self.press_base = 1013.25
        self.mq2_adc_base = 350
        self.flame_adc_base = 3800

    def get_sensor_sample(self):
        elapsed = time.time() - self.start_time
        fault_flags = 0

        if self.mode == "normal":
            temp = self.temp_base + 0.3 * math.sin(elapsed * 0.5) + random.uniform(-0.05, 0.05)
            hum = self.hum_base + 0.5 * math.cos(elapsed * 0.3) + random.uniform(-0.1, 0.1)
            press = self.press_base + random.uniform(-0.05, 0.05)
            mq2_adc = int(self.mq2_adc_base + random.randint(-10, 15))
            mq2_alert = 0
            flame_det = 0
            flame_adc = int(self.flame_adc_base + random.randint(-30, 30))

        elif self.mode == "gas_alert":
            temp = self.temp_base + random.uniform(-0.1, 0.1)
            hum = self.hum_base + random.uniform(-0.2, 0.2)
            press = self.press_base
            mq2_adc = int(min(4095, 1850 + 200 * math.sin(elapsed * 1.5) + random.randint(0, 80)))
            mq2_alert = 1
            flame_det = 0
            flame_adc = int(self.flame_adc_base + random.randint(-20, 20))

        elif self.mode == "fire_alert":
            temp = self.temp_base + 18.5 + 5.0 * math.sin(elapsed) + random.uniform(-0.2, 0.2)
            hum = max(10.0, self.hum_base - 25.0)
            press = self.press_base
            mq2_adc = int(2400 + random.randint(-50, 100))
            mq2_alert = 1
            flame_det = 1
            flame_adc = int(random.randint(250, 480))

        elif self.mode == "sensor_fault":
            temp = -999.0
            hum = -999.0
            press = -999.0
            mq2_adc = 4095
            mq2_alert = 0
            flame_det = 0
            flame_adc = 0
            fault_flags = 0x01 | 0x02

        else: # fluctuating demo
            cycle = (int(elapsed) // 10) % 3
            if cycle == 0:
                temp, hum, press, mq2_adc, mq2_alert, flame_det, flame_adc, fault_flags = \
                    self.temp_base + 1.2 * math.sin(elapsed), self.hum_base, self.press_base, 350 + int(20 * math.sin(elapsed)), 0, 0, 3800, 0
            elif cycle == 1:
                temp, hum, press, mq2_adc, mq2_alert, flame_det, flame_adc, fault_flags = \
                    29.5, 48.0, self.press_base, 2100 + int(50 * math.sin(elapsed)), 1, 0, 3750, 0
            else:
                temp, hum, press, mq2_adc, mq2_alert, flame_det, flame_adc, fault_flags = \
                    48.5 + 2.0 * math.sin(elapsed), 25.0, self.press_base, 2800, 1, 1, 320, 0

        timestamp_ms = int(elapsed * 1000)
        return temp, hum, press, mq2_adc, mq2_alert, flame_det, flame_adc, fault_flags, timestamp_ms

    def generate_packet(self) -> str:
        temp, hum, press, mq2_adc, mq2_alert, flame_det, flame_adc, fault_flags, ts_ms = self.get_sensor_sample()
        payload = f"IN,{NODE_ID},{self.seq},{ts_ms},{temp:.2f},{hum:.2f},{press:.2f},{mq2_adc},{mq2_alert},{flame_det},{flame_adc},{fault_flags}"
        checksum = calculate_xor_checksum(payload)
        packet = f"${payload}*{checksum}\r\n"
        self.seq += 1
        return packet

    def broadcast_loop(self):
        """10 Hz continuous packet broadcaster to all connected clients."""
        while self.running:
            packet = self.generate_packet()
            packet_bytes = packet.encode('ascii')

            with self.lock:
                clients_to_send = list(self.clients)

            dead_clients = []
            for c in clients_to_send:
                try:
                    c.sendall(packet_bytes)
                except Exception:
                    dead_clients.append(c)

            if dead_clients:
                with self.lock:
                    for dc in dead_clients:
                        if dc in self.clients:
                            self.clients.remove(dc)
                            try:
                                dc.close()
                            except Exception:
                                pass

            active_cnt = len(clients_to_send) - len(dead_clients)
            sys.stdout.write(f"\r[INPUT NODE] Active Clients: {active_cnt} | TX -> {packet.strip()}   ")
            sys.stdout.flush()
            time.sleep(0.1)

def run_server(port=9001, mode="fluctuating"):
    sim = InputNodeSimulator(mode=mode)
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("0.0.0.0", port))
    server.listen(10)

    print(f"[INPUT NODE SIM] Multi-Client Streamer started on 0.0.0.0:{port}")
    print(f"[INPUT NODE SIM] Ready for QNX Master and Backend connections.")

    # Start broadcast thread
    t = threading.Thread(target=sim.broadcast_loop, daemon=True)
    t.start()

    # Background auto-connector to QNX VM (192.168.160.129:9001)
    def qnx_auto_connector():
        while sim.running:
            try:
                # Check if already connected to QNX IP
                has_qnx = False
                with sim.lock:
                    for c in sim.clients:
                        try:
                            peer = c.getpeername()
                            if peer[0] == "192.168.160.129":
                                has_qnx = True
                                break
                        except Exception:
                            pass
                if not has_qnx:
                    qs = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    qs.settimeout(1.0)
                    qs.connect(("192.168.160.129", port))
                    qs.setblocking(True)
                    with sim.lock:
                        sim.clients.append(qs)
                    print(f"\n[INPUT NODE SIM] Auto-connected stream directly to QNX VM (192.168.160.129:{port})")
            except Exception:
                pass
            time.sleep(2.0)

    qt = threading.Thread(target=qnx_auto_connector, daemon=True)
    qt.start()

    try:
        while sim.running:
            client, addr = server.accept()
            client.setblocking(True)
            with sim.lock:
                sim.clients.append(client)
            print(f"\n[INPUT NODE SIM] New client connected from {addr} (Total: {len(sim.clients)})")
    except KeyboardInterrupt:
        sim.running = False
    finally:
        server.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Simulated ESP32-C3 Input Node")
    parser.add_argument("--port", type=int, default=9001)
    parser.add_argument("--mode", type=str, default="fluctuating")
    args = parser.parse_args()

    run_server(port=args.port, mode=args.mode)
