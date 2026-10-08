"""
==============================================================================
Project: QNX-Based Fully Wired Real-Time Safety & Automation Orchestrator
Component: Physical Hardware Serial-to-Ethernet Gateway Bridge
Description:
    Bridges physical USB-Serial ESP32 hardware to the real-time network:
    - Input Mode  : Reads $IN sensor frames from ESP32 Input Node and streams
                    over TCP port 9001 (Node 1 Laptop: 10.61.30.220).
    - Output Mode : Receives $CMD packets from QNX Master over TCP port 9002,
                    forwards to physical ESP32 Output Node, and routes $ACK back
                    (Master Laptop: 10.61.30.60).
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

# ============================================================================
# INPUT NODE SERIAL BRIDGE (Sensors Acquisition -> TCP 9001)
# ============================================================================
class InputNodeBridge:
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
                print(f"[INPUT BRIDGE] Attempting to open serial {self.com_port} @ {self.baud_rate}...")
                self.ser = serial.Serial(self.com_port, self.baud_rate, timeout=0.2)
                print(f"[INPUT BRIDGE] Successfully opened {self.com_port} (ESP32-C3 Node 1)")
                return True
            except Exception as e:
                print(f"[INPUT BRIDGE WARN] Could not open {self.com_port} ({e}). Retrying in 2s...")
                time.sleep(2.0)
        return False

    def auto_connect_master(self):
        """Proactively connects to QNX Master if it runs an inbound TCP server."""
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
                    print(f"\n[INPUT BRIDGE] Outbound link connected directly to Master ({self.target_master_ip}:{self.tcp_port})")
            except Exception:
                pass
            time.sleep(3.0)

    def listen_server(self):
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind(("0.0.0.0", self.tcp_port))
        server.listen(5)
        print(f"[INPUT BRIDGE] Telemetry Server listening on 0.0.0.0:{self.tcp_port}")

        while self.running:
            try:
                client, addr = server.accept()
                client.setblocking(True)
                with self.lock:
                    self.clients.append(client)
                
                client_ip = addr[0]
                if client_ip == "127.0.0.1" or client_ip == "localhost":
                    print(f"\n[INPUT BRIDGE] Local Web Dashboard connected from {addr} (Total: {len(self.clients)})")
                elif client_ip == self.target_master_ip:
                    print(f"\n[INPUT BRIDGE] *** QNX MASTER (10.61.30.60) CONNECTED from {addr} *** (Total: {len(self.clients)})")
                else:
                    print(f"\n[INPUT BRIDGE] External client connected from {addr} (Total: {len(self.clients)})")
            except Exception:
                break
        server.close()

    def run(self):
        if not HAS_SERIAL:
            print("[INPUT BRIDGE ERROR] pyserial is not installed! Run: pip install pyserial")
            return

        t_server = threading.Thread(target=self.listen_server, daemon=True)
        t_server.start()

        t_master = threading.Thread(target=self.auto_connect_master, daemon=True)
        t_master.start()

        frame_count = 0
        while self.running:
            if self.ser is None or not self.ser.is_open:
                if not self.open_serial():
                    break

            try:
                line = self.ser.readline().decode('ascii', errors='ignore').strip()
                if line and (line.startswith('$IN') or line.startswith('$TEL')):
                    data = (line + "\r\n").encode('ascii')
                    frame_count += 1
                    if frame_count % 50 == 0:
                        sys.stdout.write(f"\r[INPUT BRIDGE] Streamed {frame_count} frames | Latest: {line[:55]}... ")
                        sys.stdout.flush()

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
                print(f"\n[INPUT BRIDGE] Serial read error: {e}")
                if self.ser:
                    try:
                        self.ser.close()
                    except Exception:
                        pass
                self.ser = None
                time.sleep(1.0)

# ============================================================================
# OUTPUT NODE SERIAL BRIDGE (TCP 9002 -> Actuators & OLED Hardware)
# ============================================================================
class OutputNodeBridge:
    def __init__(self, com_port="COM4", baud_rate=115200, tcp_port=9002):
        self.com_port = com_port
        self.baud_rate = baud_rate
        self.tcp_port = tcp_port
        self.running = True
        self.ser = None
        self.ser_lock = threading.Lock()
        self.clients = []
        self.client_lock = threading.Lock()
        self.cmd_count = 0

    def open_serial(self):
        while self.running:
            try:
                print(f"[OUTPUT BRIDGE] Attempting to open serial {self.com_port} @ {self.baud_rate}...")
                self.ser = serial.Serial(
                    self.com_port,
                    self.baud_rate,
                    timeout=0.1,
                    write_timeout=0,
                    dsrdtr=False,
                    rtscts=False,
                    xonxoff=False
                )
                try:
                    self.ser.dtr = True
                    self.ser.rts = False
                except Exception:
                    pass
                print(f"[OUTPUT BRIDGE] Successfully connected to {self.com_port} (ESP32-C3 Output Node)")
                return True
            except Exception as e:
                print(f"[OUTPUT BRIDGE WARN] Could not open {self.com_port} ({e}). Retrying in 2s...")
                time.sleep(2.0)
        return False

    def serial_reader_worker(self):
        """Continuously reads $ACK responses and status lines from ESP32 and routes to QNX Master."""
        ack_count = 0
        while self.running:
            if self.ser and self.ser.is_open:
                try:
                    line = self.ser.readline().decode('ascii', errors='ignore').strip()
                    if line:
                        print(f"[ESP32 RX] {line}", flush=True)
                        if line.startswith('$ACK'):
                            ack_count += 1
                            data = (line + "\r\n").encode('ascii')
                            with self.client_lock:
                                dead = []
                                for c in self.clients:
                                    try:
                                        c.sendall(data)
                                    except Exception:
                                        dead.append(c)
                                for d in dead:
                                    try:
                                        d.close()
                                    except Exception:
                                        pass
                                    if d in self.clients:
                                        self.clients.remove(d)
                except Exception as e:
                    print(f"[OUTPUT BRIDGE SERIAL ERR] {e}", flush=True)
                    time.sleep(0.5)
            else:
                time.sleep(0.5)

    def handle_client(self, client, addr):
        print(f"\n[OUTPUT BRIDGE] Master connected from {addr}", flush=True)
        with self.client_lock:
            self.clients.append(client)
        buffer = ""
        while self.running:
            try:
                data = client.recv(256).decode('ascii', errors='ignore')
                if not data:
                    print(f"[OUTPUT BRIDGE] Client {addr} sent EOF/closed", flush=True)
                    break
                buffer += data
                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    line = line.strip()
                    if line.startswith('$CMD'):
                        if self.ser and self.ser.is_open:
                            with self.ser_lock:
                                try:
                                    self.ser.write((line + "\r\n").encode('ascii'))
                                except Exception as w_err:
                                    print(f"[OUTPUT BRIDGE WRITE ERR] {w_err}", flush=True)
                            self.cmd_count += 1
                            if self.cmd_count % 10 == 0 or self.cmd_count <= 5:
                                print(f"[OUTPUT BRIDGE] Forwarded #{self.cmd_count} to ESP32: {line[:55]}...", flush=True)
            except (ConnectionResetError, BrokenPipeError):
                break
            except Exception as e:
                print(f"[OUTPUT BRIDGE SOCKET ERR] {e}", flush=True)
                time.sleep(0.01)

        print(f"\n[OUTPUT BRIDGE] QNX Master disconnected from {addr}")
        with self.client_lock:
            if client in self.clients:
                self.clients.remove(client)
        try:
            client.close()
        except Exception:
            pass

    def run(self):
        if not HAS_SERIAL:
            print("[OUTPUT BRIDGE ERROR] pyserial is not installed! Run: pip install pyserial")
            return

        if not self.open_serial():
            return

        t_ser = threading.Thread(target=self.serial_reader_worker, daemon=True)
        t_ser.start()

        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind(("0.0.0.0", self.tcp_port))
        server.listen(5)
        print(f"[OUTPUT BRIDGE] Actuator Command Server listening on 0.0.0.0:{self.tcp_port}")

        while self.running:
            try:
                client, addr = server.accept()
                t_client = threading.Thread(target=self.handle_client, args=(client, addr), daemon=True)
                t_client.start()
            except Exception:
                break
        server.close()

# ============================================================================
# MAIN ENTRYPOINT
# ============================================================================
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Physical ESP32 Serial-to-Ethernet Bridge")
    parser.add_argument("--role", type=str, choices=["input", "output"], default="input",
                        help="Role of this bridge: 'input' (Sensor Node) or 'output' (Actuator Node)")
    parser.add_argument("--input-com", type=str, default="COM9", help="Input Node USB COM Port (default: COM9)")
    parser.add_argument("--output-com", type=str, default="COM4", help="Output Node USB COM Port (default: COM4)")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate (default: 115200)")
    parser.add_argument("--in-port", type=int, default=9001, help="TCP Port for Input Telemetry (default: 9001)")
    parser.add_argument("--out-port", type=int, default=9002, help="TCP Port for Output Commands (default: 9002)")
    parser.add_argument("--master-ip", type=str, default="10.61.30.60", help="Target Master IP (default: 10.61.30.60)")
    args = parser.parse_args()

    print("=" * 70)
    print("  PHYSICAL HARDWARE USB-SERIAL <-> ETHERNET LAN BRIDGE (NO SIMULATION)")
    print("=" * 70)

    if args.role == "input":
        print(f"Role            : INPUT NODE (Sensors Acquisition)")
        print(f"USB COM Port    : {args.input_com} @ {args.baud} baud")
        print(f"TCP Stream Port : {args.in_port}")
        print(f"Target Master IP: {args.master_ip}")
        print("=" * 70)
        bridge = InputNodeBridge(com_port=args.input_com, baud_rate=args.baud,
                                 tcp_port=args.in_port, target_master_ip=args.master_ip)
    else:
        print(f"Role            : OUTPUT NODE (Actuators, Relays & OLED)")
        print(f"USB COM Port    : {args.output_com} @ {args.baud} baud")
        print(f"TCP Command Port: {args.out_port}")
        print("=" * 70)
        bridge = OutputNodeBridge(com_port=args.output_com, baud_rate=args.baud, tcp_port=args.out_port)

    try:
        bridge.run()
    except KeyboardInterrupt:
        print("\n[HARDWARE BRIDGE] Shutting down.")
        bridge.running = False
