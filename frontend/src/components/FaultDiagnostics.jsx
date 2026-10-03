import React from 'react';
import { AlertOctagon, CheckCircle2, ShieldOff, Cable, Radio } from 'lucide-react';

export default function FaultDiagnostics({ sensorData, systemStatus }) {
  const faultFlags = sensorData?.fault_flags ?? 0;
  const failsafeActive = systemStatus?.latest_actuator?.failsafe_active ?? false;

  const bmeFault = Boolean(faultFlags & 0x01);
  const mq2Fault = Boolean(faultFlags & 0x02);
  const flameFault = Boolean(faultFlags & 0x04);

  return (
    <section className="mc-card" style={{ height: '100%' }}>
      <div className="mc-section-header">
        <div className="mc-section-title">
          <AlertOctagon size={20} color="var(--color-2)" />
          <span>Hardware diagnostics and fail-safe watchdog</span>
        </div>
        <span className="mc-badge mc-badge-subtle">
          Watchdog Armed
        </span>
      </div>

      <div style={{ display: 'flex', flexDirection: 'column', gap: 'var(--space-3)', flex: 1 }}>
        {/* 2.0s Output Watchdog */}
        <div
          className="mc-card-nested"
          style={{
            display: 'flex',
            justifyContent: 'space-between',
            alignItems: 'center',
            padding: 'var(--space-3) var(--space-4)'
          }}
        >
          <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-3)' }}>
            <Radio size={18} color={failsafeActive ? 'var(--color-2)' : 'var(--color-1)'} />
            <div>
              <div style={{ fontSize: 'var(--text-sm)', fontWeight: 'var(--font-weight-bold)' }}>2.0s output watchdog</div>
              <p className="mc-section-subtitle">Auto-safing on communication loss</p>
            </div>
          </div>
          <span className={`mc-badge ${failsafeActive ? 'mc-badge-accent' : 'mc-badge-dark'}`}>
            {failsafeActive ? 'Tripped' : 'Armed'}
          </span>
        </div>

        {/* BME280 Bus Health */}
        <div
          className="mc-card-nested"
          style={{
            display: 'flex',
            justifyContent: 'space-between',
            alignItems: 'center',
            padding: 'var(--space-3) var(--space-4)'
          }}
        >
          <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-3)' }}>
            {bmeFault ? <ShieldOff size={18} color="var(--color-2)" /> : <CheckCircle2 size={18} color="var(--color-1)" />}
            <div>
              <div style={{ fontSize: 'var(--text-sm)', fontWeight: 'var(--font-weight-bold)' }}>BME280 I2C bus link</div>
              <p className="mc-section-subtitle">Address: 0x76 / 0x77</p>
            </div>
          </div>
          <span className={`mc-badge ${bmeFault ? 'mc-badge-accent' : 'mc-badge-subtle'}`}>
            {bmeFault ? 'Fault' : 'Online'}
          </span>
        </div>

        {/* MQ-2 Circuit Integrity */}
        <div
          className="mc-card-nested"
          style={{
            display: 'flex',
            justifyContent: 'space-between',
            alignItems: 'center',
            padding: 'var(--space-3) var(--space-4)'
          }}
        >
          <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-3)' }}>
            {mq2Fault ? <Cable size={18} color="var(--color-2)" /> : <CheckCircle2 size={18} color="var(--color-1)" />}
            <div>
              <div style={{ fontSize: 'var(--text-sm)', fontWeight: 'var(--font-weight-bold)' }}>MQ-2 analog circuit</div>
              <p className="mc-section-subtitle">ADC open/short integrity checker</p>
            </div>
          </div>
          <span className={`mc-badge ${mq2Fault ? 'mc-badge-accent' : 'mc-badge-subtle'}`}>
            {mq2Fault ? 'Open / Short' : 'Healthy'}
          </span>
        </div>

        {/* Flame Sensor Integrity */}
        <div
          className="mc-card-nested"
          style={{
            display: 'flex',
            justifyContent: 'space-between',
            alignItems: 'center',
            padding: 'var(--space-3) var(--space-4)'
          }}
        >
          <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-3)' }}>
            {flameFault ? <ShieldOff size={18} color="var(--color-2)" /> : <CheckCircle2 size={18} color="var(--color-1)" />}
            <div>
              <div style={{ fontSize: 'var(--text-sm)', fontWeight: 'var(--font-weight-bold)' }}>Optical flame detector</div>
              <p className="mc-section-subtitle">Digital IR receiver circuit</p>
            </div>
          </div>
          <span className={`mc-badge ${flameFault ? 'mc-badge-accent' : 'mc-badge-subtle'}`}>
            {flameFault ? 'Fault' : 'Operational'}
          </span>
        </div>
      </div>
    </section>
  );
}
