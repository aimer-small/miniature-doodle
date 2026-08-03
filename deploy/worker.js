// Cloudflare Worker for Shujian MUD
import indexHTML from './index.html';

export default {
  async fetch(request, env, ctx) {
    const url = new URL(request.url);

    // WebSocket upgrade
    if (url.pathname === '/ws') {
      if (request.headers.get('Upgrade') !== 'websocket') {
        return new Response('Expected WebSocket', { status: 426 });
      }
      // Create a WebSocket pair
      const [client, server] = Object.values(new WebSocketPair());
      server.accept();
      
      // Connect to backend MUD server
      const backend = new WebSocket(`ws://${env.MUD_HOST || 'localhost'}:${env.MUD_PORT || '8080'}/ws`);
      
      backend.addEventListener('message', (e) => {
        server.send(e.data);
      });
      
      server.addEventListener('message', (e) => {
        backend.send(e.data);
      });
      
      backend.addEventListener('close', () => {
        server.close();
      });
      
      server.addEventListener('close', () => {
        backend.close();
      });
      
      return new Response(null, { status: 101, webSocket: client });
    }

    // Serve index.html
    return new Response(indexHTML, {
      headers: { 'Content-Type': 'text/html; charset=utf-8' }
    });
  }
};
