import express from 'express';
import http from 'http';
import { WebSocketServer } from 'ws';
import path from 'path';
import fs from 'fs';
import { fileURLToPath } from 'url';

import { WebSocketManager } from './services/WebSocketManager.js';
import { SimulationManager } from './services/SimulationManager.js';
import { createSimulationRouter } from './routes/simulation.js';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const uiRoot = path.resolve(__dirname, '..');
const projectRoot = path.resolve(uiRoot, '..');
const logsDir = path.join(projectRoot, 'logs');

// ── Express app ─────────────────────────────────────────
const app = express();
app.use(express.json());

// Serve built frontend in production
const distDir = path.join(uiRoot, 'dist');
if (fs.existsSync(distDir)) {
  app.use(express.static(distDir));
}

// ── HTTP + WebSocket servers ────────────────────────────
const server = http.createServer(app);
const wss = new WebSocketServer({ server });
const wsManager = new WebSocketManager(wss);

// ── Services ────────────────────────────────────────────
const simManager = new SimulationManager(
  projectRoot,
  logsDir,
  (data) => wsManager.broadcast(data),
);

// ── Routes ──────────────────────────────────────────────
app.use('/api', createSimulationRouter(simManager));

// SPA fallback — serve index.html for any unmatched route
if (fs.existsSync(distDir)) {
  app.get('*', (_req, res) => {
    res.sendFile(path.join(distDir, 'index.html'));
  });
}

export { server };
