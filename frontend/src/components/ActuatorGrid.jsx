import React from 'react';
import { Fan, Droplet, Bell, Volume2, Lightbulb, Monitor } from 'lucide-react';

export default function ActuatorGrid({ actuatorState }) {
  const fanRun = actuatorState?.relay_fan ?? false;
  const pumpRun = actuatorState?.relay_pump ?? false;
  const buz1 = actuatorState?.buzzer1_alarm ?? false;
  const buz2 = actuatorState?.buzzer2_warning ?? false;
  const ledYellow = actuatorState?.led_yellow ?? false;
  const ledRed = actuatorState?.led_red ?? false;
  const statusText = actuatorState?.status_text || 'NORMAL';
  const failsafe = actuatorState?.failsafe_active ?? false;

  return (
    <div className="glass-card">
      <div className="section-title">
        <Monitor size={20} color="#38bdf8" />
        <span>ESP32-C3 Output Actuators & Hardware Display</span>
      </div>

      <div className="grid-4-col">
        {/* Relay 1: Exhaust Fan */}
        <div style={{
          background: fanRun ? 'rgba(56, 189, 248, 0.1)' : 'rgba(255,255,255,0.02)',
          border: fanRun ? '1px solid rgba(56, 189, 248, 0.4)' : '1px solid rgba(255,255,255,0.06)',
          borderRadius: '12px',
          padding: '1.1rem',
          display: 'flex',
          flexDirection: 'column',
          alignItems: 'center',
          textAlign: 'center',
          gap: '0.6rem'
        }}>
          <Fan size={36} className={fanRun ? 'fan-running' : 'fan-stopped'} />
          <div style={{ fontWeight: 700, fontSize: '0.95rem' }}>Exhaust / Cooling Fan</div>
          <span style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>Relay 1 (GPIO 2)</span>
          <span className={`status-badge ${fanRun ? 'badge-normal' : 'badge-offline'}`}>
            {fanRun ? 'RUNNING' : 'STOPPED'}
          </span>
        </div>

        {/* Relay 2: Water Pump */}
        <div style={{
          background: pumpRun ? 'rgba(59, 130, 246, 0.15)' : 'rgba(255,255,255,0.02)',
          border: pumpRun ? '1px solid rgba(59, 130, 246, 0.5)' : '1px solid rgba(255,255,255,0.06)',
          borderRadius: '12px',
          padding: '1.1rem',
          display: 'flex',
          flexDirection: 'column',
          alignItems: 'center',
          textAlign: 'center',
          gap: '0.6rem'
        }}>
          <Droplet size={36} color={pumpRun ? '#38bdf8' : 'var(--text-muted)'} />
          <div style={{ fontWeight: 700, fontSize: '0.95rem' }}>Water Suppressor Pump</div>
          <span style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>Relay 2 (GPIO 3)</span>
          <span className={`status-badge ${pumpRun ? 'badge-critical' : 'badge-offline'}`}>
            {pumpRun ? 'SUPPRESSION' : 'IDLE'}
          </span>
        </div>

        {/* Primary Alarm Buzzer */}
        <div style={{
          background: buz1 ? 'rgba(244, 63, 94, 0.15)' : 'rgba(255,255,255,0.02)',
          border: buz1 ? '1px solid rgba(244, 63, 94, 0.5)' : '1px solid rgba(255,255,255,0.06)',
          borderRadius: '12px',
          padding: '1.1rem',
          display: 'flex',
          flexDirection: 'column',
          alignItems: 'center',
          textAlign: 'center',
          gap: '0.6rem'
        }}>
          <Bell size={36} color={buz1 ? '#fb7185' : 'var(--text-muted)'} />
          <div style={{ fontWeight: 700, fontSize: '0.95rem' }}>Primary Siren Buzzer</div>
          <span style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>Buzzer 1 (GPIO 0)</span>
          <span className={`status-badge ${buz1 ? 'badge-critical' : 'badge-offline'}`}>
            {buz1 ? 'ALARM ON' : 'SILENT'}
          </span>
        </div>

        {/* Warning Beep Buzzer */}
        <div style={{
          background: buz2 ? 'rgba(245, 158, 11, 0.12)' : 'rgba(255,255,255,0.02)',
          border: buz2 ? '1px solid rgba(245, 158, 11, 0.4)' : '1px solid rgba(255,255,255,0.06)',
          borderRadius: '12px',
          padding: '1.1rem',
          display: 'flex',
          flexDirection: 'column',
          alignItems: 'center',
          textAlign: 'center',
          gap: '0.6rem'
        }}>
          <Volume2 size={36} color={buz2 ? '#fbbf24' : 'var(--text-muted)'} />
          <div style={{ fontWeight: 700, fontSize: '0.95rem' }}>Warning Beep Buzzer</div>
          <span style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>Buzzer 2 (GPIO 1)</span>
          <span className={`status-badge ${buz2 ? 'badge-warning' : 'badge-offline'}`}>
            {buz2 ? 'BEEPING' : 'MUTED'}
          </span>
        </div>
      </div>

      {/* OLED & Discrete LEDs Footer Bar */}
      <div style={{
        marginTop: '1.25rem',
        display: 'grid',
        gridTemplateColumns: 'repeat(auto-fit, minmax(280px, 1fr))',
        gap: '1rem',
        alignItems: 'center'
      }}>
        {/* Discrete LED Panel */}
        <div style={{
          background: 'rgba(255,255,255,0.02)',
          border: '1px solid rgba(255,255,255,0.06)',
          borderRadius: '10px',
          padding: '0.85rem 1.25rem',
          display: 'flex',
          justifyContent: 'space-around',
          alignItems: 'center'
        }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: '0.6rem' }}>
            <div style={{
              width: '14px',
              height: '14px',
              borderRadius: '50%',
              background: ledYellow ? '#fbbf24' : '#334155',
              boxShadow: ledYellow ? '0 0 10px #fbbf24' : 'none',
              transition: 'all 0.2s ease'
            }} />
            <span style={{ fontSize: '0.85rem', fontWeight: 600 }}>Yellow LED (GPIO 6)</span>
          </div>
          <div style={{ display: 'flex', alignItems: 'center', gap: '0.6rem' }}>
            <div style={{
              width: '14px',
              height: '14px',
              borderRadius: '50%',
              background: ledRed ? '#fb7185' : '#334155',
              boxShadow: ledRed ? '0 0 10px #fb7185' : 'none',
              transition: 'all 0.2s ease'
            }} />
            <span style={{ fontSize: '0.85rem', fontWeight: 600 }}>Red LED (GPIO 7)</span>
          </div>
        </div>

        {/* OLED Emulation Banner */}
        <div style={{
          background: '#000000',
          border: '1px solid #38bdf8',
          borderRadius: '10px',
          padding: '0.75rem 1.25rem',
          fontFamily: 'var(--font-mono)',
          color: '#38bdf8',
          boxShadow: '0 0 15px rgba(56, 189, 248, 0.2)'
        }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.75rem', borderBottom: '1px dashed #38bdf8', paddingBottom: '0.3rem' }}>
            <span>SSD1306 128x64 OLED</span>
            <span>I2C: 0x3C</span>
          </div>
          <div style={{ marginTop: '0.4rem', fontSize: '0.9rem', fontWeight: 700 }}>
            SYS: {failsafe ? 'FAILSAFE (LOST LINK)' : statusText}
          </div>
          <div style={{ fontSize: '0.75rem', marginTop: '0.2rem', color: '#94a3b8' }}>
            FAN:[{fanRun ? 'RUN ' : 'STOP'}] PUMP:[{pumpRun ? 'RUN ' : 'STOP'}] LINK:[{failsafe ? 'DOWN' : 'LIVE'}]
          </div>
        </div>
      </div>
    </div>
  );
}
