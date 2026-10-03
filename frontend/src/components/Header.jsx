import React, { useState, useEffect } from 'react';
import { ShieldCheck, ShieldAlert, Cpu, Activity, Clock, Wifi, WifiOff } from 'lucide-react';

export default function Header({ systemStatus, wsConnected }) {
  const [time, setTime] = useState(new Date().toLocaleTimeString());

  useEffect(() => {
    const timer = setInterval(() => setTime(new Date().toLocaleTimeString()), 1000);
    return () => clearInterval(timer);
  }, []);

  const state = systemStatus?.qnx_state || 'NORMAL';

  const getBadgeClass = (s) => {
    switch (s) {
      case 'NORMAL': return 'badge-normal';
      case 'GAS_WARNING':
      case 'ELEVATED_TEMP': return 'badge-warning';
      case 'FIRE_CRITICAL': return 'badge-critical';
      case 'SENSOR_FAULT':
      case 'COMM_FAULT': return 'badge-fault';
      default: return 'badge-offline';
    }
  };

  return (
    <header className="glass-card" style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', flexWrap: 'wrap', gap: '1rem' }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: '1rem' }}>
        <div style={{
          background: 'linear-gradient(135deg, #0284c7, #6366f1)',
          padding: '0.75rem',
          borderRadius: '12px',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          boxShadow: '0 0 20px rgba(14, 165, 233, 0.4)'
        }}>
          <Cpu size={28} color="#ffffff" />
        </div>
        <div>
          <div style={{ display: 'flex', alignItems: 'center', gap: '0.6rem' }}>
            <h1 style={{ fontSize: '1.4rem', fontWeight: 800, letterSpacing: '-0.02em' }}>
              QNX SAFETY ORCHESTRATOR
            </h1>
            <span style={{ fontSize: '0.75rem', padding: '0.15rem 0.5rem', borderRadius: '4px', background: 'rgba(56, 189, 248, 0.1)', color: '#38bdf8', border: '1px solid rgba(56, 189, 248, 0.2)' }}>
              WIRED RTOS v8.0
            </span>
          </div>
          <p style={{ fontSize: '0.85rem', color: 'var(--text-secondary)' }}>
            Real-Time Deterministic Industrial Safety & Automation Pipeline
          </p>
        </div>
      </div>

      <div style={{ display: 'flex', alignItems: 'center', gap: '1.25rem' }}>
        {/* QNX Orchestrator State Badge */}
        <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'flex-end', gap: '0.2rem' }}>
          <span style={{ fontSize: '0.7rem', color: 'var(--text-muted)', textTransform: 'uppercase', letterSpacing: '0.05em' }}>
            QNX Core State
          </span>
          <div className={`status-badge ${getBadgeClass(state)}`}>
            {state === 'NORMAL' ? <ShieldCheck size={16} /> : <ShieldAlert size={16} />}
            <span>{state.replace('_', ' ')}</span>
          </div>
        </div>

        {/* Live Link / WS Indicator */}
        <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'flex-end', gap: '0.2rem' }}>
          <span style={{ fontSize: '0.7rem', color: 'var(--text-muted)', textTransform: 'uppercase', letterSpacing: '0.05em' }}>
            Telemetry Stream
          </span>
          <div style={{
            display: 'flex',
            alignItems: 'center',
            gap: '0.4rem',
            fontSize: '0.85rem',
            fontFamily: 'var(--font-mono)',
            color: wsConnected ? '#34d399' : '#fb7185'
          }}>
            {wsConnected ? <Wifi size={16} /> : <WifiOff size={16} />}
            <span>{wsConnected ? '10 Hz LIVE' : 'SYNCING'}</span>
          </div>
        </div>

        {/* Real-Time Clock */}
        <div style={{
          display: 'flex',
          alignItems: 'center',
          gap: '0.4rem',
          padding: '0.4rem 0.8rem',
          borderRadius: '8px',
          background: 'rgba(255,255,255,0.03)',
          border: '1px solid rgba(255,255,255,0.08)',
          fontFamily: 'var(--font-mono)',
          fontSize: '0.9rem',
          color: 'var(--text-secondary)'
        }}>
          <Clock size={16} color="#38bdf8" />
          <span>{time}</span>
        </div>
      </div>
    </header>
  );
}
