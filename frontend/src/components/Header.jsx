import React, { useState, useEffect } from 'react';
import { ShieldCheck, ShieldAlert, Cpu, Clock, Wifi, WifiOff } from 'lucide-react';

export default function Header({ systemStatus, wsConnected }) {
  const [time, setTime] = useState(new Date().toLocaleTimeString());

  useEffect(() => {
    const timer = setInterval(() => setTime(new Date().toLocaleTimeString()), 1000);
    return () => clearInterval(timer);
  }, []);

  const state = systemStatus?.qnx_state || 'NORMAL';
  const isAlert = state !== 'NORMAL';

  return (
    <header
      className="mc-card"
      style={{
        flexDirection: 'row',
        justifyContent: 'space-between',
        alignItems: 'center',
        flexWrap: 'wrap',
        gap: 'var(--space-4)',
        padding: 'var(--space-4) var(--space-5)'
      }}
    >
      {/* Brand & System Identification */}
      <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-4)' }}>
        <div
          style={{
            backgroundColor: 'var(--color-2)',
            color: 'var(--color-6)',
            width: '48px',
            height: '48px',
            borderRadius: 'var(--radius-sm)',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center'
          }}
        >
          <Cpu size={26} strokeWidth={2.2} />
        </div>
        <div>
          <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-2)' }}>
            <h1 style={{ fontSize: 'var(--text-xl)', lineHeight: 'var(--lh-xl)', fontWeight: 'var(--font-weight-bold)' }}>
              QNX Safety Orchestrator
            </h1>
            <span className="mc-badge mc-badge-subtle">
              RTOS v8.0
            </span>
          </div>
          <p className="mc-section-subtitle">
            Real-time deterministic safety and automation pipeline
          </p>
        </div>
      </div>

      {/* System Status Indicators */}
      <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-4)', flexWrap: 'wrap' }}>
        {/* Core State Pill */}
        <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'flex-start', gap: 'var(--space-1)' }}>
          <span className="mc-stat-label">
            QNX Core State
          </span>
          <div className={`mc-badge ${isAlert ? 'mc-badge-accent' : 'mc-badge-dark'}`}>
            {isAlert ? <ShieldAlert size={14} /> : <ShieldCheck size={14} />}
            <span>{state.replace(/_/g, ' ')}</span>
          </div>
        </div>

        {/* Telemetry Cadence Link */}
        <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'flex-start', gap: 'var(--space-1)' }}>
          <span className="mc-stat-label">
            Telemetry Stream
          </span>
          <div className="mc-badge mc-badge-subtle">
            {wsConnected ? <Wifi size={14} color="var(--color-2)" /> : <WifiOff size={14} color="var(--text-secondary)" />}
            <span className="mc-mono">{wsConnected ? '10 Hz Live' : 'Syncing'}</span>
          </div>
        </div>

        {/* Clock Pill */}
        <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'flex-start', gap: 'var(--space-1)' }}>
          <span className="mc-stat-label">
            System Clock
          </span>
          <div className="mc-badge mc-badge-subtle">
            <Clock size={14} color="var(--color-1)" />
            <span className="mc-mono">{time}</span>
          </div>
        </div>
      </div>
    </header>
  );
}
