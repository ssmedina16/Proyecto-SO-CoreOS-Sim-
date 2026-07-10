import React, { createContext, useContext, useState, useEffect, useRef, useCallback } from 'react';

const SimulationContext = createContext(null);

const INITIAL_CELLS = Array.from({ length: 5 }, (_, i) => ({
  id: i + 1, temp: 958, alumina: 400, carbon: 200, aluminio: 0,
}));

const INITIAL_STATE = {
  running: false,
  connected: false,
  terminal: [],
  // Phase 1
  mixer: 'INACTIVO', oven: 'INACTIVO',
  coke: 500, pitch: 120, anodes: 0,
  phase1Events: [],
  // Phase 2
  silo: 50000, deliveries: [],
  // Phase 3
  cells: INITIAL_CELLS, gases: 0,
  // Phase 4
  gtcState: 'INACTIVO', capturedGas: 0, enrichedAlumina: 0,
  phase4Events: [],
};

export function SimulationProvider({ children }) {
  const [state, setState] = useState(INITIAL_STATE);
  const wsRef = useRef(null);

  // ── WebSocket connection ──────────────────────────────
  useEffect(() => {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.hostname}:5000`;

    const connect = () => {
      const ws = new WebSocket(wsUrl);
      ws.onopen = () => setState(s => ({ ...s, connected: true }));
      ws.onclose = () => {
        setState(s => ({ ...s, connected: false }));
        setTimeout(connect, 3000);
      };
      ws.onerror = () => ws.close();
      ws.onmessage = (e) => handleMessage(JSON.parse(e.data));
      wsRef.current = ws;
    };
    connect();
    return () => wsRef.current?.close();
  }, []);

  // ── Message handler ───────────────────────────────────
  const handleMessage = useCallback((msg) => {
    switch (msg.type) {
      case 'sys':
        setState(s => {
          const next = { ...s, terminal: [...s.terminal, { t: 'sys', text: `[SISTEMA] ${msg.message}` }] };
          if (msg.message.includes('finalizada')) next.running = false;
          return next;
        });
        break;
      case 'stdout':
        setState(s => ({ ...s, terminal: [...s.terminal, { t: 'out', text: msg.message }] }));
        break;
      case 'stderr':
        setState(s => ({ ...s, terminal: [...s.terminal, { t: 'err', text: `[ERROR] ${msg.message}` }] }));
        break;
      case 'csv':
        parseCSV(msg.phase, msg.data);
        break;
    }
  }, []);

  const parseCSV = (phase, raw) => {
    const p = raw.split(',');
    const ts = parseInt(p[0]);

    if (phase === 1) {
      const [, event, , coque, brea, anodos] = p;
      setState(s => {
        const upd = { ...s, coke: +coque, pitch: +brea, anodes: +anodos };
        upd.phase1Events = [{ ts, event, coque: +coque, brea: +brea, anodos: +anodos }, ...s.phase1Events.slice(0, 49)];
        if (event === 'MEZCLADORA_CARGANDO') upd.mixer = 'CARGANDO';
        else if (event === 'MEZCLADORA_LISTO') upd.mixer = 'MASA LISTA';
        else if (event === 'HORNO_COCCION_INICIO') upd.oven = 'HORNEANDO';
        else if (event === 'HORNO_FALTA_RECURSOS') upd.oven = 'SIN RECURSOS';
        else if (event === 'HORNO_ANODO_LISTO') upd.oven = 'ÁNODO LISTO';
        else if (event === 'FINALIZADO') { upd.mixer = 'DETENIDO'; upd.oven = 'DETENIDO'; }
        return upd;
      });
    } else if (phase === 2) {
      const [, event, celda, cantidad, siloRest] = p;
      setState(s => {
        const upd = { ...s, silo: +siloRest };
        if (event === 'RELLENANDO') {
          upd.deliveries = [{ ts, celda: +celda, qty: +cantidad }, ...s.deliveries.slice(0, 49)];
          upd.cells = s.cells.map(c => c.id === +celda ? { ...c, alumina: c.alumina + +cantidad } : c);
        }
        return upd;
      });
    } else if (phase === 3) {
      const [, event, celda, temp, alumina, carbon, aluminio, gases] = p;
      setState(s => {
        const upd = { ...s, gases: +gases };
        if (+celda > 0) {
          upd.cells = s.cells.map(c =>
            c.id === +celda ? { ...c, temp: +temp, alumina: +alumina, carbon: +carbon, aluminio: +aluminio } : c
          );
        }
        return upd;
      });
    } else if (phase === 4) {
      const [, event, gasCap, alEnr] = p;
      setState(s => {
        const upd = { ...s };
        if (event === 'CAPTURA_GASES') {
          upd.gtcState = 'CAPTURANDO';
          upd.capturedGas = s.capturedGas + +gasCap;
          upd.enrichedAlumina = s.enrichedAlumina + +alEnr;
          upd.phase4Events = [{ ts, gasCap: +gasCap, alEnr: +alEnr }, ...s.phase4Events.slice(0, 49)];
        } else if (event === 'INICIADO') upd.gtcState = 'MONITOREANDO';
        else if (event === 'FINALIZADO') upd.gtcState = 'DETENIDO';
        return upd;
      });
    }
  };

  // ── Actions ───────────────────────────────────────────
  const startSimulation = async () => {
    setState(s => ({ ...INITIAL_STATE, connected: s.connected, running: true, terminal: [] }));
    try {
      const res = await fetch('/api/start-simulation', { method: 'POST' });
      if (!res.ok) {
        const err = await res.json();
        alert(`Error: ${err.error}`);
        setState(s => ({ ...s, running: false }));
      }
    } catch {
      alert('No se pudo conectar con el servidor.');
      setState(s => ({ ...s, running: false }));
    }
  };

  const stopSimulation = async () => {
    try {
      await fetch('/api/stop-simulation', { method: 'POST' });
      setState(s => ({ ...s, running: false }));
    } catch {
      alert('Error al detener la simulación.');
    }
  };

  return (
    <SimulationContext.Provider value={{ ...state, startSimulation, stopSimulation }}>
      {children}
    </SimulationContext.Provider>
  );
}

export function useSimCtx() {
  const ctx = useContext(SimulationContext);
  if (!ctx) throw new Error('useSimCtx must be inside SimulationProvider');
  return ctx;
}
