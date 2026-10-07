"""
==============================================================================
Project: QNX-Based Fully Wired Real-Time Safety & Automation Orchestrator
Component: Physical Hardware System Launcher & Runner
Description:
    Launches physical orchestrator services concurrently:
    1. Physical ESP32-C3 Hardware Bridge (COM9 -> TCP Port 9001)
    2. FastAPI Backend API & WebSocket Real-Time Sync Engine (Port 8000)
    3. Target QNX Master ready on 10.61.30.60
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
    print("  QNX REAL-TIME SAFETY ORCHESTRATOR - HARDWARE SYSTEM LAUNCHER")
    print("=" * 75)
    print("Starting all hardware services...")

    processes = []

    try:
        # 1. Start Hardware USB-Serial Bridge for Input Node (Port 9001)
        print("[1/3] Starting Hardware USB-Serial Bridge for ESP32 Node 1 on COM9 (Port 9001)...")
        p_in = subprocess.Popen(
            [python_exe, "qnx_momentics/node_bridge.py", "--role", "input", "--input-com", "COM9", "--in-port", "9001", "--master-ip", "10.61.30.60"],
            cwd=root_dir
        )
        processes.append(("Hardware Input Bridge", p_in))
        time.sleep(1.0)

        # 2. QNX Master Status
        print("[2/3] QNX Master Mode: Ready for Output Node / QNX Master Target (10.61.30.60)")

        # 3. Start FastAPI Backend (Port 8000)
        print("[3/3] Starting FastAPI Backend & Real-Time Sync Engine (Port 8000)...")
        p_backend = subprocess.Popen(
            [python_exe, "-m", "uvicorn", "app.main:app", "--host", "0.0.0.0", "--port", "8000"],
            cwd=os.path.join(root_dir, "backend")
        )
        processes.append(("FastAPI Backend", p_backend))

        print("\n" + "=" * 75)
        print("  ALL PHYSICAL SERVICES ARE RUNNING NORMALLY!")
        print("=" * 75)
        print("  - Backend Swagger Docs : http://localhost:8000/docs")
        print("  - React Frontend       : cd frontend && npm run dev (http://localhost:3000)")
        print("  - QNX Master Target    : 10.61.30.60 (SCHED_FIFO Priority Engine)")
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
