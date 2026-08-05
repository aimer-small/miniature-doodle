const http = require('http');
const fs = require('fs');
const net = require('net');
const crypto = require('crypto');

const PORT = 8000;
const MUD_HOST = '127.0.0.1';
const MUD_PORT = 5556;

// Session store: id -> { mud, output:[], waiting: res, ip }
const sessions = new Map();

const html = fs.readFileSync(__dirname + '/www/index.html', 'utf8');

const BUILD_ID = Date.now().toString(36);
function getHtml() {
  return html
    .replace('书剑 v3</span>', '书剑 v3</span><span id="build-ver" style="font-size:9px;color:#555;margin-left:4px;">build:' + BUILD_ID + '</span>')
    .replace('</head>', '<script>console.log("BUILD: ' + BUILD_ID + '");</script></head>');
}

const server = http.createServer((req, res) => {
  const url = new URL(req.url, 'http://localhost');
  console.log('HTTP ' + req.method + ' ' + url.pathname + (url.search || ''));

  if (url.pathname === '/cmd' && req.method === 'POST') {
    handleCmd(req, res);
    return;
  }
  if (url.pathname === '/poll') {
    handlePoll(req, res);
    return;
  }
  if (url.pathname === '/connect') {
    handleConnect(req, res);
    return;
  }

  // Serve HTML (no cache to prevent stale versions in preview)
  res.writeHead(200, {
    'Content-Type': 'text/html; charset=utf-8',
    'Cache-Control': 'no-store, no-cache, must-revalidate, max-age=0',
    'Pragma': 'no-cache',
    'Expires': '0'
  });
  res.end(getHtml());
});

function closeSession(sid) {
  const session = sessions.get(sid);
  if (!session) return;
  session.closed = true;
  if (session.mud) { session.mud.destroy(); }
  if (session.waiting) {
    session.waiting.writeHead(200, { 'Content-Type': 'text/plain' });
    session.waiting.end('__CLOSED__');
    session.waiting = null;
  }
  sessions.delete(sid);
}

function handleConnect(req, res) {
  const clientIp = req.socket.remoteAddress;
  // Limit to 1 active session per IP to prevent MUD connection flood
  let activeCount = 0;
  for (const [oldSid, oldSession] of sessions) {
    if (oldSession.ip === clientIp && !oldSession.closed) {
      activeCount++;
      // If there's already an active session, reuse it instead of creating new
      if (activeCount === 1) {
        console.log('Reusing existing session for IP', clientIp, 'sid:', oldSid.substr(0,6));
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ sid: oldSid }));
        return;
      }
    }
  }

  const sid = crypto.randomBytes(8).toString('hex');
  const session = { mud: null, output: [], waiting: null, closed: false, ip: clientIp, loginState: 0, loginTimer: null };

  const mud = net.createConnection({ host: MUD_HOST, port: MUD_PORT }, () => {
    console.log('MUD connected for session', sid);
  });

  function autoLoginSequence() {
    // Guest account data files were deleted, so guest always goes through new account creation.
    // The flow is: BIG5:N → guest ID → Chinese name → talent → email → gender
    const steps = [
      { delay: 500,  cmd: 'N\r\n',           label: 'BIG5: N' },
      { delay: 1500, cmd: 'guest\r\n',        label: 'ID: guest', afterGuest: true },
      { delay: 3000, cmd: '路人甲\r\n',       label: 'Name: 路人甲', needCreation: true },
      { delay: 4500, cmd: '0\r\n',            label: 'Talent: 0', needCreation: true },
      { delay: 6000, cmd: 'y\r\n',            label: 'Agree talent: y', needCreation: true },
      { delay: 7500, cmd: 'test@test.com\r\n', label: 'Email: test@test.com', needCreation: true },
      { delay: 9000, cmd: 'm\r\n',            label: 'Gender: m', needCreation: true },
    ];
    steps.forEach(s => {
      setTimeout(() => {
        if (session.closed || session.inGame) return;
        if (s.needCreation && !session.needCreation) {
          console.log('Auto-login skip [' + sid.substr(0,6) + ']: ' + s.label + ' (already in game)');
          return;
        }
        console.log('Auto-login step [' + sid.substr(0,6) + ']: ' + s.label);
        mud.write(s.cmd);
      }, s.delay);
    });
  }

  mud.on('data', (data) => {
    if (session.closed) return;
    console.log('MUD data [' + sid.substr(0,6) + '] len=' + data.length + ' hex=' + data.toString('hex').substr(0,120));
    let cleaned = Buffer.alloc(0);
    for (let i = 0; i < data.length; i++) {
      if (data[i] !== 255) {
        cleaned = Buffer.concat([cleaned, Buffer.from([data[i]])]);
        continue;
      }
      // IAC byte found
      i++;
      if (i >= data.length) break;
      if (data[i] === 255) {
        // Escaped IAC: keep one 255
        cleaned = Buffer.concat([cleaned, Buffer.from([255])]);
        continue;
      }
      // IAC SB (250) - sub-negotiation: skip until IAC SE (240)
      if (data[i] === 250) {
        while (i < data.length) {
          i++;
          if (i >= data.length) break;
          if (data[i] === 255 && i + 1 < data.length && data[i + 1] === 240) {
            i++; // skip SE byte
            break;
          }
        }
        continue;
      }
      // 2-byte Telnet commands (no option byte): 240-249
      if (data[i] >= 240 && data[i] <= 249) continue;
      // 3-byte Telnet commands (with option byte): 251-254
      i++; // skip option byte
    }
    if (cleaned.length > 0) {
      const cleanedStr = cleaned.toString('utf-8');
      console.log('MUD cleaned [' + sid.substr(0,6) + ']: ' + cleanedStr.substr(0,200));
      // Start auto-login sequence on first BIG5 prompt
      if (session.loginState === 0 && cleanedStr.includes('Are you using BIG5 font')) {
        session.loginState = 1;
        session.needCreation = true; // Guest data files deleted, always new account
        console.log('Starting auto-login sequence (new account creation)');
        autoLoginSequence();
      }
      // Detect new account creation prompt (guest data files were deleted)
      if (cleanedStr.includes('您的中文名字')) {
        if (!session.needCreation) {
          session.needCreation = true;
          console.log('Detected new account creation for guest');
        } else {
          // Name was rejected, re-send
          console.log('Name rejected, re-sending...');
          setTimeout(() => {
            if (!session.closed) mud.write('路人甲\r\n');
          }, 500);
        }
      }
      // Detect existing account password prompt (shouldn't happen, but handle it)
      if (cleanedStr.includes('请输入密码') || cleanedStr.includes('识别密码')) {
        session.needCreation = false;
        console.log('Detected existing account, need password');
        mud.write('SJGUEST\r\n');
      }
      if (session.waiting) {
        const w = session.waiting;
        session.waiting = null;
        w.writeHead(200, { 'Content-Type': 'text/plain; charset=utf-8', 'Transfer-Encoding': 'chunked' });
        w.end(cleaned);
      } else {
        session.output.push(cleaned);
      }
    }
  });

  mud.on('close', () => {
    console.log('MUD disconnected for session', sid);
    closeSession(sid);
  });

  mud.on('error', (err) => {
    console.log('MUD error:', err.message);
    closeSession(sid);
  });

  session.mud = mud;
  sessions.set(sid, session);

  res.writeHead(200, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({ sid }));
}

function handleCmd(req, res) {
  let body = '';
  req.on('data', chunk => body += chunk);
  req.on('end', () => {
    try {
      const { sid, cmd } = JSON.parse(body);
      const session = sessions.get(sid);
      if (!session || session.closed) {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ ok: false, err: 'session not found' }));
        return;
      }
      session.mud.write(cmd + '\r\n');
      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ ok: true }));
    } catch (e) {
      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ ok: false, err: e.message }));
    }
  });
}

function handlePoll(req, res) {
  const url = new URL(req.url, 'http://localhost');
  const sid = url.searchParams.get('sid');
  const session = sessions.get(sid);

  if (!session || session.closed) {
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end('__CLOSED__');
    return;
  }

  // Return buffered output immediately, or empty string
  if (session.output.length > 0) {
    const all = Buffer.concat(session.output);
    session.output = [];
    res.writeHead(200, { 'Content-Type': 'text/plain; charset=utf-8' });
    res.end(all);
  } else {
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end('');
  }
}

server.listen(PORT, () => {
  console.log('Server running on http://0.0.0.0:' + PORT);
  console.log('MUD bridge to ' + MUD_HOST + ':' + MUD_PORT);
});