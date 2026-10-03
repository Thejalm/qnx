import React from 'react';
import { 
  Shield, 
  Cpu, 
  Activity, 
  Clock, 
  CheckCircle2, 
  AlertTriangle, 
  Layers, 
  ArrowRight,
  ShieldCheck,
  ShieldAlert,
  Server,
  Radio,
  Sliders,
  Zap
} from 'lucide-react';

export default function AdaptiveSafetyGuardian({ sensorData, systemStatus }) {
  const seq = sensorData?.sequence_number || 1;
  const temp = sensorData?.temperature ?? 26.5;
  const gasAdc = sensorData?.mq2_raw_adc ?? 350;
  const flameDet = sensorData?.flame_detected ?? false;

  // Live Safety Budget breakdown (received live or calculated deterministically)
  const budget = sensorData?.safety_budget || {};
  const deadlineUs = Number(budget.deadline_us || 50.0);
  const tComm = Number(budget.t_comm_us || 1.8);
  const tSched = Number(budget.t_sched_us || 2.4);
  const tProc = Number(budget.t_proc_us || 9.8);
  const tAct = Number(budget.t_act_us || 2.2);
  const actualResponseUs = Number(budget.actual_response_us || (tComm + tSched + tProc + tAct).toFixed(1));
  const safetyMarginUs = Number(budget.safety_margin_us || (deadlineUs - actualResponseUs).toFixed(1));
  const marginRatioPct = Number(budget.margin_ratio_pct || ((safetyMarginUs / deadlineUs) * 100).toFixed(1));

  // Guardian status & protection tier
  const isConstrained = safetyMarginUs < 20.0;
  const isCritical = safetyMarginUs < 12.0;

  const protectionLevel = isCritical ? 'MAX_PROTECTION' : (isConstrained ? 'ADAPTIVE_SHED' : 'OPTIMAL');
  const protectionTag = isCritical ? 'Maximum Guardian Lock' : (isConstrained ? 'Adaptive Workload Shedding' : 'Optimal Margin');
  const badgeClass = isCritical ? 'mc-badge-accent' : (isConstrained ? 'mc-badge-outline' : 'mc-badge-dark');

  const rawCpu = sensorData?.resource_guardian?.cpu_util_pct ?? (14.5 + (temp > 35 ? 6.0 : 0) + (flameDet ? 10.0 : 0));
  const cpuUtil = Number(Number(rawCpu).toFixed(1));
  const throttlePct = isCritical ? 80 : (isConstrained ? 35 : 0);

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: 'var(--space-5)' }}>
      {/* 1. Header Overview & Guardian Status */}
      <section className="mc-card">
        <div className="mc-section-header" style={{ flexWrap: 'wrap' }}>
          <div>
            <div className="mc-section-title">
              <Shield size={22} color="var(--color-2)" />
              <span>Adaptive Safety and Resource Management</span>
            </div>
            <p className="mc-section-subtitle" style={{ marginTop: 'var(--space-1)' }}>
              Continuous real-time supervision of safety response margins, CPU load, and dynamic task protection
            </p>
          </div>

          <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-2)' }}>
            <span className={`mc-badge ${badgeClass}`}>
              {isCritical ? <ShieldAlert size={14} /> : <ShieldCheck size={14} />}
              <span>{protectionTag}</span>
            </span>
            <span className="mc-badge mc-badge-subtle">
              Priority 25 Protected
            </span>
          </div>
        </div>

        {/* Dynamic Margin Formula Bar */}
        <div
          className="mc-card-nested"
          style={{
            display: 'flex',
            justifyContent: 'space-between',
            alignItems: 'center',
            flexWrap: 'wrap',
            gap: 'var(--space-4)',
            marginTop: 'var(--space-2)'
          }}
        >
          <div>
            <span className="mc-stat-label">Safety Response Margin Formulation</span>
            <div className="mc-mono" style={{ fontSize: 'var(--text-base)', fontWeight: 'var(--font-weight-bold)', color: 'var(--color-1)', marginTop: '2px' }}>
              Safety Response Margin = Maximum Deadline ({deadlineUs.toFixed(1)} µs) − Actual Response Time ({actualResponseUs.toFixed(1)} µs)
            </div>
          </div>
          <div style={{ textAlign: 'right' }}>
            <span className="mc-stat-label">Available Margin Headroom</span>
            <div className="mc-mono" style={{ fontSize: 'var(--text-xl)', fontWeight: 'var(--font-weight-bold)', color: 'var(--color-1)' }}>
              {safetyMarginUs.toFixed(1)} µs <span style={{ fontSize: 'var(--text-sm)', color: 'var(--text-secondary)' }}>({marginRatioPct}%)</span>
            </div>
          </div>
        </div>
      </section>

      {/* 2. Key Metrics Grid (3 Focused Cards) */}
      <section className="mc-grid-3">
        {/* Metric 1: Safety Response Margin */}
        <div className="mc-card">
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span className="mc-stat-label">Safety Response Margin</span>
            <ShieldCheck size={18} color="var(--color-2)" />
          </div>
          <div style={{ margin: 'var(--space-3) 0' }}>
            <div style={{ display: 'flex', alignItems: 'baseline', gap: 'var(--space-1)' }}>
              <span className="mc-stat-value mc-mono">{safetyMarginUs}</span>
              <span style={{ fontSize: 'var(--text-sm)', color: 'var(--text-secondary)' }}>µs margin</span>
            </div>
            <div className="mc-meter" style={{ marginTop: 'var(--space-2)' }}>
              <div
                className="mc-meter-fill"
                style={{
                  width: `${marginRatioPct}%`,
                  backgroundColor: marginRatioPct < 30 ? 'var(--color-2)' : 'var(--color-1)'
                }}
              />
            </div>
          </div>
          <p className="mc-section-subtitle">{marginRatioPct}% headroom of 50.0 µs budget</p>
        </div>

        {/* Metric 2: Actual Response Duration */}
        <div className="mc-card">
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span className="mc-stat-label">Actual Response Time</span>
            <Clock size={18} color="var(--color-1)" />
          </div>
          <div style={{ margin: 'var(--space-3) 0' }}>
            <div style={{ display: 'flex', alignItems: 'baseline', gap: 'var(--space-1)' }}>
              <span className="mc-stat-value mc-mono">{actualResponseUs}</span>
              <span style={{ fontSize: 'var(--text-sm)', color: 'var(--text-secondary)' }}>µs total</span>
            </div>
            <div className="mc-meter" style={{ marginTop: 'var(--space-2)' }}>
              <div
                className="mc-meter-fill"
                style={{
                  width: `${(actualResponseUs / deadlineUs) * 100}%`,
                  backgroundColor: 'var(--color-2)'
                }}
              />
            </div>
          </div>
          <p className="mc-section-subtitle">T<sub>comm</sub> + T<sub>sched</sub> + T<sub>proc</sub> + T<sub>act</sub></p>
        </div>

        {/* Metric 3: Resource Guardian & CPU */}
        <div className="mc-card">
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span className="mc-stat-label">Resource Guardian Status</span>
            <Cpu size={18} color="var(--color-1)" />
          </div>
          <div style={{ margin: 'var(--space-3) 0' }}>
            <div style={{ display: 'flex', alignItems: 'baseline', gap: 'var(--space-1)' }}>
              <span className="mc-stat-value mc-mono">{cpuUtil.toFixed(1)}%</span>
              <span style={{ fontSize: 'var(--text-sm)', color: 'var(--text-secondary)' }}>CPU Load</span>
            </div>
            <div className="mc-meter" style={{ marginTop: 'var(--space-2)' }}>
              <div
                className="mc-meter-fill"
                style={{
                  width: `${cpuUtil}%`,
                  backgroundColor: cpuUtil > 75 ? 'var(--color-2)' : 'var(--color-1)'
                }}
              />
            </div>
          </div>
          <p className="mc-section-subtitle">Non-critical throttle: {throttlePct}%</p>
        </div>
      </section>

      {/* 3. Safety Budget Execution Pipeline (Clean Visual Flow) */}
      <section className="mc-card">
        <div className="mc-section-header">
          <div className="mc-section-title">
            <Activity size={20} color="var(--color-2)" />
            <span>Safety Budget Timing Pipeline (Maximum Deadline: {deadlineUs.toFixed(1)} µs)</span>
          </div>
          <span className="mc-badge mc-badge-subtle">
            Execution Stage Breakdown
          </span>
        </div>

        {/* Flowchart Stages Grid */}
        <div className="mc-grid-4">
          {/* Stage 1: Communication */}
          <div className="mc-card-nested">
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <span className="mc-stat-label">1. Communication</span>
              <Radio size={15} color="var(--color-1)" />
            </div>
            <div style={{ margin: 'var(--space-2) 0' }}>
              <span className="mc-stat-value mc-mono" style={{ fontSize: 'var(--text-xl)' }}>{tComm}</span>
              <span style={{ fontSize: 'var(--text-xs)', color: 'var(--text-secondary)', marginLeft: '4px' }}>µs</span>
            </div>
            <p className="mc-section-subtitle">Socket packet ingestion &amp; checksum</p>
          </div>

          {/* Stage 2: Scheduling */}
          <div className="mc-card-nested">
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <span className="mc-stat-label">2. Scheduling</span>
              <Zap size={15} color="var(--color-1)" />
            </div>
            <div style={{ margin: 'var(--space-2) 0' }}>
              <span className="mc-stat-value mc-mono" style={{ fontSize: 'var(--text-xl)' }}>{tSched}</span>
              <span style={{ fontSize: 'var(--text-xs)', color: 'var(--text-secondary)', marginLeft: '4px' }}>µs</span>
            </div>
            <p className="mc-section-subtitle">SCHED_FIFO thread preemption</p>
          </div>

          {/* Stage 3: Processing */}
          <div className="mc-card-nested">
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <span className="mc-stat-label">3. Processing</span>
              <Activity size={15} color="var(--color-2)" />
            </div>
            <div style={{ margin: 'var(--space-2) 0' }}>
              <span className="mc-stat-value mc-mono" style={{ fontSize: 'var(--text-xl)', color: 'var(--color-2)' }}>{tProc}</span>
              <span style={{ fontSize: 'var(--text-xs)', color: 'var(--text-secondary)', marginLeft: '4px' }}>µs</span>
            </div>
            <p className="mc-section-subtitle">Threshold eval &amp; state transitions</p>
          </div>

          {/* Stage 4: Actuation */}
          <div className="mc-card-nested">
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <span className="mc-stat-label">4. Actuation</span>
              <Sliders size={15} color="var(--color-1)" />
            </div>
            <div style={{ margin: 'var(--space-2) 0' }}>
              <span className="mc-stat-value mc-mono" style={{ fontSize: 'var(--text-xl)' }}>{tAct}</span>
              <span style={{ fontSize: 'var(--text-xs)', color: 'var(--text-secondary)', marginLeft: '4px' }}>µs</span>
            </div>
            <p className="mc-section-subtitle">Relay &amp; buzzer command dispatch</p>
          </div>
        </div>
      </section>

      {/* 4. Adaptive Workload Shedding & Live Task Supervisor Table */}
      <section className="mc-card">
        <div className="mc-section-header">
          <div className="mc-section-title">
            <Server size={20} color="var(--color-2)" />
            <span>Resource Guardian Task Supervision and Adaptive Workload Shedding</span>
          </div>
          <span className="mc-badge mc-badge-dark">
            Real-Time Guardian Active
          </span>
        </div>

        <div style={{ overflowX: 'auto' }}>
          <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: 'var(--text-xs)' }}>
            <thead>
              <tr style={{ borderBottom: '1px solid var(--border-subtle)', textAlign: 'left', backgroundColor: 'var(--color-4)' }}>
                <th style={{ padding: 'var(--space-2) var(--space-3)', color: 'var(--text-secondary)' }}>Task Name</th>
                <th style={{ padding: 'var(--space-2) var(--space-3)', color: 'var(--text-secondary)' }}>Category</th>
                <th style={{ padding: 'var(--space-2) var(--space-3)', color: 'var(--color-1)' }}>RTOS Priority</th>
                <th style={{ padding: 'var(--space-2) var(--space-3)', color: 'var(--color-1)' }}>Assigned Core</th>
                <th style={{ padding: 'var(--space-2) var(--space-3)', color: 'var(--color-1)' }}>CPU Budget</th>
                <th style={{ padding: 'var(--space-2) var(--space-3)', color: 'var(--color-2)' }}>Guardian Policy</th>
              </tr>
            </thead>
            <tbody>
              <tr style={{ borderBottom: '1px solid var(--border-subtle)' }}>
                <td style={{ padding: 'var(--space-2) var(--space-3)', fontWeight: 'var(--font-weight-bold)' }}>
                  Safety State Interlocking
                </td>
                <td style={{ padding: 'var(--space-2) var(--space-3)' }}>
                  <span className="mc-badge mc-badge-dark" style={{ fontSize: '11px', padding: '1px 6px' }}>Safety Critical</span>
                </td>
                <td className="mc-mono" style={{ padding: 'var(--space-2) var(--space-3)', fontWeight: 'var(--font-weight-bold)' }}>
                  Priority 25 (SCHED_FIFO)
                </td>
                <td style={{ padding: 'var(--space-2) var(--space-3)' }}>Performance Core 0</td>
                <td className="mc-mono" style={{ padding: 'var(--space-2) var(--space-3)' }}>100% Reserved</td>
                <td style={{ padding: 'var(--space-2) var(--space-3)' }}>
                  <span className="mc-badge mc-badge-dark">Protected (Zero Preemption)</span>
                </td>
              </tr>

              <tr style={{ borderBottom: '1px solid var(--border-subtle)' }}>
                <td style={{ padding: 'var(--space-2) var(--space-3)', fontWeight: 'var(--font-weight-bold)' }}>
                  Telemetry Socket Ingestion
                </td>
                <td style={{ padding: 'var(--space-2) var(--space-3)' }}>
                  <span className="mc-badge mc-badge-subtle" style={{ fontSize: '11px', padding: '1px 6px' }}>I/O Pipeline</span>
                </td>
                <td className="mc-mono" style={{ padding: 'var(--space-2) var(--space-3)' }}>
                  Priority 20 (SCHED_RR)
                </td>
                <td style={{ padding: 'var(--space-2) var(--space-3)' }}>Core 0 / Core 1</td>
                <td className="mc-mono" style={{ padding: 'var(--space-2) var(--space-3)' }}>Guaranteed Cadence</td>
                <td style={{ padding: 'var(--space-2) var(--space-3)' }}>
                  <span className="mc-badge mc-badge-subtle">10 Hz Synchronized</span>
                </td>
              </tr>

              <tr style={{ borderBottom: '1px solid var(--border-subtle)' }}>
                <td style={{ padding: 'var(--space-2) var(--space-3)', fontWeight: 'var(--font-weight-bold)' }}>
                  Event Audit Logger
                </td>
                <td style={{ padding: 'var(--space-2) var(--space-3)' }}>
                  <span className="mc-badge mc-badge-subtle" style={{ fontSize: '11px', padding: '1px 6px' }}>Storage / Audit</span>
                </td>
                <td className="mc-mono" style={{ padding: 'var(--space-2) var(--space-3)' }}>
                  Priority 10 (SCHED_OTHER)
                </td>
                <td style={{ padding: 'var(--space-2) var(--space-3)' }}>Efficiency Core</td>
                <td className="mc-mono" style={{ padding: 'var(--space-2) var(--space-3)' }}>Throttled on Contention</td>
                <td style={{ padding: 'var(--space-2) var(--space-3)' }}>
                  <span className="mc-badge mc-badge-subtle">Background Worker</span>
                </td>
              </tr>

              <tr>
                <td style={{ padding: 'var(--space-2) var(--space-3)', fontWeight: 'var(--font-weight-bold)' }}>
                  Telemetry Analytics Engine
                </td>
                <td style={{ padding: 'var(--space-2) var(--space-3)' }}>
                  <span className="mc-badge mc-badge-subtle" style={{ fontSize: '11px', padding: '1px 6px' }}>Non-Critical</span>
                </td>
                <td className="mc-mono" style={{ padding: 'var(--space-2) var(--space-3)' }}>
                  Priority 5 (SCHED_OTHER)
                </td>
                <td style={{ padding: 'var(--space-2) var(--space-3)' }}>Efficiency Core</td>
                <td className="mc-mono" style={{ padding: 'var(--space-2) var(--space-3)' }}>Dynamic Share (0–60%)</td>
                <td style={{ padding: 'var(--space-2) var(--space-3)' }}>
                  <span className={`mc-badge ${isConstrained ? 'mc-badge-accent' : 'mc-badge-subtle'}`}>
                    {isCritical ? 'Suspended (80% Shed)' : (isConstrained ? 'Throttled (35% Shed)' : 'Normal (0% Shed)')}
                  </span>
                </td>
              </tr>
            </tbody>
          </table>
        </div>
      </section>
    </div>
  );
}
