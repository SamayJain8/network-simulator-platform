import React, { useState, useEffect, useRef } from 'react';

function App() {
  const [analysisData, setAnalysisData] = useState({});
  const [systemAlerts, setSystemAlerts] = useState([]);
  const canvasRef = useRef(null);

  useEffect(() => {
    const fetchTelemetry = async () => {
      try {
        const response = await fetch('http://127.0.0.1:8000/telemetry/analysis');
        if (!response.ok) throw new Error('API server unreachable');
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
          setSystemAlerts(prev => [...newAlerts, ...prev].slice(0, 10));
        }
      } catch (err) {
        console.warn('FastAPI Polling warning: ', err.message);
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
    
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    const nodes = {
      RouterA: { x: 150, y: 80, label: 'RouterA', ip: '10.0.1.1' },
      RouterB: { x: 450, y: 80, label: 'RouterB', ip: '10.0.2.1' },
      RouterC: { x: 300, y: 280, label: 'RouterC', ip: '10.0.3.1' }
    };

    const getLinkStyle = (src, dest) => {
      const key1 = `${src}->${dest}`;
      const key2 = `${dest}->${src}`;
      const prediction = analysisData[key1] || analysisData[key2];

      if (prediction) {
        return prediction.is_anomaly ? { color: '#ef4444', dashed: true, width: 4 } : { color: '#10b981', dashed: false, width: 2.5 };
      }
      return { color: '#475569', dashed: false, width: 1.5 };
    };

    const drawLink = (src, dest) => {
      const style = getLinkStyle(src.label, dest.label);
      ctx.beginPath();
      ctx.strokeStyle = style.color;
      ctx.lineWidth = style.width;
      if (style.dashed) {
        ctx.setLineDash([8, 6]);
      } else {
        ctx.setLineDash([]);
      }
      ctx.moveTo(src.x, src.y);
      ctx.lineTo(dest.x, dest.y);
      ctx.stroke();
    };

    drawLink(nodes.RouterA, nodes.RouterB);
    drawLink(nodes.RouterB, nodes.RouterC);
    drawLink(nodes.RouterA, nodes.RouterC);

    Object.values(nodes).forEach(node => {
      ctx.setLineDash([]);
      ctx.beginPath();
      ctx.arc(node.x, node.y, 24, 0, 2 * Math.PI);
      ctx.fillStyle = '#1e293b';
      ctx.strokeStyle = '#38bdf8';
      ctx.lineWidth = 2.5;
      ctx.fill();
      ctx.stroke();

      ctx.fillStyle = '#f1f5f9';
      ctx.font = 'bold 12px sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText(node.label, node.x, node.y + 4);

      ctx.fillStyle = '#94a3b8';
      ctx.font = '10px sans-serif';
      ctx.fillText(node.ip, node.x, node.y + 40);
    });

  }, [analysisData]);

  return (
    <div style={{ padding: '24px', maxWidth: '1200px', margin: '0 auto' }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', borderBottom: '1px solid #334155', paddingBottom: '16px', marginBottom: '24px' }}>
        <div>
          <h1 style={{ fontSize: '24px', fontWeight: 'bold', margin: '0', color: '#38bdf8' }}>Observable Network Simulator Platform</h1>
          <p style={{ margin: '4px 0 0 0', fontSize: '13px', color: '#94a3b8' }}>Real-time telemetry and Holt's Linear Trend predictive anomalies</p>
        </div>
        <div style={{ padding: '6px 12px', borderRadius: '4px', backgroundColor: '#1e293b', border: '1px solid #334155', fontSize: '12px' }}>
          Telemetry Polling Status: <span style={{ color: '#10b981', fontWeight: 'bold' }}>ACTIVE</span>
        </div>
      </div>

      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1.2fr', gap: '24px' }}>
        <div style={{ backgroundColor: '#1e293b', borderRadius: '8px', border: '1px solid #334155', padding: '16px', display: 'flex', flexDirection: 'column', alignItems: 'center' }}>
          <h2 style={{ fontSize: '16px', fontWeight: 'bold', width: '100%', margin: '0 0 16px 0', borderBottom: '1px solid #334155', paddingBottom: '8px' }}>Live Network Topology Map</h2>
          <canvas 
            ref={canvasRef} 
            width={600} 
            height={360} 
            style={{ backgroundColor: '#0f172a', borderRadius: '6px', border: '1px solid #334155' }}
          />
          <div style={{ display: 'flex', gap: '16px', marginTop: '16px', fontSize: '11px', color: '#94a3b8' }}>
            <span style={{ display: 'flex', alignItems: 'center', gap: '6px' }}><span style={{ width: '12px', height: '12px', backgroundColor: '#10b981', borderRadius: '2px' }}></span> Healthy Link</span>
            <span style={{ display: 'flex', alignItems: 'center', gap: '6px' }}><span style={{ width: '12px', height: '12px', border: '2px dashed #ef4444', borderRadius: '2px' }}></span> Congested / Anomaly Link</span>
            <span style={{ display: 'flex', alignItems: 'center', gap: '6px' }}><span style={{ width: '12px', height: '12px', backgroundColor: '#475569', borderRadius: '2px' }}></span> Idle Link</span>
          </div>
        </div>

        <div style={{ display: 'flex', flexDirection: 'column', gap: '20px' }}>
          <div style={{ backgroundColor: '#1e293b', borderRadius: '8px', border: '1px solid #334155', padding: '16px' }}>
            <h2 style={{ fontSize: '16px', fontWeight: 'bold', margin: '0 0 12px 0', borderBottom: '1px solid #334155', paddingBottom: '8px' }}>Active Path Diagnostics</h2>
            {Object.keys(analysisData).length === 0 ? (
              <p style={{ color: '#94a3b8', fontSize: '13px', margin: '0' }}>Waiting for metric streams from C++ core engine...</p>
            ) : (
              <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
                {Object.entries(analysisData).map(([link, info]) => (
                  <div key={link} style={{ display: 'flex', justifyContent: 'space-between', padding: '10px', backgroundColor: '#0f172a', borderRadius: '4px', border: '1px solid #334155', fontSize: '13px' }}>
                    <span style={{ fontWeight: 'bold', color: '#38bdf8' }}>{link}</span>
                    <span>Status: <span style={{ color: info.is_anomaly ? '#ef4444' : '#10b981', fontWeight: 'bold' }}>{info.is_anomaly ? 'ANOMALOUS' : 'HEALTHY'}</span></span>
                    <span>Risk Level: <span style={{ fontWeight: 'bold' }}>{(info.anomaly_probability * 100).toFixed(0)}%</span></span>
                  </div>
                ))}
              </div>
            )}
          </div>

          <div style={{ backgroundColor: '#1e293b', borderRadius: '8px', border: '1px solid #334155', padding: '16px', flexGrow: 1, display: 'flex', flexDirection: 'column' }}>
            <h2 style={{ fontSize: '16px', fontWeight: 'bold', margin: '0 0 12px 0', borderBottom: '1px solid #334155', paddingBottom: '8px' }}>Telemetry AI Alerts & Routing Directives</h2>
            <div style={{ overflowY: 'auto', maxHeight: '200px', flexGrow: 1, padding: '8px', backgroundColor: '#0f172a', borderRadius: '4px', border: '1px solid #334155', display: 'flex', flexDirection: 'column', gap: '8px' }}>
              {systemAlerts.length === 0 ? (
                <p style={{ color: '#94a3b8', fontSize: '12px', margin: '0', textAlign: 'center', paddingTop: '20px' }}>No active anomaly flags detected. Link topology is operating normally.</p>
              ) : (
                systemAlerts.map((alert, idx) => (
                  <div key={idx} style={{ borderBottom: '1px solid #1e293b', paddingBottom: '8px', fontSize: '12px' }}>
                    <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '4px' }}>
                      <span style={{ color: '#ef4444', fontWeight: 'bold' }}>[ANOMALY DETECTED] - {alert.link}</span>
                      <span style={{ color: '#64748b' }}>{alert.time}</span>
                    </div>
                    <div style={{ color: '#f1f5f9', marginBottom: '4px' }}>{alert.message}</div>
                    <div style={{ color: '#38bdf8', fontStyle: 'italic' }}>Directive: {alert.recommendation}</div>
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