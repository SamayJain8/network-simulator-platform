import http from 'node:http';

const port = Number(process.env.PORT || 8000);
const host = process.env.HOST || '127.0.0.1';
let mode = 'idle';
let analysisCache = {};

const payloads = {
  idle: {},
  healthy: {
    'RouterA->RouterB': {
      is_anomaly: false,
      anomaly_probability: 0.1,
      predicted_bottleneck_node: null,
      alert_message: 'Link stability within normal operating parameters.',
      recommended_action: 'Maintain current routing table configuration.'
    },
    'RouterB->RouterC': {
      is_anomaly: false,
      anomaly_probability: 0.2,
      predicted_bottleneck_node: null,
      alert_message: 'Link stability within normal operating parameters.',
      recommended_action: 'Maintain current routing table configuration.'
    },
    'RouterA->RouterC': {
      is_anomaly: false,
      anomaly_probability: 0.1,
      predicted_bottleneck_node: null,
      alert_message: 'Link stability within normal operating parameters.',
      recommended_action: 'Maintain current routing table configuration.'
    }
  },
  anomaly: {
    'RouterA->RouterB': {
      is_anomaly: false,
      anomaly_probability: 0.2,
      predicted_bottleneck_node: null,
      alert_message: 'Link stability within normal operating parameters.',
      recommended_action: 'Maintain current routing table configuration.'
    },
    'RouterB->RouterC': {
      is_anomaly: true,
      anomaly_probability: 0.7,
      predicted_bottleneck_node: 'RouterC',
      alert_message: 'Predicted latency anomaly (428.4 ns) on link RouterB->RouterC within the next 3 intervals.',
      recommended_action: 'Congestion imminent. Set Link Cost for RouterB->RouterC to 999999 to trigger dynamic Dijkstra routing bypass.'
    },
    'RouterA->RouterC': {
      is_anomaly: false,
      anomaly_probability: 0.3,
      predicted_bottleneck_node: null,
      alert_message: 'Link stability within normal operating parameters.',
      recommended_action: 'Maintain current routing table configuration.'
    }
  }
};

const writeJson = (res, statusCode, body) => {
  res.writeHead(statusCode, {
    'Access-Control-Allow-Origin': '*',
    'Access-Control-Allow-Methods': 'GET, OPTIONS',
    'Access-Control-Allow-Headers': 'Content-Type',
    'Content-Type': 'application/json'
  });
  res.end(JSON.stringify(body));
};

const readBody = (req) => new Promise((resolve, reject) => {
  let body = '';
  req.on('data', chunk => {
    body += chunk;
    if (body.length > 1_000_000) {
      req.destroy();
      reject(new Error('Request body too large'));
    }
  });
  req.on('end', () => resolve(body));
  req.on('error', reject);
});

const analyzeTelemetry = (sample) => {
  const isAnomaly = sample.latency_ns > 300 || sample.dropped_packets > 0;
  const anomalyProbability = Math.min(
    1,
    (sample.latency_ns > 300 ? 0.4 : 0) +
      (sample.dropped_packets > 0 ? 0.3 : 0) +
      (sample.throughput_kbps < 700 ? 0.2 : 0)
  );

  return {
    is_anomaly: isAnomaly,
    anomaly_probability: anomalyProbability,
    predicted_bottleneck_node: isAnomaly ? sample.dest_node : null,
    alert_message: isAnomaly
      ? `Predicted latency anomaly (${Number(sample.latency_ns).toFixed(1)} ns) on link ${sample.source_node}->${sample.dest_node} within the next 3 intervals.`
      : 'Link stability within normal operating parameters.',
    recommended_action: isAnomaly
      ? `Congestion imminent. Set Link Cost for ${sample.source_node}->${sample.dest_node} to 999999 to trigger dynamic Dijkstra routing bypass.`
      : 'Maintain current routing table configuration.'
  };
};

const server = http.createServer(async (req, res) => {
  const url = new URL(req.url, `http://${req.headers.host}`);

  if (req.method === 'OPTIONS') {
    writeJson(res, 204, {});
    return;
  }

  if (url.pathname === '/health') {
    writeJson(res, 200, { status: 'healthy', service: 'portfolio-telemetry-stub', mode });
    return;
  }

  if (url.pathname === '/mode') {
    const nextMode = url.searchParams.get('state');
    if (!Object.hasOwn(payloads, nextMode)) {
      writeJson(res, 400, { error: 'Unknown mode', allowed: Object.keys(payloads) });
      return;
    }
    mode = nextMode;
    analysisCache = payloads[mode];
    writeJson(res, 200, { mode });
    return;
  }

  if (url.pathname === '/telemetry/stream' && req.method === 'POST') {
    try {
      const sample = JSON.parse(await readBody(req));
      const link = `${sample.source_node}->${sample.dest_node}`;
      const prediction = analyzeTelemetry(sample);
      analysisCache[link] = prediction;
      console.log(`[portfolio] ingested ${link} latency=${sample.latency_ns} drops=${sample.dropped_packets} anomaly=${prediction.is_anomaly}`);
      writeJson(res, 201, prediction);
    } catch (error) {
      writeJson(res, 400, { error: error.message });
    }
    return;
  }

  if (url.pathname === '/telemetry/analysis') {
    writeJson(res, 200, mode === 'idle' ? analysisCache : payloads[mode]);
    return;
  }

  writeJson(res, 404, { error: 'Not found' });
});

server.listen(port, host, () => {
  analysisCache = payloads[mode];
  console.log(`[portfolio] telemetry stub listening on http://${host}:${port} in ${mode} mode`);
});
