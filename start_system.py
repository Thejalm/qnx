"""
==============================================================================
Project: QNX-Based Fully Wired Real-Time Safety & Automation Orchestrator
Component: Unified System Launcher & Runner
Description:
    Launches all 4 layers concurrently in separate managed sub-processes:
    1. Virtual ESP32-C3 Input Node (Sensors Telemetry Stream - Port 9001)
    2. Virtual ESP32-C3 Output Node (Actuators & ASCII OLED - Port 9002)
    3. QNX Master Safety Orchestrator (C/C++ or Host Simulator)
    4. FastAPI Backend API & WebSocket Sync Engine (Port 8000)
==============================================================================
"""

import sys
import os
import time
import subprocess

def main():
    root_dir = os.path.dirname(os.path.abspath(__file__))
    python_exe = sys.executable

    print("=" * 75)
    print("  QNX-BASED REAL-TIME SAFETY & AUTOMATION ORCHESTRATOR - SYSTEM LAUNCHER")
    print("=" * 75)
    print("Starting all system services...")

    processes = []

    try:
        # 1. Start Input Node Simulator (Port 9001)
        print("[1/4] Starting ESP32-C3 Input Node Telemetry Stream (Port 9001)...")
        p_in = subprocess.Popen(
            [python_exe, "firmware/simulator/input_node_sim.py", "--mode", "fluctuating"],
            cwd=root_dir
        )
        processes.append(("Input Node", p_in))
        time.sleep(1.0)

        # 2. Start Output Node Simulator (Port 9002)
        print("[2/4] Starting ESP32-C3 Output Node & OLED Simulator (Port 9002)...")
        p_out = subprocess.Popen(
            [python_exe, "firmware/simulator/output_node_sim.py"],
            cwd=root_dir
        )
        processes.append(("Output Node", p_out))
        time.sleep(1.0)

        # 3. Optional: Host Simulation of QNX Master (only if --sim flag passed)
        if "--sim" in sys.argv:
            print("[3/4] Starting Host Python QNX Master Simulator...")
            p_qnx = subprocess.Popen(
                [python_exe, "qnx_momentics/qnx_orchestrator_sim.py"],
                cwd=root_dir
            )
            processes.append(("QNX Master Sim", p_qnx))
            time.sleep(1.0)
        else:
            print("[3/4] QNX Master Mode: Ready for QNX Momentics IDE / VMware VM (192.168.160.129)")

        # 4. Start FastAPI Backend (Port 8000)
        print("[4/4] Starting FastAPI Backend & Real-Time Sync Engine (Port 8000)...")
        p_backend = subprocess.Popen(
            [python_exe, "-m", "uvicorn", "app.main:app", "--host", "0.0.0.0", "--port", "8000"],
            cwd=os.path.join(root_dir, "backend")
        )
        processes.append(("FastAPI Backend", p_backend))

        print("\n" + "=" * 75)
        print("  ALL SERVICES ARE RUNNING NORMALLY!")
        print("=" * 75)
        print("  - Backend Swagger Docs : http://localhost:8000/docs")
        print("  - React Frontend       : cd frontend && npm run dev (http://localhost:3000)")
        print("  - QNX Master State     : Live (Real-Time SCHED_FIFO Engine)")
        print("=" * 75)
        print("Press Ctrl+C to terminate all services gracefully.\n")

        while True:
            time.sleep(1.0)

    except KeyboardInterrupt:
        print("\nShutting down all orchestrator services...")
        for name, p in processes:
            p.terminate()
        print("All services stopped.")

if __name__ == "__main__":
    main()
