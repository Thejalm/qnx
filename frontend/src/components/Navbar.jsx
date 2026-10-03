import React, { useState, useEffect } from 'react';
import { Cpu, LayoutDashboard, GitCompare, ShieldCheck, ShieldAlert, Clock, Wifi, WifiOff, Shield } from 'lucide-react';

export default function Navbar({ activeTab, setActiveTab, systemStatus, wsConnected }) {
  const [time, setTime] = useState(new Date().toLocaleTimeString());

  useEffect(() => {
    const timer = setInterval(() => setTime(new Date().toLocaleTimeString()), 1000);
    return () => clearInterval(timer);
  }, []);

  const state = systemStatus?.qnx_state || 'NORMAL';
  const isAlert = state !== 'NORMAL';

  return (
    <nav
      className="mc-card"
      style={{
        flexDirection: 'row',
        justifyContent: 'space-between',
        alignItems: 'center',
        flexWrap: 'wrap',
        gap: 'var(--space-4)',
        padding: 'var(--space-3) var(--space-5)'
      }}
    >
      {/* Brand & Identity */}
      <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-3)' }}>
        <div
          style={{
            backgroundColor: 'var(--color-2)',
            color: 'var(--color-6)',
            width: '42px',
            height: '42px',
            borderRadius: 'var(--radius-sm)',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center'
          }}
        >
          <Cpu size={22} strokeWidth={2.2} />
        </div>
        <div>
          <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-2)' }}>
            <span style={{ fontSize: 'var(--text-lg)', lineHeight: 'var(--lh-lg)', fontWeight: 'var(--font-weight-bold)' }}>
              QNX Safety Orchestrator
            </span>
            <span className="mc-badge mc-badge-subtle" style={{ fontSize: '11px', padding: '2px 8px' }}>
              RTOS v8.0
            </span>
          </div>
          <p className="mc-section-subtitle">
            Deterministic industrial safety &amp; adaptive resource management
          </p>
        </div>
      </div>

      {/* Main Tab Navigation Buttons */}
      <div style={{ display: 'flex', gap: 'var(--space-2)', background: 'var(--color-4)', padding: 'var(--space-1)', borderRadius: 'var(--radius-xl)', flexWrap: 'wrap' }}>
        <button
          onClick={() => setActiveTab('dashboard')}
          className={`mc-btn ${activeTab === 'dashboard' ? 'mc-btn-primary' : 'mc-btn-secondary'}`}
          style={{
            border: 'none',
            fontSize: 'var(--text-xs)'
          }}
        >
          <LayoutDashboard size={15} />
          <span>Live Telemetry</span>
        </button>

        <button
          onClick={() => setActiveTab('guardian')}
          className={`mc-btn ${activeTab === 'guardian' ? 'mc-btn-primary' : 'mc-btn-secondary'}`}
          style={{
            border: 'none',
            fontSize: 'var(--text-xs)'
          }}
        >
          <Shield size={15} />
          <span>Safety Budget &amp; Guardian</span>
        </button>

        <button
          onClick={() => setActiveTab('compare')}
          className={`mc-btn ${activeTab === 'compare' ? 'mc-btn-primary' : 'mc-btn-secondary'}`}
          style={{
            border: 'none',
            fontSize: 'var(--text-xs)'
          }}
        >
          <GitCompare size={15} />
          <span>Compare Latency</span>
        </button>
      </div>

      {/* Live State & Stream Badges */}
      <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-3)', flexWrap: 'wrap' }}>
        {/* Core State Pill */}
        <div className={`mc-badge ${isAlert ? 'mc-badge-accent' : 'mc-badge-dark'}`}>
          {isAlert ? <ShieldAlert size={14} /> : <ShieldCheck size={14} />}
          <span>{state.replace(/_/g, ' ')}</span>
        </div>

        {/* Telemetry Cadence Link */}
        <div className="mc-badge mc-badge-subtle">
          {wsConnected ? <Wifi size={14} color="var(--color-2)" /> : <WifiOff size={14} color="var(--text-secondary)" />}
          <span className="mc-mono">{wsConnected ? '10 Hz Live' : 'Syncing'}</span>
        </div>

        {/* Clock Pill */}
        <div className="mc-badge mc-badge-subtle">
          <Clock size={14} color="var(--color-1)" />
          <span className="mc-mono">{time}</span>
        </div>
      </div>
    </nav>
  );
}
