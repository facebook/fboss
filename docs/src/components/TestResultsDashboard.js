/**
 * (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
 */

import React, {useState, useMemo, useRef, useEffect} from 'react';

const VENDOR_NAMES = {
  brcm: 'Broadcom',
  leaba: 'Cisco',
  nvda: 'Chenab',
  chenab: 'Chenab',
};

const TAG_COLORS = {
  Vendor: {bg: 'rgba(99,102,241,0.08)', color: '#4f46e5', border: 'rgba(99,102,241,0.2)'},
  SDK: {bg: 'rgba(37,194,160,0.08)', color: '#059669', border: 'rgba(37,194,160,0.2)'},
  ASIC: {bg: 'rgba(245,158,11,0.08)', color: '#d97706', border: 'rgba(245,158,11,0.2)'},
  Mode: {bg: 'rgba(236,72,153,0.08)', color: '#db2777', border: 'rgba(236,72,153,0.2)'},
  Role: {bg: 'rgba(139,92,246,0.08)', color: '#7c3aed', border: 'rgba(139,92,246,0.2)'},
  Platform: {bg: 'rgba(14,165,233,0.08)', color: '#0284c7', border: 'rgba(14,165,233,0.2)'},
  Component: {bg: 'rgba(168,85,247,0.08)', color: '#9333ea', border: 'rgba(168,85,247,0.2)'},
  'ASIC SDK': {bg: 'rgba(37,194,160,0.08)', color: '#059669', border: 'rgba(37,194,160,0.2)'},
  'PHY SDK': {bg: 'rgba(37,194,160,0.08)', color: '#059669', border: 'rgba(37,194,160,0.2)'},
};

const RING_RADIUS = 23;
const RING_CIRCUMFERENCE = 2 * Math.PI * RING_RADIUS;

function parseConfig(testType, config) {
  const parts = config.split('/');
  const simple = [
    'BSP Tests', 'LED Tests', 'Platform Manager', 'Fan Tests',
    'Sensors Tests', 'Data Corral Tests', 'Firmware Util',
  ];

  if (simple.includes(testType)) {
    return [{label: 'Platform', value: parts[0]}];
  }

  if (testType === 'Agent HW Test' || testType === 'SAI Test') {
    const tags = [];
    if (parts[0]) tags.push({label: 'Vendor', value: VENDOR_NAMES[parts[0]] || parts[0]});
    if (parts[1]) tags.push({label: 'SDK', value: parts[1].replace(/_/g, ' ')});
    if (parts[3]) tags.push({label: 'ASIC', value: parts[3]});
    if (parts[4]) tags.push({label: 'Mode', value: parts[4].replace(/_/g, ' ')});
    if (parts[5]) tags.push({label: 'Role', value: parts[5].replace(/_/g, ' ')});
    return tags;
  }

  if (testType === 'Agent Benchmarks') {
    const tags = [];
    if (parts[0]) tags.push({label: 'Vendor', value: VENDOR_NAMES[parts[0]] || parts[0]});
    if (parts[1]) tags.push({label: 'SDK', value: parts[1].replace(/_/g, ' ')});
    if (parts[2]) tags.push({label: 'ASIC', value: parts[2]});
    if (parts[3]) tags.push({label: 'Mode', value: parts[3].replace(/_/g, ' ')});
    if (parts[4]) tags.push({label: 'Role', value: parts[4].replace(/_/g, ' ')});
    return tags;
  }

  if (testType === 'Link Tests') {
    const tags = [{label: 'Platform', value: parts[0]}];
    if (parts[2]) tags.push({label: 'ASIC SDK', value: parts[2].replace('asicsdk-', '')});
    return tags;
  }

  if (testType === 'Qsfp HW Tests') {
    const tags = [{label: 'Platform', value: parts[0]}];
    if (parts[1]) tags.push({label: 'PHY SDK', value: parts[1].replace('physdk-', '')});
    return tags;
  }

  if (testType === 'FSDB Benchmarks') {
    const tags = [];
    if (parts[0]) tags.push({label: 'Component', value: parts[0]});
    if (parts[1]) tags.push({label: 'Platform', value: parts[1]});
    if (parts[2]) tags.push({label: 'SDK', value: parts[2].replace(/_/g, ' ')});
    if (parts[3]) tags.push({label: 'Vendor', value: VENDOR_NAMES[parts[3]] || parts[3]});
    return tags;
  }

  if (testType === 'Qsfp Benchmarks') {
    const tags = [{label: 'Platform', value: parts[0]}];
    if (parts[1]) tags.push({label: 'PHY SDK', value: parts[1]});
    return tags;
  }

  return [{label: 'Config', value: config}];
}

function RingProgress({pct}) {
  const hasPct = pct !== null && pct !== undefined;
  const offset = hasPct
    ? RING_CIRCUMFERENCE - (pct / 100) * RING_CIRCUMFERENCE
    : RING_CIRCUMFERENCE;
  const strokeColor = !hasPct
    ? 'var(--ifm-color-emphasis-400)'
    : pct >= 95
      ? '#25c2a0'
      : pct >= 80
        ? '#f59e0b'
        : '#ef4444';

  return (
    <div style={{position: 'relative', width: 56, height: 56, flexShrink: 0}}>
      <svg viewBox="0 0 52 52" width={56} height={56} style={{transform: 'rotate(-90deg)'}}>
        <circle
          cx={26} cy={26} r={RING_RADIUS}
          fill="none"
          stroke="var(--ifm-color-emphasis-200)"
          strokeWidth={5}
        />
        <circle
          cx={26} cy={26} r={RING_RADIUS}
          fill="none"
          stroke={strokeColor}
          strokeWidth={5}
          strokeLinecap="round"
          strokeDasharray={RING_CIRCUMFERENCE}
          strokeDashoffset={offset}
          style={{transition: 'stroke-dashoffset 1s ease-out'}}
        />
      </svg>
      <span style={{
        position: 'absolute',
        top: '50%',
        left: '50%',
        transform: 'translate(-50%, -50%)',
        fontSize: '0.78rem',
        fontWeight: 700,
      }}>
        {hasPct ? pct + '%' : '---'}
      </span>
    </div>
  );
}

function TestTypeCard({data, configCount, onClick}) {
  const hasPct = data.pct !== null && data.pct !== undefined;
  const badgeStyle = !hasPct
    ? {background: 'rgba(99,102,241,0.1)', color: 'var(--ifm-color-emphasis-600)'}
    : data.pct >= 95
      ? {background: 'rgba(37,194,160,0.1)', color: '#25c2a0'}
      : {background: 'rgba(245,158,11,0.1)', color: '#f59e0b'};
  const badgeText = !hasPct ? 'BENCHMARK' : data.pct >= 95 ? 'HEALTHY' : 'ATTENTION';

  return (
    <div
      onClick={onClick}
      style={{
        background: 'var(--ifm-background-surface-color)',
        border: '1px solid var(--ifm-color-emphasis-200)',
        borderRadius: 10,
        padding: '1rem',
        display: 'flex',
        alignItems: 'center',
        gap: '1rem',
        cursor: 'pointer',
        transition: 'transform 0.2s, border-color 0.2s',
      }}
      onMouseEnter={e => {
        e.currentTarget.style.transform = 'translateY(-2px)';
        e.currentTarget.style.borderColor = 'var(--ifm-color-primary)';
      }}
      onMouseLeave={e => {
        e.currentTarget.style.transform = 'none';
        e.currentTarget.style.borderColor = 'var(--ifm-color-emphasis-200)';
      }}>
      <RingProgress pct={data.pct} />
      <div>
        <div style={{fontSize: '0.88rem', fontWeight: 600, marginBottom: '0.25rem'}}>
          {data.name}{' '}
          <span style={{
            display: 'inline-block',
            padding: '2px 7px',
            borderRadius: 20,
            fontSize: '0.65rem',
            fontWeight: 600,
            letterSpacing: '0.03em',
            textTransform: 'uppercase',
            verticalAlign: 'middle',
            ...badgeStyle,
          }}>
            {badgeText}
          </span>
        </div>
        <div style={{fontSize: '0.73rem', color: 'var(--ifm-color-emphasis-600)', lineHeight: 1.4}}>
          <span style={{marginRight: '0.6rem'}}>{data.total.toLocaleString()} tests</span>
          <span style={{marginRight: '0.6rem'}}>{data.passed !== null ? data.passed.toLocaleString() : 'N/A'} passed</span>
          <span style={{marginRight: '0.6rem'}}>{data.failed !== null ? data.failed.toLocaleString() : 'N/A'} failed</span>
          <span>{configCount} configs</span>
        </div>
      </div>
    </div>
  );
}

function KpiCard({label, value, warn}) {
  return (
    <div style={{
      textAlign: 'center',
      padding: '1rem 0.8rem',
      borderRadius: 10,
      border: '1px solid var(--ifm-color-emphasis-200)',
      background: 'var(--ifm-background-surface-color)',
      transition: 'transform 0.2s',
    }}>
      <div style={{
        fontSize: '1.5rem',
        fontWeight: 700,
        color: warn ? '#f59e0b' : 'var(--ifm-color-primary)',
        lineHeight: 1.2,
      }}>
        {value}
      </div>
      <div style={{
        fontSize: '0.72rem',
        color: 'var(--ifm-color-emphasis-600)',
        textTransform: 'uppercase',
        letterSpacing: '0.05em',
        marginTop: '0.2rem',
        fontWeight: 500,
      }}>
        {label}
      </div>
    </div>
  );
}

function SectionTitle({children}) {
  return (
    <div style={{
      fontSize: '1rem',
      fontWeight: 600,
      color: 'var(--ifm-color-emphasis-600)',
      textTransform: 'uppercase',
      letterSpacing: '0.06em',
      marginBottom: '1rem',
      paddingBottom: '0.5rem',
      borderBottom: '1px solid var(--ifm-color-emphasis-200)',
    }}>
      {children}
    </div>
  );
}

function ConfigTag({label, value}) {
  const style = TAG_COLORS[label] || TAG_COLORS.Platform;
  return (
    <span style={{
      display: 'inline-block',
      padding: '2px 7px',
      borderRadius: 5,
      fontSize: '0.68rem',
      fontWeight: 600,
      letterSpacing: '0.02em',
      margin: '1px 2px 1px 0',
      whiteSpace: 'nowrap',
      background: style.bg,
      color: style.color,
      border: `1px solid ${style.border}`,
    }}>
      <span style={{
        fontSize: '0.58rem',
        textTransform: 'uppercase',
        letterSpacing: '0.05em',
        opacity: 0.7,
        marginRight: 2,
      }}>
        {label}
      </span>
      {value}
    </span>
  );
}

function FilterButton({label, active, onClick}) {
  return (
    <button
      onClick={onClick}
      style={{
        padding: '5px 12px',
        borderRadius: 20,
        border: `1px solid ${active ? 'var(--ifm-color-primary)' : 'var(--ifm-color-emphasis-200)'}`,
        background: active ? 'var(--ifm-color-primary)' : 'transparent',
        color: active ? '#fff' : 'var(--ifm-color-emphasis-600)',
        fontFamily: 'inherit',
        fontSize: '0.75rem',
        fontWeight: active ? 600 : 400,
        cursor: 'pointer',
        transition: 'all 0.15s',
      }}>
      {label}
    </button>
  );
}

export default function TestResultsDashboard({summary, details}) {
  const [filter, setFilter] = useState('All');
  const chartRef = useRef(null);
  const chartInstanceRef = useRef(null);
  const detailSectionRef = useRef(null);

  const testTypes = useMemo(() => [...new Set(details.map(d => d[0]))], [details]);

  const configCounts = useMemo(() => {
    const counts = {};
    details.forEach(d => { counts[d[0]] = (counts[d[0]] || 0) + 1; });
    return counts;
  }, [details]);

  const filteredDetails = useMemo(() => {
    if (filter === 'All') return details;
    return details.filter(d => d[0] === filter);
  }, [details, filter]);

  const totalTests = summary.reduce((a, s) => a + s.total, 0);
  const totalPassed = summary.reduce((a, s) => a + (s.passed || 0), 0);
  const totalFailed = summary.reduce((a, s) => a + (s.failed || 0), 0);
  const passRate = totalPassed > 0
    ? ((totalPassed / (totalPassed + totalFailed)) * 100).toFixed(1) + '%'
    : 'N/A';

  // Load Chart.js from CDN and render bar chart
  useEffect(() => {
    if (!chartRef.current) return;

    function createChart() {
      if (chartInstanceRef.current) {
        chartInstanceRef.current.destroy();
      }
      const ctx = chartRef.current.getContext('2d');
      chartInstanceRef.current = new window.Chart(ctx, {
        type: 'bar',
        data: {
          labels: summary.map(t => t.name),
          datasets: [{
            label: 'Total Tests',
            data: summary.map(t => t.total),
            backgroundColor: summary.map(t => {
              if (t.pct === null) return 'rgba(99,102,241,0.5)';
              if (t.pct >= 95) return 'rgba(37,194,160,0.5)';
              if (t.pct >= 80) return 'rgba(245,158,11,0.5)';
              return 'rgba(239,68,68,0.5)';
            }),
            borderColor: summary.map(t => {
              if (t.pct === null) return '#6366f1';
              if (t.pct >= 95) return '#25c2a0';
              if (t.pct >= 80) return '#f59e0b';
              return '#ef4444';
            }),
            borderWidth: 1,
            borderRadius: 5,
          }],
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          plugins: {
            legend: {display: false},
          },
          scales: {
            y: {
              type: 'logarithmic',
              grid: {color: 'rgba(0,0,0,0.06)'},
            },
            x: {
              grid: {display: false},
              ticks: {
                font: {size: 10},
                maxRotation: 45,
                minRotation: 30,
              },
            },
          },
        },
      });
    }

    if (window.Chart) {
      createChart();
    } else {
      const script = document.createElement('script');
      script.src = 'https://cdn.jsdelivr.net/npm/chart.js@4';
      script.crossOrigin = 'anonymous';
      script.onload = createChart;
      document.head.appendChild(script);
    }

    return () => {
      if (chartInstanceRef.current) {
        chartInstanceRef.current.destroy();
        chartInstanceRef.current = null;
      }
    };
  }, [summary]);

  function handleFilterClick(type) {
    setFilter(type);
    if (detailSectionRef.current) {
      detailSectionRef.current.scrollIntoView({behavior: 'smooth', block: 'start'});
    }
  }

  return (
    <div>
      {/* Subtitle & Timestamp */}
      <p style={{fontSize: '0.95rem', color: 'var(--ifm-color-emphasis-600)', marginBottom: '0.3rem'}}>
        Facebook Open Switching System &mdash; Continuous Testing Dashboard
      </p>
      <p style={{fontSize: '0.8rem', color: 'var(--ifm-color-emphasis-400)', marginBottom: '2rem'}}>
        Refreshed daily &middot; {details.length} test configs &middot; {summary.length} test types
      </p>

      {/* KPI Cards */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: 'repeat(auto-fit, minmax(140px, 1fr))',
        gap: '0.8rem',
        marginBottom: '2rem',
      }}>
        <KpiCard label="Total Tests" value={totalTests.toLocaleString()} />
        <KpiCard label="Passed" value={totalPassed.toLocaleString()} />
        <KpiCard label="Failed" value={totalFailed.toLocaleString()} warn />
        <KpiCard label="Pass Rate" value={passRate} />
        <KpiCard label="CSV Files" value={details.length.toString()} />
        <KpiCard label="Test Types" value={summary.length.toString()} />
      </div>

      {/* Test Type Health Cards */}
      <SectionTitle>Test Type Health</SectionTitle>
      <div style={{
        display: 'grid',
        gridTemplateColumns: 'repeat(auto-fill, minmax(260px, 1fr))',
        gap: '0.8rem',
        marginBottom: '2.5rem',
      }}>
        {summary.map((s, i) => (
          <TestTypeCard
            key={i}
            data={s}
            configCount={configCounts[s.name] || 0}
            onClick={() => handleFilterClick(s.name)}
          />
        ))}
      </div>

      {/* Chart */}
      <SectionTitle>Test Volume by Type</SectionTitle>
      <div style={{
        background: 'var(--ifm-background-surface-color)',
        border: '1px solid var(--ifm-color-emphasis-200)',
        borderRadius: 10,
        padding: '1.2rem',
        height: 300,
        marginBottom: '2.5rem',
        position: 'relative',
      }}>
        <canvas ref={chartRef} />
      </div>

      {/* Detail Table */}
      <div ref={detailSectionRef}>
        <SectionTitle>Test Config Details</SectionTitle>
        <div style={{display: 'flex', flexWrap: 'wrap', gap: '0.5rem', marginBottom: '0.8rem'}}>
          <FilterButton label="All" active={filter === 'All'} onClick={() => handleFilterClick('All')} />
          {testTypes.map(t => (
            <FilterButton key={t} label={t} active={filter === t} onClick={() => handleFilterClick(t)} />
          ))}
        </div>
        <div style={{
          background: 'var(--ifm-background-surface-color)',
          border: '1px solid var(--ifm-color-emphasis-200)',
          borderRadius: 10,
          overflow: 'hidden',
        }}>
          <table style={{width: '100%', borderCollapse: 'collapse', fontSize: '0.8rem'}}>
            <thead>
              <tr>
                <th style={thStyle}>Test Type</th>
                <th style={thStyle}>Config Breakdown</th>
                <th style={thStyle}>CSV</th>
              </tr>
            </thead>
            <tbody>
              {filteredDetails.map((d, i) => {
                const tags = parseConfig(d[0], d[1]);
                return (
                  <tr key={i} style={{borderBottom: '1px solid var(--ifm-color-emphasis-200)'}}>
                    <td style={{...tdStyle, fontWeight: 500, whiteSpace: 'nowrap'}}>{d[0]}</td>
                    <td style={tdStyle}>
                      <div style={{display: 'flex', flexWrap: 'wrap', alignItems: 'center', gap: 2}}>
                        {tags.map((t, j) => <ConfigTag key={j} label={t.label} value={t.value} />)}
                      </div>
                    </td>
                    <td style={tdStyle}>
                      <a
                        href={`/fboss/test_results/${d[2]}`}
                        style={{color: 'var(--ifm-color-primary)', textDecoration: 'none', fontWeight: 500}}>
                        Download
                      </a>
                    </td>
                  </tr>
                );
              })}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
}

const thStyle = {
  background: 'var(--ifm-background-color)',
  color: 'var(--ifm-color-emphasis-600)',
  fontSize: '0.7rem',
  fontWeight: 600,
  textTransform: 'uppercase',
  letterSpacing: '0.06em',
  padding: '0.7rem 0.8rem',
  textAlign: 'left',
  borderBottom: '1px solid var(--ifm-color-emphasis-200)',
  position: 'sticky',
  top: 0,
  zIndex: 2,
};

const tdStyle = {
  padding: '0.6rem 0.8rem',
  verticalAlign: 'middle',
};
