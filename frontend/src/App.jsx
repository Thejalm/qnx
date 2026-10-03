import React, { useState, useEffect } from 'react';
import Header from './components/Header';
import LiveSensors from './components/LiveSensors';
import ActuatorGrid from './components/ActuatorGrid';
import RealtimeMetrics from './components/RealtimeMetrics';
import SafetyEventsLog from './components/SafetyEventsLog';
import FaultDiagnostics from './components/FaultDiagnostics';

const API_BASE = 'http://localhost:8000/api';
const WS_URL = 'ws://localhost:8000/ws/telemetry';

export default function App() {
  const [sensorData, setSensorData] = useState({
    temperature: 26.5,
    humidity: 52.0,
    pressure: 1013.25,
    mq2_raw_adc: 320,
    mq2_digital_alert: false,
    flame_detected: false,
    flame_raw_adc: 3800,
    sequence_number: 1,
    fault_flags: 0
  });

  const [actuatorState, setActuatorState] = useState({
    relay_fan: false,
    relay_pump: false,
    buzzer1_alarm: false,
    buzzer2_warning: false,
    led_yellow: false,
    led_red: false,
    status_text: 'NORMAL',
    failsafe_active: false
  });

  const [systemStatus, setSystemStatus] = useState({
    qnx_state: 'NORMAL',
    latency_us: 18.5,
    eval_latency_us: 18.5,
    loop_latency_ms: 100.0,
    is_comm_healthy: true,
    total_packets_received: 1,
    total_dropped_packets: 0
  });

  const [events, setEvents] = useState([]);
  const [wsConnected, setWsConnected] = useState(false);

  // 1. WebSocket Live Telemetry Connection
  useEffect(() => {
    let ws;
    const connectWs = () => {
      try {
        ws = new WebSocket(WS_URL);
        ws.onopen = () => setWsConnected(true);
        ws.onmessage = (event) => {
          try {
            const data = JSON.parse(event.data);
            setSensorData(data);

            setSystemStatus(prev => ({
              ...prev,
              eval_latency_us: data.eval_latency_us || data.latency_us || 18.5,
              loop_latency_ms: data.loop_latency_ms || 100.0,
              latency_us: data.eval_latency_us || data.latency_us || 18.5,
              total_packets_received: data.sequence_number || prev.total_packets_received
            }));

            // Compute local actuator estimation from state machine rules if live
            if (data.flame_detected || data.temperature >= 50) {
              setActuatorState(prev => ({ ...prev, relay_fan: true, relay_pump: true, buzzer1_alarm: true, led_red: true, status_text: 'FIRE_CRITICAL' }));
              setSystemStatus(prev => ({ ...prev, qnx_state: 'FIRE_CRITICAL' }));
            } else if (data.mq2_digital_alert || data.mq2_raw_adc >= 1800) {
              setActuatorState(prev => ({ ...prev, relay_fan: true, relay_pump: false, buzzer2_warning: true, led_yellow: true, status_text: 'GAS_WARNING' }));
              setSystemStatus(prev => ({ ...prev, qnx_state: 'GAS_WARNING' }));
            } else if (data.temperature >= 35) {
              setActuatorState(prev => ({ ...prev, relay_fan: true, relay_pump: false, led_yellow: true, status_text: 'ELEVATED_TEMP' }));
              setSystemStatus(prev => ({ ...prev, qnx_state: 'ELEVATED_TEMP' }));
            } else {
              setActuatorState(prev => ({ ...prev, relay_fan: false, relay_pump: false, buzzer1_alarm: false, buzzer2_warning: false, led_yellow: false, led_red: false, status_text: 'NORMAL' }));
              setSystemStatus(prev => ({ ...prev, qnx_state: 'NORMAL' }));
            }
          } catch (err) {}
        };
        ws.onclose = () => {
          setWsConnected(false);
          setTimeout(connectWs, 2000);
        };
      } catch (err) {
        setWsConnected(false);
        setTimeout(connectWs, 2000);
      }
    };
    connectWs();
    return () => ws && ws.close();
  }, []);

  // 2. Periodic REST Poll for System Status & Events (Fallback & Complementary)
  useEffect(() => {
    const fetchData = async () => {
      try {
        const [resStatus, resEvents, resDevices] = await Promise.allSettled([
          fetch(`${API_BASE}/system/status`).then(r => r.json()),
          fetch(`${API_BASE}/events?limit=20`).then(r => r.json()),
          fetch(`${API_BASE}/devices`).then(r => r.json())
        ]);

        if (resStatus.status === 'fulfilled' && resStatus.value) {
          const val = resStatus.value;
          setSystemStatus(prev => ({
            ...prev,
            ...val,
            eval_latency_us: val.eval_latency_us || val.latency_us || prev.eval_latency_us,
            loop_latency_ms: val.loop_latency_ms || prev.loop_latency_ms,
            latency_us: val.eval_latency_us || val.latency_us || prev.latency_us
          }));
          if (val.latest_sensor) {
            setSensorData(val.latest_sensor);
          }
        }
        if (resEvents.status === 'fulfilled' && Array.isArray(resEvents.value)) {
          setEvents(resEvents.value);
        }
        if (resDevices.status === 'fulfilled' && resDevices.value) {
          setActuatorState(resDevices.value);
        }
      } catch (err) {}
    };

    fetchData();
    const interval = setInterval(fetchData, 800);
    return () => clearInterval(interval);
  }, []);

  return (
    <div className="dashboard-container">
      {/* Header Bar */}
      <Header systemStatus={systemStatus} wsConnected={wsConnected} />

      {/* Real-Time Metrics Overview */}
      <RealtimeMetrics systemStatus={systemStatus} sensorData={sensorData} />

      {/* Main Telemetry & Control Grid */}
      <div className="grid-2-col">
        <LiveSensors sensorData={sensorData} />
        <ActuatorGrid actuatorState={actuatorState} />
      </div>

      {/* Events Log & Diagnostics Grid */}
      <div className="grid-2-col">
        <SafetyEventsLog events={events} />
        <FaultDiagnostics sensorData={sensorData} systemStatus={systemStatus} />
      </div>
    </div>
  );
}
