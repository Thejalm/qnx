import React from 'react';
import { Thermometer, Droplets, Gauge, Flame, Wind, AlertTriangle } from 'lucide-react';

export default function LiveSensors({ sensorData }) {
  const temp = sensorData?.temperature ?? 26.5;
  const hum = sensorData?.humidity ?? 52.0;
  const press = sensorData?.pressure ?? 1013.25;
  const gasAdc = sensorData?.mq2_raw_adc ?? 320;
  const gasAlert = sensorData?.mq2_digital_alert ?? false;
  const flameDet = sensorData?.flame_detected ?? false;
  const flameAdc = sensorData?.flame_raw_adc ?? 3800;

  // Gas level percent (0 - 4095 mapped to 0-100%)
  const gasPct = Math.min(100, Math.max(0, (gasAdc / 3500) * 100));

  return (
    <div className="glass-card">
      <div className="section-title">
        <Wind size={20} color="#38bdf8" />
        <span>ESP32-C3 Input Node Sensors (10 Hz Telemetry)</span>
      </div>

      <div className="grid-3-col">
        {/* Temperature Card */}
        <div style={{
          background: 'rgba(255,255,255,0.02)',
          border: '1px solid rgba(255,255,255,0.06)',
          borderRadius: '12px',
          padding: '1rem',
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'space-between'
        }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span style={{ fontSize: '0.85rem', color: 'var(--text-secondary)' }}>BME280 Temperature</span>
            <Thermometer size={20} color={temp > 40 ? '#f43f5e' : temp > 32 ? '#f59e0b' : '#38bdf8'} />
          </div>
          <div style={{ marginTop: '0.8rem', display: 'flex', alignItems: 'baseline', gap: '0.4rem' }}>
            <span className="mono-val" style={{
              fontSize: '2rem',
              color: temp > 40 ? '#fb7185' : temp > 32 ? '#fbbf24' : '#f8fafc'
            }}>
              {temp.toFixed(1)}
            </span>
            <span style={{ fontSize: '1rem', color: 'var(--text-muted)' }}>°C</span>
          </div>
          <div style={{ marginTop: '0.6rem', fontSize: '0.75rem', color: 'var(--text-muted)' }}>
            Safety Threshold: &gt;35°C (Fan) / &gt;50°C (Critical)
          </div>
        </div>

        {/* Humidity & Pressure */}
        <div style={{
          background: 'rgba(255,255,255,0.02)',
          border: '1px solid rgba(255,255,255,0.06)',
          borderRadius: '12px',
          padding: '1rem',
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'space-between'
        }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span style={{ fontSize: '0.85rem', color: 'var(--text-secondary)' }}>Humidity & Barometer</span>
            <Droplets size={20} color="#06b6d4" />
          </div>
          <div style={{ marginTop: '0.8rem', display: 'flex', justifyContent: 'space-between' }}>
            <div>
              <div className="mono-val" style={{ fontSize: '1.4rem' }}>{hum.toFixed(1)}%</div>
              <span style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>RH Level</span>
            </div>
            <div style={{ textAlign: 'right' }}>
              <div className="mono-val" style={{ fontSize: '1.4rem' }}>{press.toFixed(0)}</div>
              <span style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>hPa Atmos</span>
            </div>
          </div>
          <div style={{ marginTop: '0.6rem', fontSize: '0.75rem', color: 'var(--text-muted)' }}>
            I2C Bus: GPIO 4 (SDA) / GPIO 5 (SCL)
          </div>
        </div>

        {/* MQ-2 Flammable Gas Sensor */}
        <div style={{
          background: gasAlert || gasAdc > 1200 ? 'rgba(245, 158, 11, 0.08)' : 'rgba(255,255,255,0.02)',
          border: gasAlert || gasAdc > 1200 ? '1px solid rgba(245, 158, 11, 0.4)' : '1px solid rgba(255,255,255,0.06)',
          borderRadius: '12px',
          padding: '1rem',
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'space-between'
        }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span style={{ fontSize: '0.85rem', color: 'var(--text-secondary)' }}>MQ-2 Flammable Gas</span>
            <AlertTriangle size={20} color={gasAlert || gasAdc > 1200 ? '#fbbf24' : '#10b981'} />
          </div>
          <div style={{ marginTop: '0.5rem' }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'baseline' }}>
              <span className="mono-val" style={{ fontSize: '1.8rem', color: gasAdc > 1500 ? '#fbbf24' : '#f8fafc' }}>
                {gasAdc}
              </span>
              <span style={{ fontSize: '0.8rem', color: 'var(--text-muted)' }}>ADC / {gasPct.toFixed(0)}%</span>
            </div>
            {/* Progress bar */}
            <div style={{ width: '100%', height: '6px', background: 'rgba(255,255,255,0.1)', borderRadius: '3px', marginTop: '0.4rem', overflow: 'hidden' }}>
              <div style={{
                width: `${gasPct}%`,
                height: '100%',
                background: gasPct > 60 ? '#f43f5e' : gasPct > 35 ? '#f59e0b' : '#10b981',
                transition: 'width 0.2s ease'
              }} />
            </div>
          </div>
          <div style={{ marginTop: '0.6rem', fontSize: '0.75rem', color: gasAlert ? '#fbbf24' : 'var(--text-muted)' }}>
            Digital Trip: {gasAlert ? '⚠️ HAZARD ALERT' : 'NORMAL'}
          </div>
        </div>
      </div>

      {/* Flame Sensor Banner */}
      <div style={{
        marginTop: '1.25rem',
        padding: '0.85rem 1.25rem',
        borderRadius: '10px',
        background: flameDet ? 'rgba(244, 63, 94, 0.2)' : 'rgba(255,255,255,0.02)',
        border: flameDet ? '1px solid rgba(244, 63, 94, 0.6)' : '1px solid rgba(255,255,255,0.05)',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'space-between',
        animation: flameDet ? 'pulse-critical 1s infinite' : 'none'
      }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: '0.8rem' }}>
          <Flame size={24} color={flameDet ? '#fb7185' : '#64748b'} />
          <div>
            <div style={{ fontWeight: 700, color: flameDet ? '#fb7185' : 'var(--text-primary)' }}>
              {flameDet ? '🔥 ACTIVE FLAME DETECTED - SUPPRESSION ENGAGED' : 'Optical IR Flame Detector: Clear'}
            </div>
            <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>
              IR Intensity: {flameAdc} ADC (Trigger threshold: &lt; 1000 ADC)
            </div>
          </div>
        </div>
        <span className={`status-badge ${flameDet ? 'badge-critical' : 'badge-normal'}`}>
          {flameDet ? 'FIRE ALARM' : 'SAFE'}
        </span>
      </div>
    </div>
  );
}
