"""
==============================================================================
Project: QNX-Based Fully Wired Real-Time Safety & Automation Orchestrator
Component: Node Computer Serial-to-Ethernet Gateway Bridge
Description:
    Runs on the intermediate Node Computer (10.61.30.220) connected to physical
    USB-Serial ESP32 boards (COM9). Bridges incoming sensor frames over Ethernet
    to the QNX Master / Output Node (10.61.30.60), and serves local telemetry
    subscribers (Backend / Dashboard).
==============================================================================
"""

import sys
import time
import socket
import argparse
import threading

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(line_buffering=True)

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

class SerialBridge:
    def __init__(self, com_port="COM9", baud_rate=115200, tcp_port=9001, target_master_ip="10.61.30.60"):
        self.com_port = com_port
        self.baud_rate = baud_rate
        self.tcp_port = tcp_port
        self.target_master_ip = target_master_ip
        self.running = True
        self.clients = []
        self.lock = threading.Lock()
        self.ser = None

    def open_serial(self):
        while self.running:
            try:
                print(f"[NODE BRIDGE] Attempting to open serial {self.com_port} @ {self.baud_rate}...")
                self.ser = serial.Serial(self.com_port, self.baud_rate, timeout=0.2)
                print(f"[NODE BRIDGE] Successfully connected to {self.com_port}!")
                return True
            except Exception as e:
                print(f"[NODE BRIDGE WARN] Could not open {self.com_port} ({e}). Retrying in 2s...")
                time.sleep(2.0)
        return False

    def auto_connect_master(self):
        """Proactively connects to QNX Master if it runs a TCP server."""
        while self.running:
            try:
                has_master = False
                with self.lock:
                    for c in self.clients:
                        try:
                            if c.getpeername()[0] == self.target_master_ip:
                                has_master = True
                                break
                        except Exception:
                            pass
                if not has_master and self.target_master_ip:
                    ms = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    ms.settimeout(1.0)
                    ms.connect((self.target_master_ip, self.tcp_port))
                    ms.setblocking(True)
                    with self.lock:
                        self.clients.append(ms)
                    print(f"\n[NODE BRIDGE] Connected outbound stream directly to Master ({self.target_master_ip}:{self.tcp_port})")
            except Exception:
                pass
            time.sleep(3.0)

    def listen_server(self):
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind(("0.0.0.0", self.tcp_port))
        server.listen(5)
        print(f"[NODE BRIDGE] TCP Telemetry Server listening on 0.0.0.0:{self.tcp_port}")

        while self.running:
            try:
                client, addr = server.accept()
                client.setblocking(True)
                with self.lock:
                    self.clients.append(client)
                print(f"\n[NODE BRIDGE] Client connected from {addr} (Total: {len(self.clients)})")
            except Exception:
                break
        server.close()

    def run(self):
        if not HAS_SERIAL:
            print("[NODE BRIDGE ERROR] pyserial is not installed! Run: pip install pyserial")
            return

        # Start TCP listener thread
        t_server = threading.Thread(target=self.listen_server, daemon=True)
        t_server.start()

        # Start master auto-connector thread
        t_master = threading.Thread(target=self.auto_connect_master, daemon=True)
        t_master.start()

        while self.running:
            if self.ser is None or not self.ser.is_open:
                if not self.open_serial():
                    break

            try:
                line = self.ser.readline().decode('ascii', errors='ignore').strip()
                if line and (line.startswith('$IN') or line.startswith('$TEL')):
                    data = (line + "\r\n").encode('ascii')
                    with self.lock:
                        dead_clients = []
                        for c in self.clients:
                            try:
                                c.sendall(data)
                            except Exception:
                                dead_clients.append(c)
                        for dc in dead_clients:
                            try:
                                dc.close()
                            except Exception:
                                pass
                            self.clients.remove(dc)
            except Exception as e:
                print(f"[NODE BRIDGE] Serial read error: {e}")
                if self.ser:
                    try:
                        self.ser.close()
                    except Exception:
                        pass
                self.ser = None
                time.sleep(1.0)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Node Computer Serial-to-Ethernet Bridge")
    parser.add_argument("--input-com", type=str, default="COM9", help="Input Node USB COM Port (default: COM9)")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate (default: 115200)")
    parser.add_argument("--in-port", type=int, default=9001, help="TCP Port for Input Telemetry (default: 9001)")
    parser.add_argument("--master-ip", type=str, default="10.61.30.60", help="Target Master IP (default: 10.61.30.60)")
    args = parser.parse_args()

    print("=" * 70)
    print("  NODE COMPUTER USB-SERIAL <-> ETHERNET LAN BRIDGE")
    print("=" * 70)
    print(f"Local Node 1 IP : 10.61.30.220")
    print(f"Input Node COM  : {args.input_com} @ {args.baud} baud")
    print(f"TCP Stream Port : {args.in_port}")
    print(f"Target Master IP: {args.master_ip}")
    print("=" * 70)

    bridge = SerialBridge(com_port=args.input_com, baud_rate=args.baud, tcp_port=args.in_port, target_master_ip=args.master_ip)
    try:
        bridge.run()
    except KeyboardInterrupt:
        print("\n[NODE BRIDGE] Shutting down.")
        bridge.running = False
