import { Router } from 'express';

/**
 * simulationRouter — REST endpoints for controlling the C++ simulation.
 * Receives the SimulationManager instance via factory function.
 */
export function createSimulationRouter(simManager) {
  const router = Router();

  router.post('/start-simulation', (_req, res) => {
    try {
      simManager.start();
      res.json({ status: 'started' });
    } catch (err) {
      res.status(400).json({ error: err.message });
    }
  });

  router.post('/stop-simulation', (_req, res) => {
    try {
      simManager.stop();
      res.json({ status: 'stopped' });
    } catch (err) {
      res.status(400).json({ error: err.message });
    }
  });

  return router;
}
