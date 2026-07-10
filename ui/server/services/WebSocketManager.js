import { WebSocket } from 'ws';

/**
 * WebSocketManager — Manages WS connections and provides a broadcast method.
 * Decoupled from the HTTP server so routes/services can broadcast without
 * importing the raw `wss` instance.
 */
export class WebSocketManager {
  constructor(wss) {
    this.wss = wss;
    this.wss.on('connection', (ws) => {
      console.log('[WS] Client connected');
      ws.send(JSON.stringify({ type: 'sys', message: 'Conectado al servidor de monitorización' }));
    });
  }

  /** Send a JSON payload to every open client. */
  broadcast(data) {
    const payload = JSON.stringify(data);
    this.wss.clients.forEach((client) => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(payload);
      }
    });
  }
}
