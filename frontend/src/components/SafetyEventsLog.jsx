import React, { useState } from 'react';
import { ScrollText, ShieldAlert, AlertTriangle, Info, Wrench } from 'lucide-react';

export default function SafetyEventsLog({ events }) {
  const [filter, setFilter] = useState('ALL');

  const filteredEvents = events.filter(e => {
    if (filter === 'ALL') return true;
    return e.level === filter;
  });

  const getLevelStyle = (lvl) => {
    switch (lvl) {
      case 'CRITICAL':
        return { color: '#fb7185', icon: <ShieldAlert size={16} /> };
      case 'WARN':
        return { color: '#fbbf24', icon: <AlertTriangle size={16} /> };
      case 'FAULT':
        return { color: '#c084fc', icon: <Wrench size={16} /> };
      default:
        return { color: '#38bdf8', icon: <Info size={16} /> };
    }
  };

  return (
    <div className="glass-card" style={{ display: 'flex', flexDirection: 'column', height: '100%' }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '1rem' }}>
        <div className="section-title" style={{ marginBottom: 0 }}>
          <ScrollText size={20} color="#f59e0b" />
          <span>QNX Real-Time Safety Event Log</span>
        </div>

        {/* Filter Buttons */}
        <div style={{ display: 'flex', gap: '0.4rem' }}>
          {['ALL', 'CRITICAL', 'WARN', 'FAULT', 'INFO'].map(lvl => (
            <button
              key={lvl}
              onClick={() => setFilter(lvl)}
              style={{
                background: filter === lvl ? 'rgba(56, 189, 248, 0.2)' : 'rgba(255,255,255,0.03)',
                color: filter === lvl ? '#38bdf8' : 'var(--text-muted)',
                border: filter === lvl ? '1px solid rgba(56, 189, 248, 0.4)' : '1px solid rgba(255,255,255,0.06)',
                borderRadius: '6px',
                padding: '0.25rem 0.6rem',
                fontSize: '0.75rem',
                fontWeight: 600,
                cursor: 'pointer',
                transition: 'all 0.15s ease'
              }}
            >
              {lvl}
            </button>
          ))}
        </div>
      </div>

      {/* Events List Container */}
      <div style={{
        flex: 1,
        maxHeight: '320px',
        overflowY: 'auto',
        display: 'flex',
        flexDirection: 'column',
        gap: '0.5rem',
        paddingRight: '0.4rem'
      }}>
        {filteredEvents.length === 0 ? (
          <div style={{ textAlign: 'center', color: 'var(--text-muted)', padding: '2rem', fontSize: '0.85rem' }}>
            No safety events recorded. System operating normally.
          </div>
        ) : (
          filteredEvents.map((evt, idx) => {
            const style = getLevelStyle(evt.level);
            return (
              <div
                key={evt.id || idx}
                style={{
                  background: 'rgba(255,255,255,0.02)',
                  border: `1px solid ${style.color}30`,
                  borderLeft: `4px solid ${style.color}`,
                  borderRadius: '6px',
                  padding: '0.65rem 0.85rem',
                  display: 'flex',
                  justifyContent: 'space-between',
                  alignItems: 'center',
                  gap: '0.8rem'
                }}
              >
                <div style={{ display: 'flex', alignItems: 'center', gap: '0.6rem' }}>
                  <span style={{ color: style.color }}>{style.icon}</span>
                  <div>
                    <div style={{ fontSize: '0.85rem', fontWeight: 600, color: '#f8fafc' }}>
                      {evt.description}
                    </div>
                    <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>
                      Source: {evt.source} | V1={evt.sensor_val1?.toFixed(1)} V2={evt.sensor_val2?.toFixed(1)}
                    </div>
                  </div>
                </div>

                <div style={{ textAlign: 'right', fontFamily: 'var(--font-mono)', fontSize: '0.75rem', color: 'var(--text-secondary)' }}>
                  <span style={{ color: style.color, fontWeight: 700 }}>[{evt.level}]</span>
                </div>
              </div>
            );
          })
        )}
      </div>
    </div>
  );
}
