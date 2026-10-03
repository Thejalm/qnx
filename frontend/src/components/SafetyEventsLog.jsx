import React, { useState } from 'react';
import { ScrollText, ShieldAlert, AlertTriangle, Info, Wrench } from 'lucide-react';

export default function SafetyEventsLog({ events }) {
  const [filter, setFilter] = useState('ALL');

  const filteredEvents = events.filter(e => {
    if (filter === 'ALL') return true;
    return e.level === filter;
  });

  const getLevelBadge = (lvl) => {
    switch (lvl) {
      case 'CRITICAL':
        return { badgeClass: 'mc-badge-accent', icon: <ShieldAlert size={14} /> };
      case 'WARN':
        return { badgeClass: 'mc-badge-outline', icon: <AlertTriangle size={14} /> };
      case 'FAULT':
        return { badgeClass: 'mc-badge-dark', icon: <Wrench size={14} /> };
      default:
        return { badgeClass: 'mc-badge-subtle', icon: <Info size={14} /> };
    }
  };

  return (
    <section className="mc-card" style={{ height: '100%' }}>
      <div className="mc-section-header" style={{ flexWrap: 'wrap' }}>
        <div className="mc-section-title">
          <ScrollText size={20} color="var(--color-2)" />
          <span>Safety event audit log</span>
        </div>

        {/* Filter Pill Buttons with interactive states */}
        <div style={{ display: 'flex', gap: 'var(--space-1)', flexWrap: 'wrap' }}>
          {['ALL', 'CRITICAL', 'WARN', 'FAULT', 'INFO'].map(lvl => (
            <button
              key={lvl}
              onClick={() => setFilter(lvl)}
              className={`mc-btn mc-btn-secondary ${filter === lvl ? 'is-active' : ''}`}
            >
              {lvl}
            </button>
          ))}
        </div>
      </div>

      {/* Events List */}
      <div
        style={{
          flex: 1,
          maxHeight: '340px',
          overflowY: 'auto',
          display: 'flex',
          flexDirection: 'column',
          gap: 'var(--space-2)',
          paddingRight: 'var(--space-1)'
        }}
      >
        {filteredEvents.length === 0 ? (
          <div className="mc-card-nested" style={{ textAlign: 'left', color: 'var(--text-secondary)', padding: 'var(--space-5)' }}>
            No safety events recorded. System operating normally.
          </div>
        ) : (
          filteredEvents.map((evt, idx) => {
            const { badgeClass, icon } = getLevelBadge(evt.level);
            return (
              <div
                key={evt.id || idx}
                className="mc-card-nested"
                style={{
                  display: 'flex',
                  justifyContent: 'space-between',
                  alignItems: 'center',
                  padding: 'var(--space-3) var(--space-4)',
                  gap: 'var(--space-3)'
                }}
              >
                <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-3)' }}>
                  <span className={`mc-badge ${badgeClass}`}>
                    {icon}
                    <span>{evt.level}</span>
                  </span>
                  <div>
                    <div style={{ fontSize: 'var(--text-sm)', fontWeight: 'var(--font-weight-bold)' }}>
                      {evt.description}
                    </div>
                    <div className="mc-mono" style={{ fontSize: 'var(--text-xs)', color: 'var(--text-secondary)' }}>
                      source: {evt.source} | v1={evt.sensor_val1?.toFixed(1)} v2={evt.sensor_val2?.toFixed(1)}
                    </div>
                  </div>
                </div>

                <div className="mc-mono" style={{ fontSize: 'var(--text-xs)', color: 'var(--text-secondary)', whiteSpace: 'nowrap' }}>
                  {evt.timestamp ? new Date(evt.timestamp).toLocaleTimeString() : 'Live'}
                </div>
              </div>
            );
          })
        )}
      </div>
    </section>
  );
}
