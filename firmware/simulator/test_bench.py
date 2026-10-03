"""
==============================================================================
Project: QNX-Based Fully Wired Real-Time Safety & Automation Orchestrator
Component: Interactive Hardware Simulator Test Bench
Description:
    Provides an interactive command-line bench to verify both Input and Output
    simulators before proceeding to the QNX Orchestrator.
==============================================================================
"""

import sys
import time
import socket

def calculate_xor_checksum(payload: str) -> str:
    checksum = 0
    for ch in payload:
        checksum ^= ord(ch)
    return f"{checksum:02X}"

def main():
    print("=" * 70)
    print("  QNX WIRED SAFETY ORCHESTRATOR - HARDWARE SIMULATOR TEST BENCH")
    print("=" * 70)
    print("Options:")
    print("  1. Monitor Input Node Telemetry Stream (Port 9001)")
    print("  2. Send Normal Command to Output Node (Port 9002)")
    print("  3. Send Gas Warning Command (Fan ON, Yellow LED, Buzzer 2)")
    print("  4. Send Fire Critical Command (Fan ON, Pump ON, Red LED, Buzzer 1)")
    print("  5. Send Custom Command")
    print("  6. Exit")
    print("=" * 70)

    seq = 1

    while True:
        choice = input("\nSelect Option (1-6): ").strip()

        if choice == "1":
            print("\nConnecting to Input Node Simulator (127.0.0.1:9001)...")
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.connect(("127.0.0.1", 9001))
                print("Connected! Streaming 10 Hz Telemetry (Press Ctrl+C to stop)...\n")
                buffer = ""
                while True:
                    data = s.recv(1024).decode('ascii')
                    if not data:
                        break
                    buffer += data
                    while "\n" in buffer:
                        line, buffer = buffer.split("\n", 1)
                        if line.strip():
                            print(f"[RX Telemetry] {line.strip()}")
            except ConnectionRefusedError:
                print("Error: Input Node simulator is not running on port 9001.")
                print("Start it with: python firmware/simulator/input_node_sim.py")
            except KeyboardInterrupt:
                print("\nStopped monitoring.")

        elif choice in ["2", "3", "4", "5"]:
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.connect(("127.0.0.1", 9002))

                if choice == "2":
                    payload = f"CMD,2,{seq},0,0,0,0,0,0,ALL_NORMAL"
                elif choice == "3":
                    payload = f"CMD,2,{seq},0,1,1,0,1,0,GAS_WARNING"
                elif choice == "4":
                    payload = f"CMD,2,{seq},1,0,1,1,0,1,FIRE_ALARM"
                elif choice == "5":
                    raw = input("Enter payload (e.g. CMD,2,1,0,0,1,1,0,1,MANUAL_TEST): ").strip()
                    payload = raw if not raw.startswith("$") else raw[1:].split("*")[0]

                chk = calculate_xor_checksum(payload)
                packet = f"${payload}*{chk}\r\n"

                print(f"[TX -> Output Node] {packet.strip()}")
                s.sendall(packet.encode('ascii'))
                ack = s.recv(1024).decode('ascii')
                print(f"[RX <- ACK Reply  ] {ack.strip()}")
                s.close()
                seq += 1

            except ConnectionRefusedError:
                print("Error: Output Node simulator is not running on port 9002.")
                print("Start it with: python firmware/simulator/output_node_sim.py")

        elif choice == "6":
            print("Exiting test bench.")
            break

if __name__ == "__main__":
    main()
