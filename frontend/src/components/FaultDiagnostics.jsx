import React from 'react';
import { AlertOctagon, CheckCircle2, ShieldOff, Cable, Radio } from 'lucide-react';

export default function FaultDiagnostics({ sensorData, systemStatus }) {
  const faultFlags = sensorData?.fault_flags ?? 0;
  const isCommHealthy = systemStatus?.is_comm_healthy ?? true;
  const failsafeActive = systemStatus?.latest_actuator?.failsafe_active ?? false;

  const bmeFault = Boolean(faultFlags & 0x01);
  const mq2Fault = Boolean(faultFlags & 0x02);
  const flameFault = Boolean(faultFlags & 0x04);

  return (
    <div className="glass-card" style={{ display: 'flex', flexDirection: 'column', height: '100%' }}>
      <div className="section-title">
        <AlertOctagon size={20} color="#8b5cf6" />
        <span>Hardware Faults &amp; Fail-Safe Watchdog</span>
      </div>

      <div style={{ display: 'flex', flexDirection: 'column', gap: '0.75rem', flex: 1 }}>
        {/* Watchdog Status */}
        <div style={{
          background: failsafeActive ? 'rgba(244, 63, 94, 0.15)' : 'rgba(255,255,255,0.02)',
          border: failsafeActive ? '1px solid rgba(244, 63, 94, 0.5)' : '1px solid rgba(255,255,255,0.06)',
          borderRadius: '8px',
          padding: '0.75rem',
          display: 'flex',
          justifyContent: 'space-between',
          alignItems: 'center'
        }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: '0.6rem' }}>
            <Radio size={18} color={failsafeActive ? '#fb7185' : '#34d399'} />
            <div>
              <div style={{ fontSize: '0.85rem', fontWeight: 600 }}>2.0s Output Watchdog</div>
              <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>Auto-safing on link loss</div>
            </div>
          </div>
          <span className={`status-badge ${failsafeActive ? 'badge-critical' : 'badge-normal'}`}>
            {failsafeActive ? 'TRIPPED' : 'ARMED'}
          </span>
        </div>

        {/* BME280 Bus Health */}
        <div style={{
          background: 'rgba(255,255,255,0.02)',
          border: '1px solid rgba(255,255,255,0.06)',
          borderRadius: '8px',
          padding: '0.75rem',
          display: 'flex',
          justifyContent: 'space-between',
          alignItems: 'center'
        }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: '0.6rem' }}>
            {bmeFault ? <ShieldOff size={18} color="#fb7185" /> : <CheckCircle2 size={18} color="#34d399 Wand" />}
            <div>
              <div style={{ fontSize: '0.85rem', fontWeight: 600 }}>BME280 I2C Bus Link</div>
              <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>Addr: 0x76 / 0x77</div>
            </div>
          </div>
          <span className={`status-badge ${bmeFault ? 'badge-fault' : 'badge-normal'}`}>
            {bmeFault ? 'FAULT' : 'ONLINE'}
          </span>
        </div>

        {/* MQ-2 Circuit Integrity */}
        <div style={{
          background: 'rgba(255,255,255,0.02)',
          border: '1px solid rgba(255,255,255,0.06)',
          borderRadius: '8px',
          padding: '0.75rem',
          display: 'flex',
          justifyContent: 'space-between',
          alignItems: 'center'
        }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: '0.6rem' }}>
            {mq2Fault ? <Cable size={18} color="#fb7185" /> : <CheckCircle2 size={18} color="#34d399" />}
            <div>
              <div style={{ fontSize: '0.85rem', fontWeight: 600 }}>MQ-2 Analog Circuit</div>
              <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>ADC Open/Short Checker</div>
            </div>
          </div>
          <span className={`status-badge ${mq2Fault ? 'badge-fault' : 'badge-normal'}`}>
            {mq2Fault ? 'OPEN/SHORT' : 'HEALTHY'}
          </span>
        </div>

        {/* Flame Sensor Integrity */}
        <div style={{
          background: 'rgba(255,255,255,0.02)',
          border: '1px solid rgba(255,255,255,0.06)',
          borderRadius: '8px',
          padding: '0.75rem',
          display: 'flex',
          justifyContent: 'space-between',
          alignItems: 'center'
        }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: '0.6rem' }}>
            {flameFault ? <ShieldOff size={18} color="#fb7185" /> : <CheckCircle2 size={18} color="#34d399" />}
            <div>
              <div style={{ fontSize: '0.85rem', fontWeight: 600 }}>Optical Flame Detector</div>
              <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>Digital IR Receiver</div>
            </div>
          </div>
          <span className={`status-badge ${flameFault ? 'badge-fault' : 'badge-normal'}`}>
            {flameFault ? 'FAULT' : 'OPERATIONAL'}
          </span>
        </div>
      </div>
    </div>
  );
}
