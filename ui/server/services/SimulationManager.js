import { spawn } from 'child_process';
import fs from 'fs';
import path from 'path';
import { FileTailer } from './FileTailer.js';

// Detectar entorno: Docker / Linux nativo → usa make directamente
// Windows sin Docker                     → usa wsl make (comportamiento original)
const IS_LINUX_OR_DOCKER = process.env.IN_DOCKER === 'true' || process.platform === 'linux';

const CSV_FILES = [
  { filename: 'fase1_planta.csv',     phase: 1 },
  { filename: 'fase2_logistica.csv',  phase: 2 },
  { filename: 'fase3_reduccion.csv',  phase: 3 },
  { filename: 'fase4_gtc.csv',        phase: 4 },
  { filename: 'fase5_crisol.csv',     phase: 5 },
];

/**
 * SimulationManager — Owns the lifecycle of the C++ simulation process
 * and the file tailers that stream CSV data to the frontend.
 */
export class SimulationManager {
  constructor(projectRoot, logsDir, wsBroadcast) {
    this.projectRoot = projectRoot;
    this.logsDir = logsDir;
    this.broadcast = wsBroadcast;
    this.process = null;
    this.tailers = [];
  }

  get isRunning() {
    return this.process !== null;
  }

  /** Start the simulation (natively on Linux/Docker, via WSL2 on Windows). */
  start() {
    if (this.isRunning) {
      throw new Error('La simulación ya está corriendo.');
    }

    // Clean up previous tailers
    this._stopTailers();

    // Ensure logs directory exists
    if (!fs.existsSync(this.logsDir)) {
      fs.mkdirSync(this.logsDir, { recursive: true });
    }

    // Remove stale CSV files so tailers start clean
    for (const { filename } of CSV_FILES) {
      const fp = path.join(this.logsDir, filename);
      try { fs.unlinkSync(fp); } catch { /* not found, ok */ }
    }

    // Seleccionar comando según entorno
    const command = IS_LINUX_OR_DOCKER ? 'make' : 'wsl';
    const args    = IS_LINUX_OR_DOCKER ? ['run'] : ['make', 'run'];
    console.log(`[SimulationManager] Entorno: ${IS_LINUX_OR_DOCKER ? 'Linux/Docker' : 'Windows/WSL2'}`);
    console.log(`[SimulationManager] Arrancando simulación C++: ${command} ${args.join(' ')}`);
    this.broadcast({ type: 'sys', message: 'Compilando y arrancando simulación…' });

    this.process = spawn(command, args, { cwd: this.projectRoot });

    // Setup CSV tailers
    for (const { filename, phase } of CSV_FILES) {
      const tailer = new FileTailer(
        path.join(this.logsDir, filename),
        (line) => this.broadcast({ type: 'csv', phase, data: line }),
      );
      tailer.start();
      this.tailers.push(tailer);
    }

    // Relay stdout
    this.process.stdout.on('data', (chunk) => {
      for (const line of chunk.toString().split('\n')) {
        const trimmed = line.trim();
        if (trimmed) this.broadcast({ type: 'stdout', message: trimmed });
      }
    });

    // Relay stderr
    this.process.stderr.on('data', (chunk) => {
      for (const line of chunk.toString().split('\n')) {
        const trimmed = line.trim();
        if (trimmed) this.broadcast({ type: 'stderr', message: trimmed });
      }
    });

    // Handle process exit
    this.process.on('close', (code) => {
      console.log(`[SimulationManager] Simulation exited with code ${code}`);
      this.broadcast({ type: 'sys', message: `Simulación finalizada (código ${code})` });
      this.process = null;
      setTimeout(() => this._stopTailers(), 1000);
    });
  }

  /** Kill the running simulation (natively on Linux/Docker, via WSL2 on Windows). */
  stop() {
    if (!this.isRunning) {
      throw new Error('No hay simulación en ejecución.');
    }

    console.log('[SimulationManager] Stopping simulation');
    this.broadcast({ type: 'sys', message: 'Deteniendo simulación…' });

    // Seleccionar comando según entorno
    const killerCmd  = IS_LINUX_OR_DOCKER ? 'pkill'                  : 'wsl';
    const killerArgs = IS_LINUX_OR_DOCKER ? ['-f', 'build/main']     : ['pkill', '-f', 'build/main'];
    const killer = spawn(killerCmd, killerArgs);
    killer.on('close', () => {
      if (this.process) {
        this.process.kill();
        this.process = null;
      }
      this._stopTailers();
    });
  }

  _stopTailers() {
    this.tailers.forEach((t) => t.stop());
    this.tailers = [];
  }
}
