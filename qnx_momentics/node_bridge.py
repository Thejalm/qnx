"""
==============================================================================
Project: QNX-Based Fully Wired Real-Time Safety & Automation Orchestrator
Component: Node Computer Serial-to-Ethernet Gateway Bridge
Description:
    Runs on the intermediate Node Computer connected to physical USB-Serial
    ESP32 boards. Bridges incoming sensor frames over Ethernet/LAN to the
    QNX Master, and routes QNX actuator commands back to the Output Node.
==============================================================================
"""

import sys
import time
import socket
import argparse
import threading

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

def serial_to_socket_worker(com_port: str, baud_rate: int, tcp_port: int):
    """Bridges physical USB-Serial COM Port to TCP Socket."""
    if not HAS_SERIAL:
        print("[NODE BRIDGE ERROR] pyserial is not installed! Run: pip install pyserial")
        return

    print(f"[NODE BRIDGE] Opening Serial Port {com_port} @ {baud_rate} baud...")
    try:
        ser = serial.Serial(com_port, baud_rate, timeout=0.1)
    except Exception as e:
        print(f"[NODE BRIDGE ERROR] Could not open {com_port}: {e}")
        return

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("0.0.0.0", tcp_port))
    server.listen(1)
    print(f"[NODE BRIDGE] Listening for QNX Master connection on TCP Port {tcp_port}...")

    while True:
        client, addr = server.accept()
        print(f"[NODE BRIDGE] QNX Master connected from {addr}")
        try:
            while True:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('ascii', errors='ignore')
                    if line:
                        client.sendall(line.encode('ascii'))
        except (ConnectionResetError, BrokenPipeError):
            print(f"[NODE BRIDGE] QNX Master disconnected.")
        finally:
            client.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Node Computer Serial-to-Ethernet Bridge")
    parser.add_argument("--input-com", type=str, default="COM3", help="Input Node USB COM Port")
    parser.add_argument("--output-com", type=str, default="COM4", help="Output Node USB COM Port")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate (default: 115200)")
    parser.add_argument("--in-port", type=int, default=9001, help="TCP Port for Input Telemetry (default: 9001)")
    parser.add_argument("--out-port", type=int, default=9002, help="TCP Port for Output Commands (default: 9002)")
    args = parser.parse_args()

    print("=" * 70)
    print("  NODE COMPUTER USB-SERIAL <-> ETHERNET LAN BRIDGE")
    print("=" * 70)
    print(f"Input Node COM: {args.input_com}  -> TCP Port: {args.in_port}")
    print(f"Output Node COM: {args.output_com} -> TCP Port: {args.out_port}")
    print("=" * 70)

    # For running with physical hardware:
    # t1 = threading.Thread(target=serial_to_socket_worker, args=(args.input_com, args.baud, args.in_port))
    # t1.start()
    # t1.join()
