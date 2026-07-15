import React, { useState, useEffect, useRef } from 'react';

// Scoped Cockpit Styles
const styles = {
  container: {
    padding: '24px',
    maxWidth: '1280px',
    margin: '0 auto',
    backgroundColor: '#070a13',
    minHeight: '90vh'
  },
  header: {
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
    borderBottom: '1px solid #1e293b',
    paddingBottom: '16px',
    marginBottom: '24px'
  },
  title: {
    fontSize: '26px',
    fontWeight: 'bold',
    fontFamily: 'monospace',
    letterSpacing: '1px',
    margin: '0',
    color: '#00f0ff',
    textShadow: '0 0 10px rgba(0, 240, 255, 0.3)'
  },
  subtitle: {
    margin: '6px 0 0 0',
    fontSize: '12px',
    fontFamily: 'monospace',
    color: '#64748b',
    textTransform: 'uppercase'
  },
  statusBox: {
    padding: '8px 16px',
    borderRadius: '4px',
    backgroundColor: '#121824',
    border: '1px solid #1e293b',
    fontSize: '11px',
    fontFamily: 'monospace',
    color: '#94a3b8'
  },
  statusText: {
    color: '#39ff14',
    fontWeight: 'bold',
    textShadow: '0 0 8px rgba(57, 255, 20, 0.4)'
  },
  grid: {
    display: 'grid',
    gridTemplateColumns: '1.1fr 1fr',
    gap: '24px'
  },
  card: {
    backgroundColor: '#121824',
    borderRadius: '6px',
    border: '1px solid #1e293b',
    padding: '16px',
    display: 'flex',
    flexDirection: 'column'
  },
  cardHeader: {
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
    borderBottom: '1px solid #1e293b',
    paddingBottom: '10px',
    marginBottom: '16px'
  },
  cardTitle: {
    fontSize: '13px',
    fontWeight: 'bold',
    fontFamily: 'monospace',
    color: '#f1f5f9',
    textTransform: 'uppercase'
  },
  cardStatus: {
    fontSize: '10px',
    fontFamily: 'monospace',
    color: '#38bdf8'
  },
  canvasContainer: {
    backgroundColor: '#070a13',
    padding: '8px',
    borderRadius: '4px',
    border: '1px solid #1e293b',
    width: '100%',
    boxSizing: 'border-box'
  },
  canvas: {
    display: 'block',
    width: '100%',
    borderRadius: '2px'
  },
  legendContainer: {
    display: 'flex',
    gap: '16px',
    marginTop: '16px',
    fontSize: '11px',
    fontFamily: 'monospace',
    color: '#64748b'
  },
  legendItem: {
    display: 'flex',
    alignItems: 'center',
    gap: '6px'
  },
  dot: {
    width: '10px',
    height: '10px',
    borderRadius: '50%',
    display: 'inline-block'
  },
  logsColumn: {
    display: 'flex',
    flexDirection: 'column',
    gap: '24px'
  },
  noData: {
    color: '#64748b',
    fontSize: '12px',
    fontFamily: 'monospace',
    margin: '12px 0'
  },
  metricsContainer: {
    display: 'flex',
    flexDirection: 'column',
    gap: '8px'
  },
  metricRow: {
    display: 'flex',
    justifyContent: 'space-between',
    padding: '10px 14px',
    backgroundColor: '#070a13',
    borderRadius: '4px',
    border: '1px solid #1e293b',
    fontSize: '12px',
    fontFamily: 'monospace'
  },
  metricLink: {
    fontWeight: 'bold',
    color: '#00f0ff'
  },
  metricProbability: {
    color: '#94a3b8'
  },
  terminalContainer: {
    overflowY: 'auto',
    maxHeight: '220px',
    flexGrow: 1,
    padding: '12px',
    backgroundColor: '#070a13',
    borderRadius: '4px',
    border: '1px solid #1e293b',
    display: 'flex',
    flexDirection: 'column',
    gap: '12px',
    fontFamily: 'monospace'
  },
  terminalIdle: {
    color: '#39ff14',
    fontSize: '12px',
    lineHeight: '1.6'
  },
  terminalLog: {
    fontSize: '11px',
    lineHeight: '1.5',
    borderBottom: '1px solid #121824',
    paddingBottom: '8px'
  },
  terminalLogHeader: {
    display: 'flex',
    justifyContent: 'space-between',
    marginBottom: '4px'
  },
  terminalLogMsg: {
    color: '#f1f5f9'
  },
  terminalLogAction: {
    color: '#00f0ff',
    fontStyle: 'italic',
    marginTop: '2px'
  }
};

function App() {
  const [analysisData, setAnalysisData] = useState({});
  const [systemAlerts, setSystemAlerts] = useState([]);
  const canvasRef = useRef(null);

  // 1. Telemetry API Polling Hook (Runs once per second)
  useEffect(() => {
    const fetchTelemetry = async () => {
      try {
        const response = await fetch('http://127.0.0.1:8000/telemetry/analysis');
        if (!response.ok) throw new Error('API offline');
        const data = await response.json();
        setAnalysisData(data);

        const newAlerts = [];
        Object.entries(data).forEach(([link, prediction]) => {
          if (prediction.is_anomaly) {
            newAlerts.push({
              time: new Date().toLocaleTimeString(),
              link,
              message: prediction.alert_message,
              recommendation: prediction.recommended_action
            });
          }
        });

        if (newAlerts.length > 0) {
          setSystemAlerts(prev => [...newAlerts, ...prev].slice(0, 15));
        }
      } catch (err) {
        console.warn('Backend API connection offline');
      }
    };

    fetchTelemetry();
    const interval = setInterval(fetchTelemetry, 1000);
    return () => clearInterval(interval);
  }, []);

  // 2. Immediate-Mode 60 FPS Canvas Animation Loop
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');

    let animationFrameId;
    let pulseOffset = 0; // Animates flowing data particles

    const nodes = {
      RouterA: { x: 150, y: 100, label: 'Router A', ip: '10.0.1.1' },
      RouterB: { x: 450, y: 100, label: 'Router B', ip: '10.0.2.1' },
      RouterC: { x: 300, y: 280, label: 'Router C', ip: '10.0.3.1' }
    };

    const getLinkState = (src, dest) => {
      const key1 = `${src}->${dest}`;
      const key2 = `${dest}->${src}`;
      const prediction = analysisData[key1] || analysisData[key2];

      if (prediction) {
        return {
          active: true,
          anomalous: prediction.is_anomaly,
          risk: prediction.anomaly_probability
        };
      }
      return { active: false, anomalous: false, risk: 0.0 };
    };

    const drawGrid = () => {
      ctx.strokeStyle = '#1e293b';
      ctx.lineWidth = 0.5;
      const step = 30;
      for (let i = 0; i < canvas.width; i += step) {
        ctx.beginPath();
        ctx.moveTo(i, 0);
        ctx.lineTo(i, canvas.height);
        ctx.stroke();
      }
      for (let j = 0; j < canvas.height; j += step) {
        ctx.beginPath();
        ctx.moveTo(0, j);
        ctx.lineTo(canvas.width, j);
        ctx.stroke();
      }
    };

    const drawLinks = () => {
      const connections = [
        [nodes.RouterA, nodes.RouterB],
        [nodes.RouterB, nodes.RouterC],
        [nodes.RouterA, nodes.RouterC]
      ];

      connections.forEach(([src, dest]) => {
        const status = getLinkState(src.label.replace(' ', ''), dest.label.replace(' ', ''));

        ctx.beginPath();
        ctx.moveTo(src.x, src.y);
        ctx.lineTo(dest.x, dest.y);

        if (!status.active) {
          // Idle Link: Dark Slate
          ctx.strokeStyle = '#334155';
          ctx.lineWidth = 1.5;
          ctx.setLineDash([]);
          ctx.stroke();
        } else if (status.anomalous) {
          // Anomalous Link: Flickering Neon Red
          const flicker = Math.random() > 0.15 ? '#ef4444' : '#7f1d1d';
          ctx.strokeStyle = flicker;
          ctx.lineWidth = 3.5;
          ctx.setLineDash([8, 5]);
          ctx.shadowColor = '#ef4444';
          ctx.shadowBlur = 12;
          ctx.stroke();
          ctx.shadowBlur = 0; // Reset shadow

          // Animate irregular, falling packet drops along the path
          ctx.fillStyle = '#fca5a5';
          for (let j = 0; j < 3; ++j) {
            const dropOffset = (pulseOffset + j * 33) % 100;
            const t = dropOffset / 100;
            const x = src.x + (dest.x - src.x) * t;
            const y = src.y + (dest.y - src.y) * t;
            ctx.beginPath();
            ctx.arc(x + (Math.random() - 0.5) * 8, y + (Math.random() - 0.5) * 8, 2, 0, 2 * Math.PI);
            ctx.fill();
          }
        } else {
          // Healthy Link: Neon Green with smooth packet flow
          ctx.strokeStyle = '#10b981';
          ctx.lineWidth = 2.5;
          ctx.setLineDash([]);
          ctx.shadowColor = '#10b981';
          ctx.shadowBlur = 8;
          ctx.stroke();
          ctx.shadowBlur = 0; // Reset shadow

          // Render active flowing packet pulse
          const t = pulseOffset / 100;
          const px = src.x + (dest.x - src.x) * t;
          const py = src.y + (dest.y - src.y) * t;
          ctx.beginPath();
          ctx.arc(px, py, 5, 0, 2 * Math.PI);
          ctx.fillStyle = '#a7f3d0';
          ctx.shadowColor = '#10b981';
          ctx.shadowBlur = 15;
          ctx.fill();
          ctx.shadowBlur = 0; // Reset shadow
        }
      });
    };

    const drawNodes = () => {
      Object.values(nodes).forEach(node => {
        // Outer glowing ring
        ctx.beginPath();
        ctx.arc(node.x, node.y, 28, 0, 2 * Math.PI);
        ctx.strokeStyle = 'rgba(56, 189, 248, 0.2)';
        ctx.lineWidth = 3;
        ctx.stroke();

        // Inner solid core
        ctx.beginPath();
        ctx.arc(node.x, node.y, 22, 0, 2 * Math.PI);
        ctx.fillStyle = '#0f172a';
        ctx.strokeStyle = '#38bdf8';
        ctx.lineWidth = 2.5;
        ctx.shadowColor = '#38bdf8';
        ctx.shadowBlur = 8;
        ctx.fill();
        ctx.stroke();
        ctx.shadowBlur = 0;

        // Node Label
        ctx.fillStyle = '#f1f5f9';
        ctx.font = 'bold 11px monospace';
        ctx.textAlign = 'center';
        ctx.fillText(node.label, node.x, node.y + 4);

        // IP Metadata Subtext
        ctx.fillStyle = '#64748b';
        ctx.font = '10px monospace';
        ctx.fillText(node.ip, node.x, node.y + 45);
      });
    };

    // The primary 60FPS animation loop
    const animate = () => {
      ctx.fillStyle = '#0b0f19'; // Deep slate cockpit background
      ctx.fillRect(0, 0, canvas.width, canvas.height);

      drawGrid();
      drawLinks();
      drawNodes();

      pulseOffset = (pulseOffset + 1.2) % 100; // Increment particle velocity
      animationFrameId = requestAnimationFrame(animate);
    };

    animate();

    return () => cancelAnimationFrame(animationFrameId);
  }, [analysisData]);

  return (
    <div style={styles.container}>
      {/* Upper Metrics Ticker */}
      <div style={styles.header}>
        <div>
          <h1 style={styles.title}>Netsim Telemetry Cockpit</h1>
          <p style={styles.subtitle}>Asynchronous Real-Time Observability Portal</p>
        </div>
        <div style={styles.statusBox}>
          Ecosystem Status: <span style={styles.statusText}>OPERATIONAL</span>
        </div>
      </div>

      {/* Main Grid Division Layout */}
      <div style={styles.grid}>

        {/* Left Hand Column: The HTML5 Canvas Monitor */}
        <div style={styles.card}>
          <div style={styles.cardHeader}>
            <span style={styles.cardTitle}>Live Link-State Radar</span>
            <span style={styles.cardStatus}>60 FPS GRAPHICS</span>
          </div>
          <div style={styles.canvasContainer}>
            <canvas
              ref={canvasRef}
              width={600}
              height={360}
              style={styles.canvas}
            />
          </div>
          <div style={styles.legendContainer}>
            <span style={styles.legendItem}><span style={{ ...styles.dot, backgroundColor: '#10b981' }}></span> Active Route</span>
            <span style={styles.legendItem}><span style={{ ...styles.dot, border: '2px dashed #ef4444' }}></span> Congested / Tripped Link</span>
            <span style={styles.legendItem}><span style={{ ...styles.dot, backgroundColor: '#334155' }}></span> Idle / Unused Link</span>
          </div>
        </div>

        {/* Right Hand Column: Terminal Diagnostics Logs */}
        <div style={styles.logsColumn}>

          {/* Diagnostics Panel */}
          <div style={styles.card}>
            <div style={styles.cardHeader}>
              <span style={styles.cardTitle}>Active Path Metrics</span>
              <span style={styles.cardStatus}>API INGESTION</span>
            </div>
            {Object.keys(analysisData).length === 0 ? (
              <p style={styles.noData}>Awaiting packet metrics stream from C++ core daemon...</p>
            ) : (
              <div style={styles.metricsContainer}>
                {Object.entries(analysisData).map(([link, info]) => (
                  <div key={link} style={styles.metricRow}>
                    <span style={styles.metricLink}>{link}</span>
                    <span style={{ color: info.is_anomaly ? '#ef4444' : '#10b981', fontWeight: 'bold' }}>
                      {info.is_anomaly ? 'ANOMALOUS' : 'HEALTHY'}
                    </span>
                    <span style={styles.metricProbability}>Risk: {(info.anomaly_probability * 100).toFixed(0)}%</span>
                  </div>
                ))}
              </div>
            )}
          </div>

          {/* Linux Terminal Emulator Alert Logs */}
          <div style={{ ...styles.card, flexGrow: 1, display: 'flex', flexDirection: 'column' }}>
            <div style={styles.cardHeader}>
              <span style={styles.cardTitle}>samay@netsim-api:~# tail -f alerts.log</span>
              <span style={{ ...styles.cardStatus, color: '#ef4444' }}>LIVE TELEMETRY ALERT</span>
            </div>
            <div style={styles.terminalContainer}>
              {systemAlerts.length === 0 ? (
                <div style={styles.terminalIdle}>
                  samay@netsim-api:~# _<br />
                  <span style={{ color: '#64748b' }}>System telemetry within safe bounds. No warnings recorded.</span>
                </div>
              ) : (
                systemAlerts.map((alert, idx) => (
                  <div key={idx} style={styles.terminalLog}>
                    <div style={styles.terminalLogHeader}>
                      <span style={{ color: '#ff073a' }}>[CRITICAL CONGESTION]</span>
                      <span style={{ color: '#64748b' }}>{alert.time}</span>
                    </div>
                    <div style={styles.terminalLogMsg}>Source: {alert.message}</div>
                    <div style={styles.terminalLogAction}>netsim-daemon --set-cost {alert.link} 999999</div>
                  </div>
                ))
              )}
            </div>
          </div>

        </div>

      </div>
    </div>
  );
}

export default App;