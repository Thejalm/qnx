import React from 'react';
import { Fan, Droplet, Bell, Volume2, Monitor } from 'lucide-react';

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
    <section className="mc-card">
      <div className="mc-section-header">
        <div className="mc-section-title">
          <Monitor size={20} color="var(--color-2)" />
          <span>Output actuators and hardware display</span>
        </div>
        <span className="mc-badge mc-badge-subtle">
          ESP32-C3 Node 2
        </span>
      </div>

      <div className="mc-grid-4">
        {/* Relay 1: Exhaust Fan */}
        <div className="mc-card-nested" style={{ display: 'flex', flexDirection: 'column', gap: 'var(--space-2)' }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <Fan size={22} color={fanRun ? 'var(--color-2)' : 'var(--text-secondary)'} />
            <span className={`mc-badge ${fanRun ? 'mc-badge-dark' : 'mc-badge-subtle'}`}>
              {fanRun ? 'Running' : 'Stopped'}
            </span>
          </div>
          <div>
            <div style={{ fontSize: 'var(--text-sm)', fontWeight: 'var(--font-weight-bold)' }}>Cooling fan</div>
            <p className="mc-section-subtitle">Relay 1 (GPIO 2)</p>
          </div>
        </div>

        {/* Relay 2: Water Pump */}
        <div className="mc-card-nested" style={{ display: 'flex', flexDirection: 'column', gap: 'var(--space-2)' }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <Droplet size={22} color={pumpRun ? 'var(--color-2)' : 'var(--text-secondary)'} />
            <span className={`mc-badge ${pumpRun ? 'mc-badge-accent' : 'mc-badge-subtle'}`}>
              {pumpRun ? 'Active' : 'Idle'}
            </span>
          </div>
          <div>
            <div style={{ fontSize: 'var(--text-sm)', fontWeight: 'var(--font-weight-bold)' }}>Water pump</div>
            <p className="mc-section-subtitle">Relay 2 (GPIO 3)</p>
          </div>
        </div>

        {/* Buzzer 1: Primary Siren */}
        <div className="mc-card-nested" style={{ display: 'flex', flexDirection: 'column', gap: 'var(--space-2)' }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <Bell size={22} color={buz1 ? 'var(--color-2)' : 'var(--text-secondary)'} />
            <span className={`mc-badge ${buz1 ? 'mc-badge-accent' : 'mc-badge-subtle'}`}>
              {buz1 ? 'Alarm on' : 'Silent'}
            </span>
          </div>
          <div>
            <div style={{ fontSize: 'var(--text-sm)', fontWeight: 'var(--font-weight-bold)' }}>Primary siren</div>
            <p className="mc-section-subtitle">Buzzer 1 (GPIO 0)</p>
          </div>
        </div>

        {/* Buzzer 2: Warning Beep */}
        <div className="mc-card-nested" style={{ display: 'flex', flexDirection: 'column', gap: 'var(--space-2)' }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <Volume2 size={22} color={buz2 ? 'var(--color-2)' : 'var(--text-secondary)'} />
            <span className={`mc-badge ${buz2 ? 'mc-badge-dark' : 'mc-badge-subtle'}`}>
              {buz2 ? 'Beeping' : 'Muted'}
            </span>
          </div>
          <div>
            <div style={{ fontSize: 'var(--text-sm)', fontWeight: 'var(--font-weight-bold)' }}>Warning beeper</div>
            <p className="mc-section-subtitle">Buzzer 2 (GPIO 1)</p>
          </div>
        </div>
      </div>

      {/* OLED & Discrete LEDs Footer Bar */}
      <div
        style={{
          marginTop: 'var(--space-4)',
          display: 'grid',
          gridTemplateColumns: 'repeat(auto-fit, minmax(280px, 1fr))',
          gap: 'var(--space-4)',
          alignItems: 'center'
        }}
      >
        {/* Discrete LED Indicators */}
        <div className="mc-card-nested" style={{ display: 'flex', justifyContent: 'space-around', alignItems: 'center' }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-2)' }}>
            <div
              style={{
                width: '12px',
                height: '12px',
                borderRadius: 'var(--radius-lg)',
                backgroundColor: ledYellow ? 'var(--color-2)' : 'var(--text-secondary)',
                transition: 'var(--duration-fast)'
              }}
            />
            <div>
              <div style={{ fontSize: 'var(--text-xs)', fontWeight: 'var(--font-weight-bold)' }}>Yellow LED</div>
              <p style={{ fontSize: 'var(--text-xs)', color: 'var(--text-secondary)' }}>GPIO 6</p>
            </div>
          </div>

          <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-2)' }}>
            <div
              style={{
                width: '12px',
                height: '12px',
                borderRadius: 'var(--radius-lg)',
                backgroundColor: ledRed ? 'var(--color-2)' : 'var(--text-secondary)',
                transition: 'var(--duration-fast)'
              }}
            />
            <div>
              <div style={{ fontSize: 'var(--text-xs)', fontWeight: 'var(--font-weight-bold)' }}>Red LED</div>
              <p style={{ fontSize: 'var(--text-xs)', color: 'var(--text-secondary)' }}>GPIO 7</p>
            </div>
          </div>
        </div>

        {/* SSD1306 OLED Display Emulation */}
        <div
          className="mc-card-nested"
          style={{
            backgroundColor: 'var(--color-1)',
            color: 'var(--color-6)',
            padding: 'var(--space-3) var(--space-4)',
            display: 'flex',
            flexDirection: 'column',
            gap: 'var(--space-1)'
          }}
        >
          <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: 'var(--text-xs)', color: 'var(--text-secondary)' }}>
            <span>SSD1306 OLED (128x64)</span>
            <span>I2C: 0x3C</span>
          </div>
          <div className="mc-mono" style={{ fontSize: 'var(--text-sm)', color: 'var(--color-6)' }}>
            STATUS: {failsafe ? 'FAILSAFE (LOST LINK)' : statusText}
          </div>
          <div className="mc-mono" style={{ fontSize: 'var(--text-xs)', color: 'var(--text-secondary)' }}>
            FAN:[{fanRun ? 'ON ' : 'OFF'}] PUMP:[{pumpRun ? 'ON ' : 'OFF'}] LINK:[{failsafe ? 'DOWN' : 'OK'}]
          </div>
        </div>
      </div>
    </section>
  );
}
