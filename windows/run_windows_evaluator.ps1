# ==============================================================================
# PowerShell Launcher for Standalone Windows Safety Evaluator
# ==============================================================================
Write-Host "Starting Windows Python Safety Evaluator & Latency Benchmark..." -ForegroundColor Cyan
Set-Location $PSScriptRoot
python windows_safety_evaluator.py
