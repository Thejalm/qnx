# Windows Safety Computation & Latency Benchmark

This directory contains the standalone Windows-native evaluation engine used for side-by-side real-time latency comparisons against BlackBerry QNX Neutrino RTOS.

## Contents

- **`windows_safety_evaluator.py`**: Standalone Python script that connects to the live ESP32-C3 Input Node (Port 9001), executes safety-critical state machine logic, and measures execution latency using high-resolution performance counters (`time.perf_counter_ns()`).
- **`run_windows_evaluator.ps1`**: PowerShell runner script.

## Usage

From PowerShell or Command Prompt:

```powershell
cd windows
python windows_safety_evaluator.py
```

Or run the PowerShell script directly:

```powershell
.\windows\run_windows_evaluator.ps1
```
