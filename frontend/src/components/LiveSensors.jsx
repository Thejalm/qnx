import React from 'react';
import { Thermometer, Droplets, Flame, Wind, AlertTriangle } from 'lucide-react';

export default function LiveSensors({ sensorData }) {
  const temp = sensorData?.temperature ?? 26.5;
  const hum = sensorData?.humidity ?? 52.0;
  const press = sensorData?.pressure ?? 1013.25;
  const gasAdc = sensorData?.mq2_raw_adc ?? 320;
  const gasAlert = sensorData?.mq2_digital_alert ?? false;
  const flameDet = sensorData?.flame_detected ?? false;
  const flameAdc = sensorData?.flame_raw_adc ?? 3800;

  // Gas level percent (0 - 3500 mapped to 0-100%)
  const gasPct = Math.min(100, Math.max(0, (gasAdc / 3500) * 100));

  return (
    <section className="mc-card">
      <div className="mc-section-header">
        <div className="mc-section-title">
          <Wind size={20} color="var(--color-2)" />
          <span>Input node sensor telemetry</span>
        </div>
        <span className="mc-badge mc-badge-subtle">
          ESP32-C3 Node 1
        </span>
      </div>

      <div className="mc-grid-3">
        {/* BME280 Temperature */}
        <div className="mc-card-nested">
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span className="mc-stat-label">BME280 temperature</span>
            <Thermometer size={18} color={temp > 35 ? 'var(--color-2)' : 'var(--color-1)'} />
          </div>
          <div style={{ margin: 'var(--space-2) 0', display: 'flex', alignItems: 'baseline', gap: 'var(--space-1)' }}>
            <span className="mc-stat-value mc-mono" style={{ color: temp > 35 ? 'var(--color-2)' : 'var(--color-1)' }}>
              {temp.toFixed(1)}
            </span>
            <span style={{ fontSize: 'var(--text-sm)', color: 'var(--text-secondary)' }}>°C</span>
          </div>
          <p className="mc-section-subtitle">
            Threshold: &gt;35°C fan / &gt;50°C critical
          </p>
        </div>

        {/* Humidity & Barometer */}
        <div className="mc-card-nested">
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span className="mc-stat-label">Humidity and pressure</span>
            <Droplets size={18} color="var(--color-1)" />
          </div>
          <div style={{ margin: 'var(--space-2) 0', display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start' }}>
            <div>
              <span className="mc-stat-value mc-mono">{hum.toFixed(1)}</span>
              <span style={{ fontSize: 'var(--text-xs)', color: 'var(--text-secondary)', marginLeft: 'var(--space-1)' }}>% RH</span>
            </div>
            <div style={{ textAlign: 'right' }}>
              <span className="mc-stat-value mc-mono">{press.toFixed(0)}</span>
              <span style={{ fontSize: 'var(--text-xs)', color: 'var(--text-secondary)', marginLeft: 'var(--space-1)' }}>hPa</span>
            </div>
          </div>
          <p className="mc-section-subtitle">
            I2C Bus: GPIO 4 (SDA) / GPIO 5 (SCL)
          </p>
        </div>

        {/* MQ-2 Flammable Gas Sensor */}
        <div className="mc-card-nested">
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span className="mc-stat-label">MQ-2 flammable gas</span>
            <AlertTriangle size={18} color={gasAlert || gasAdc > 1200 ? 'var(--color-2)' : 'var(--text-secondary)'} />
          </div>
          <div style={{ margin: 'var(--space-2) 0' }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'baseline' }}>
              <span className="mc-stat-value mc-mono" style={{ color: gasAdc > 1500 ? 'var(--color-2)' : 'var(--color-1)' }}>
                {gasAdc}
              </span>
              <span className="mc-stat-label mc-mono">{gasPct.toFixed(0)}% ADC</span>
            </div>
            <div className="mc-meter">
              <div
                className="mc-meter-fill"
                style={{
                  width: `${gasPct}%`,
                  backgroundColor: gasPct > 50 ? 'var(--color-2)' : 'var(--color-1)'
                }}
              />
            </div>
          </div>
          <p className="mc-section-subtitle">
            Digital alert: {gasAlert ? 'Hazard triggered' : 'Normal'}
          </p>
        </div>
      </div>

      {/* Optical Flame Detector Banner */}
      <div
        className="mc-card-nested"
        style={{
          marginTop: 'var(--space-4)',
          backgroundColor: flameDet ? 'var(--color-2)' : 'var(--color-4)',
          color: flameDet ? 'var(--color-6)' : 'var(--color-1)',
          display: 'flex',
          justifyContent: 'space-between',
          alignItems: 'center',
          flexWrap: 'wrap',
          gap: 'var(--space-3)'
        }}
      >
        <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-3)' }}>
          <Flame size={24} color={flameDet ? 'var(--color-6)' : 'var(--color-2)'} />
          <div>
            <div style={{ fontSize: 'var(--text-sm)', fontWeight: 'var(--font-weight-bold)' }}>
              {flameDet ? 'Active flame detected — suppression active' : 'Optical infrared flame detector: Clear'}
            </div>
            <p style={{ fontSize: 'var(--text-xs)', color: flameDet ? 'var(--color-6)' : 'var(--text-secondary)' }}>
              IR intensity: {flameAdc} ADC (Trigger threshold &lt; 1000 ADC)
            </p>
          </div>
        </div>
        <span className={`mc-badge ${flameDet ? 'mc-badge-dark' : 'mc-badge-subtle'}`}>
          {flameDet ? 'Fire Critical' : 'Sensor Secure'}
        </span>
      </div>
    </section>
  );
}
