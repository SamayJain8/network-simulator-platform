import React, { useState, useEffect, useRef } from 'react';

// Sleek, Minimalist Industrial Styles
const styles = {
  container: {
    padding: '32px',
    maxWidth: '1200px',
    margin: '0 auto',
    backgroundColor: '#0a0d14', // Deep, clean charcoal base
    minHeight: '95vh',
    color: '#f8fafc',
    fontFamily: '-apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif'
  },
  header: {
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
    borderBottom: '1px solid #1e293b',
    paddingBottom: '20px',
    marginBottom: '32px'
  },
  title: {
    fontSize: '20px',
    fontWeight: '600',
    letterSpacing: '-0.5px',
    margin: '0',
    color: '#f8fafc'
  },
  subtitle: {
    margin: '4px 0 0 0',
    fontSize: '12px',
    color: '#475569',
    fontFamily: 'monospace'
  },
  statusBox: {
    padding: '6px 12px',
    borderRadius: '4px',
    backgroundColor: '#111827',
    border: '1px solid #1e293b',
    fontSize: '11px',
    fontFamily: 'monospace',
    color: '#94a3b8'
  },
  statusText: {
    color: '#10b981',
    fontWeight: '600'
  },
  grid: {
    display: 'grid',
    gridTemplateColumns: '1.2fr 1fr',
    gap: '32px'
  },
  card: {
    backgroundColor: '#0f172a',
    borderRadius: '4px',
    border: '1px solid #1e293b',
    padding: '20px',
    display: 'flex',
    flexDirection: 'column'
  },
  cardHeader: {
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
    borderBottom: '1px solid #1e293b',
    paddingBottom: '12px',
    marginBottom: '20px'
  },
  cardTitle: {
    fontSize: '12px',
    fontWeight: '600',
    fontFamily: 'monospace',
    color: '#94a3b8',
    letterSpacing: '0.5px',
    textTransform: 'uppercase'
  },
  cardStatus: {
    fontSize: '10px',
    fontFamily: 'monospace',
    color: '#64748b'
  },
  canvasContainer: {
    backgroundColor: '#090b11',
    padding: '12px',
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
    gap: '20px',
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
    width: '8px',
    height: '8px',
    borderRadius: '50%',
    display: 'inline-block'
  },
  logsColumn: {
    display: 'flex',
    flexDirection: 'column',
    gap: '32px'
  },
  noData: {
    color: '#475569',
    fontSize: '12px',
    fontFamily: 'monospace',
    margin: '12px 0'
  },
  metricsContainer: {
    display: 'flex',
    flexDirection: 'column',
    gap: '10px'
  },
  metricRow: {
    display: 'flex',
    justifyContent: 'space-between',
    padding: '12px 16px',
    backgroundColor: '#090b11',
    borderRadius: '4px',
    border: '1px solid #1e293b',
    fontSize: '12px',
    fontFamily: 'monospace'
  },
  metricLink: {
    fontWeight: '600',
    color: '#38bdf8'
  },
  metricProbability: {
    color: '#64748b'
  },
  terminalContainer: {
    overflowY: 'auto',
    maxHeight: '220px',
    flexGrow: 1,
    padding: '14px',
    backgroundColor: '#090b11',
    borderRadius: '4px',
    border: '1px solid #1e293b',
    display: 'flex',
    flexDirection: 'column',
    gap: '10px',
    fontFamily: 'monospace'
  },
  terminalIdle: {
    color: '#64748b',
    fontSize: '11px',
    lineHeight: '1.6'
  },
  terminalLog: {
    fontSize: '11px',
    lineHeight: '1.5',
    borderBottom: '1px solid #0f172a',
    paddingBottom: '8px'
  },
  terminalLogHeader: {
    display: 'flex',
    justifyContent: 'space-between',
    marginBottom: '4px'
  },
  terminalLogMsg: {
    color: '#cbd5e1'
  },
  terminalLogAction: {
    color: '#38bdf8',
    marginTop: '2px'
  }
};

function App() {
  const [analysisData, setAnalysisData] = useState({});
  const [systemAlerts, setSystemAlerts] = useState([]);
  const canvasRef = useRef(null);

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

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');

    let animationFrameId;
    let pulseOffset = 0;

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

    const drawGridPoints = () => {
      ctx.fillStyle = '#1e293b';
      const step = 30;
      for (let i = step; i < canvas.width; i += step) {
        for (let j = step; j < canvas.height; j += step) {
          ctx.fillRect(i, j, 1, 1); // Draw subtle, single-pixel grid intersection points
        }
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
          // Idle Link: Muted Dark Blue-Gray
          ctx.strokeStyle = '#27272a';
          ctx.lineWidth = 1.5;
          ctx.stroke();
        } else if (status.anomalous) {
          // Anomalous Link: Solid, sharp crimson red
          ctx.strokeStyle = '#ef4444';
          ctx.lineWidth = 2.5;
          ctx.stroke();

          // Render falling data particles representing dropped packets along the line
          ctx.fillStyle = '#f87171';
          for (let j = 0; j < 2; ++j) {
            const dropOffset = (pulseOffset + j * 50) % 100;
            const t = dropOffset / 100;
            const x = src.x + (dest.x - src.x) * t;
            const y = src.y + (dest.y - src.y) * t;
            ctx.beginPath();
            ctx.arc(x, y, 1.5, 0, 2 * Math.PI);
            ctx.fill();
          }
        } else {
          // Healthy Link: Solid, sharp emerald green
          ctx.strokeStyle = '#10b981';
          ctx.lineWidth = 2.0;
          ctx.stroke();

          // Render a clean, high-velocity data packet pulse
          const t = pulseOffset / 100;
          const px = src.x + (dest.x - src.x) * t;
          const py = src.y + (dest.y - src.y) * t;
          ctx.beginPath();
          ctx.arc(px, py, 3.5, 0, 2 * Math.PI);
          ctx.fillStyle = '#f8fafc'; // Clean white data packet
          ctx.fill();
        }
      });
    };

    const drawNodes = () => {
      Object.values(nodes).forEach(node => {
        // Flat, clean node circle
        ctx.beginPath();
        ctx.arc(node.x, node.y, 20, 0, 2 * Math.PI);
        ctx.fillStyle = '#0f172a';
        ctx.strokeStyle = '#64748b';
        ctx.lineWidth = 1.5;
        ctx.fill();
        ctx.stroke();

        // Node Label
        ctx.fillStyle = '#f1f5f9';
        ctx.font = 'bold 10px monospace';
        ctx.textAlign = 'center';
        ctx.fillText(node.label, node.x, node.y + 3);

        // IP Metadata Subtext
        ctx.fillStyle = '#475569';
        ctx.font = '10px monospace';
        ctx.fillText(node.ip, node.x, node.y + 38);
      });
    };

    const animate = () => {
      ctx.fillStyle = '#090b11'; // High-contrast, clean dark background
      ctx.fillRect(0, 0, canvas.width, canvas.height);

      drawGridPoints();
      drawLinks();
      drawNodes();

      pulseOffset = (pulseOffset + 1.5) % 100; // Increment particle velocity
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
          <h1 style={styles.title}>netsim-cockpit</h1>
          <p style={styles.subtitle}>Unified System Telemetry & Congestion Forecasting</p>
        </div>
        <div style={styles.statusBox}>
          System Status: <span style={styles.statusText}>HEALTHY</span>
        </div>
      </div>

      {/* Main Grid Division Layout */}
      <div style={styles.grid}>

        {/* Left Hand Column: The HTML5 Canvas Monitor */}
        <div style={styles.card}>
          <div style={styles.cardHeader}>
            <span style={styles.cardTitle}>Topology Map Monitor</span>
            <span style={styles.cardStatus}>NATIVE CANVAS RENDER</span>
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
            <span style={styles.legendItem}><span style={{ ...styles.dot, backgroundColor: '#ef4444' }}></span> Anomalous Link</span>
            <span style={styles.legendItem}><span style={{ ...styles.dot, backgroundColor: '#27272a' }}></span> Idle Link</span>
          </div>
        </div>

        {/* Right Hand Column: Terminal Diagnostics Logs */}
        <div style={styles.logsColumn}>

          {/* Diagnostics Panel */}
          <div style={styles.card}>
            <div style={styles.cardHeader}>
              <span style={styles.cardTitle}>Active Subnet Status</span>
              <span style={styles.cardStatus}>REST INGESTION</span>
            </div>
            {Object.keys(analysisData).length === 0 ? (
              <p style={styles.noData}>No active traffic. Ingesting metric streams from C++ core daemon...</p>
            ) : (
              <div style={styles.metricsContainer}>
                {Object.entries(analysisData).map(([link, info]) => (
                  <div key={link} style={styles.metricRow}>
                    <span style={styles.metricLink}>{link}</span>
                    <span style={{ color: info.is_anomaly ? '#ef4444' : '#10b981', fontWeight: '500' }}>
                      {info.is_anomaly ? 'ANOMALOUS' : 'HEALTHY'}
                    </span>
                    <span style={styles.metricProbability}>Risk: {(info.anomaly_probability * 100).toFixed(0)}%</span>
                  </div>
                ))}
              </div>
            )}
          </div>

          {/* Clean monolithic system logs */}
          <div style={{ ...styles.card, flexGrow: 1, display: 'flex', flexDirection: 'column' }}>
            <div style={styles.cardHeader}>
              <span style={styles.cardTitle}>System stdout alerts</span>
              <span style={{ ...styles.cardStatus, color: '#ef4444' }}>ROUTING EVENTS</span>
            </div>
            <div style={styles.terminalContainer}>
              {systemAlerts.length === 0 ? (
                <div style={styles.terminalIdle}>
                  $ tail -f /var/log/netsim_telemetry.log<br />
                  <span style={{ color: '#475569' }}>Watching stream... No congestion alerts reported.</span>
                </div>
              ) : (
                systemAlerts.map((alert, idx) => (
                  <div key={idx} style={styles.terminalLog}>
                    <div style={styles.terminalLogHeader}>
                      <span style={{ color: '#ef4444' }}>[ALERT_ROUTING_CONGESTION]</span>
                      <span style={{ color: '#475569' }}>{alert.time}</span>
                    </div>
                    <div style={styles.terminalLogMsg}>{alert.message}</div>
                    <div style={styles.terminalLogAction}>$ netsim-router --recompute --bypass {alert.link}</div>
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