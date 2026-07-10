import { Outlet } from 'react-router-dom';
import { Play, Square, Cpu } from 'lucide-react';
import { useSimCtx } from '../context/SimulationContext';
import ProcessPipeline from '../components/navigation/ProcessPipeline';
import KernelConsole from '../components/terminal/KernelConsole';
import StatusIndicator from '../components/common/StatusIndicator';

/**
 * MainLayout — Application shell: header + pipeline nav + page content + console.
 */
export default function MainLayout() {
  const { running, connected, startSimulation, stopSimulation } = useSimCtx();

  return (
    <div className="min-h-screen flex flex-col bg-foundry select-none">
      {/* ── Header ────────────────────────────────────── */}
      <header className="flex items-center justify-between px-5 py-3 border-b border-surface-line bg-steel/40 backdrop-blur-md sticky top-0 z-50">
        <div className="flex items-center gap-3">
          <div className="p-2 rounded-lg bg-molten/10 border border-molten/20">
            <Cpu className="w-5 h-5 text-molten" />
          </div>
          <div>
            <h1 className="text-base font-bold font-display tracking-tight text-white flex items-center gap-2">
              CoreOS-Sim
            </h1>
            <p className="text-[10px] text-aluminum-dim tracking-wide">Simulador multiproceso de producción industrial</p>
          </div>
        </div>

        {/* Connection + Controls */}
        <div className="flex items-center gap-4">
          <div className="flex items-center gap-1.5 text-[11px]">
            <StatusIndicator status={connected ? 'active' : 'error'} />
            <span className="text-aluminum-dim font-medium">{connected ? 'Conectado' : 'Sin conexión'}</span>
          </div>

          {!running ? (
            <button
              onClick={startSimulation}
              disabled={!connected}
              className="flex items-center gap-2 px-4 py-2 bg-molten/15 hover:bg-molten/25 text-molten font-semibold rounded-lg text-xs border border-molten/30 transition-all duration-200 active:scale-95 disabled:opacity-40 disabled:cursor-not-allowed cursor-pointer"
            >
              <Play className="w-3.5 h-3.5 fill-current" />
              <span>Iniciar</span>
            </button>
          ) : (
            <button
              onClick={stopSimulation}
              className="flex items-center gap-2 px-4 py-2 bg-alarm/15 hover:bg-alarm/25 text-alarm font-semibold rounded-lg text-xs border border-alarm/30 transition-all duration-200 active:scale-95 cursor-pointer"
            >
              <Square className="w-3.5 h-3.5 fill-current" />
              <span>Detener</span>
            </button>
          )}
        </div>
      </header>

      {/* ── Pipeline Navigation ───────────────────────── */}
      <div className="border-b border-surface-line bg-steel/20 px-3">
        <ProcessPipeline />
      </div>

      {/* ── Page Content ──────────────────────────────── */}
      <main className="flex-1 p-5 overflow-y-auto">
        <Outlet />
      </main>

      {/* ── Console Footer ────────────────────────────── */}
      <KernelConsole />
    </div>
  );
}
