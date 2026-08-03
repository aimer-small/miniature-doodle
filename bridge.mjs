// MUD Bridge Server - WebSocket relay between browser clients and FluffOS
import { createServer } from 'http';
import { WebSocket, WebSocketServer } from 'ws';
import { readFileSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const MUD_WS_URL = 'ws://127.0.0.1:8000';
const BRIDGE_PORT = 8080;

// MUD connection state
let mudWs = null;
let outputBuffer = [];
let outputSeq = 0;
const MAX_BUFFER = 5000;
let keepAliveTimer = null;
let sessionEpoch = 0;

// Connected browser clients
const clients = new Set();

// Strip telnet IAC sequences from output data
function stripTelnetIAC(data) {
  const bytes = new Uint8Array(data);
  const out = [];
  let i = 0;
  while (i < bytes.length) {
    if (bytes[i] === 0xFF) {
      i++;
      if (i >= bytes.length) break;
      const cmd = bytes[i];
      i++;
      if (cmd === 0xFA) {
        while (i < bytes.length - 1) {
          if (bytes[i] === 0xFF && bytes[i + 1] === 0xF0) { i += 2; break; }
          i++;
        }
      } else if (cmd >= 0xFB && cmd <= 0xFE) {
        while (i < bytes.length - 1) {
          if (bytes[i] === 0xFF && bytes[i + 1] === 0xF0) { i += 2; break; }
          i++;
        }
      }
    } else {
      out.push(bytes[i]);
      i++;
    }
  }
  return new Uint8Array(out);
}

// Broadcast to all connected browser clients
function broadcastToClients(msg) {
  const data = JSON.stringify(msg);
  for (const ws of clients) {
    if (ws.readyState === WebSocket.OPEN) {
      try { ws.send(data); } catch (e) { /* ignore */ }
    }
  }
}

function connectMUD() {
  if (mudWs) {
    try { mudWs.close(); } catch (e) { /* ignore */ }
    mudWs = null;
  }

  mudWs = new WebSocket(MUD_WS_URL, ['telnet']);
  mudWs.binaryType = 'arraybuffer';

  mudWs.on('open', () => {
    console.log('[Bridge] Connected to MUD server (telnet)');
    outputBuffer = [];
    outputSeq = 0;
    sessionEpoch++;
    broadcastToClients({ type: 'session', epoch: sessionEpoch });
    broadcastToClients({ type: 'status', connected: true });

    if (keepAliveTimer) clearInterval(keepAliveTimer);
    keepAliveTimer = setInterval(() => {
      if (mudWs && mudWs.readyState === WebSocket.OPEN) {
        mudWs.send(new Uint8Array([0xFF, 0xF1])); // IAC NOP
        mudWs.ping();
      }
    }, 15000);
  });

  mudWs.on('message', (data) => {
    let raw;
    if (data instanceof ArrayBuffer) {
      raw = new Uint8Array(data);
    } else if (Buffer.isBuffer(data)) {
      raw = new Uint8Array(data);
    } else {
      raw = new Uint8Array(Buffer.from(String(data), 'binary'));
    }
    const clean = stripTelnetIAC(raw);
    const text = new TextDecoder('utf-8').decode(clean);
    const seq = ++outputSeq;
    const item = { seq, text, time: Date.now() };
    outputBuffer.push(item);
    if (outputBuffer.length > MAX_BUFFER) outputBuffer.shift();

    // Push to all connected clients immediately
    broadcastToClients({ type: 'output', seq, text });
  });

  mudWs.on('close', (code) => {
    console.log('[Bridge] MUD connection closed, code:', code);
    if (keepAliveTimer) { clearInterval(keepAliveTimer); keepAliveTimer = null; }
    broadcastToClients({ type: 'status', connected: false });
    mudWs = null;
    setTimeout(connectMUD, 3000);
  });

  mudWs.on('error', (err) => {
    console.error('[Bridge] MUD error:', err.message);
  });
}

connectMUD();

// ===== HTTP Server (serves HTML + WebSocket upgrade) =====
const server = createServer((req, res) => {
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, OPTIONS');
  if (req.method === 'OPTIONS') {
    res.writeHead(204);
    res.end();
    return;
  }

  const url = new URL(req.url, `http://localhost:${BRIDGE_PORT}`);

  if (req.method === 'GET' && (url.pathname === '/' || url.pathname === '/index.html')) {
    try {
      const html = readFileSync(join(__dirname, 'www', 'index.html'), 'utf-8');
      res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
      res.end(html);
    } catch (e) {
      res.writeHead(500);
      res.end('Error loading page');
    }
    return;
  }

  if (req.method === 'GET' && url.pathname === '/status') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({
      connected: mudWs && mudWs.readyState === WebSocket.OPEN,
      bufferSize: outputBuffer.length,
      lastSeq: outputSeq,
      epoch: sessionEpoch
    }));
    return;
  }

  res.writeHead(404);
  res.end('Not found');
});

// ===== WebSocket Server for browser clients =====
const wss = new WebSocketServer({ server });

wss.on('connection', (ws) => {
  console.log('[Bridge] Client connected');
  clients.add(ws);

  // Send current state
  ws.send(JSON.stringify({
    type: 'session',
    epoch: sessionEpoch,
    connected: mudWs && mudWs.readyState === WebSocket.OPEN,
    history: outputBuffer.slice(-200) // Send recent history
  }));

  ws.on('message', (raw) => {
    let msg;
    try { msg = JSON.parse(raw.toString()); } catch (e) { return; }

    if (msg.type === 'cmd' && msg.cmd) {
      if (mudWs && mudWs.readyState === WebSocket.OPEN) {
        const utf8Cmd = new TextEncoder().encode(msg.cmd + '\r\n');
        mudWs.send(utf8Cmd);
        ws.send(JSON.stringify({ type: 'ack', ok: true }));
      } else {
        ws.send(JSON.stringify({ type: 'ack', ok: false, error: 'MUD not connected, reconnecting...' }));
      }
    }
  });

  ws.on('close', () => {
    console.log('[Bridge] Client disconnected');
    clients.delete(ws);
  });

  ws.on('error', (err) => {
    console.error('[Bridge] Client error:', err.message);
    clients.delete(ws);
  });
});

// Ping clients every 30s to detect dead connections
setInterval(() => {
  for (const ws of clients) {
    if (ws.readyState === WebSocket.OPEN) ws.ping();
  }
}, 30000);

server.listen(BRIDGE_PORT, '0.0.0.0', () => {
  console.log(`[Bridge] HTTP+WebSocket server listening on 0.0.0.0:${BRIDGE_PORT}`);
});