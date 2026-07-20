import { useNavigate } from 'react-router-dom';
import { Flame, Truck, Layers, Wind, Container, ArrowRight } from 'lucide-react';
import { useSimCtx } from '../context/SimulationContext';
import MetricCard from '../components/common/MetricCard';
import StatusIndicator from '../components/common/StatusIndicator';
import ProgressBar from '../components/common/ProgressBar';

function PhaseSummary({ title, icon: Icon, status, to, children }) {
  const nav = useNavigate();
  return (
    <div
      onClick={() => nav(to)}
      className="bg-steel rounded-xl border border-surface-line p-5 flex flex-col gap-4 cursor-pointer card-hover group"
    >
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-2.5">
          <StatusIndicator status={status} size="lg" />
          <Icon className="w-4 h-4 text-aluminum" />
          <h3 className="text-sm font-bold font-display text-white">{title}</h3>
        </div>
        <ArrowRight className="w-4 h-4 text-aluminum-dim group-hover:text-molten transition-colors" />
      </div>
      {children}
    </div>
  );
}

export default function DashboardPage() {
  const ctx = useSimCtx();
  const totalAl = ctx.cells.reduce((a, c) => a + c.aluminio, 0);
  const lowCells = ctx.cells.filter(c => c.alumina < 200 || c.carbon < 1).length;
  const totalRecolectado = ctx.crucibles.reduce((a, c) => a + c.nivel, 0);
  const vaciados = ctx.phase5Events.filter(e => e.tipo === 'vaciado').length;

  return (
    <div className="max-w-6xl mx-auto space-y-6">
      <div>
        <h2 className="text-lg font-bold font-display text-white">Estado general del sistema</h2>
        <p className="text-xs text-aluminum-dim mt-0.5">
          Vista resumen de todas las fases del proceso de producción de aluminio
        </p>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-5">
        {/* Phase 1 */}
        <PhaseSummary
          title="Planta de Carbón"
          icon={Flame}
          status={ctx.running && ctx.mixer !== 'DETENIDO' ? (ctx.oven === 'SIN RECURSOS' ? 'warning' : 'active') : 'idle'}
          to="/fase-1"
        >
          <div className="grid grid-cols-3 gap-3">
            <div className="bg-foundry rounded-lg p-3 border border-surface-line text-center">
              <p className="text-[9px] text-aluminum-dim uppercase font-semibold">Coque</p>
              <p className="font-mono font-bold text-molten text-sm">{ctx.coke.toFixed(0)} kg</p>
            </div>
            <div className="bg-foundry rounded-lg p-3 border border-surface-line text-center">
              <p className="text-[9px] text-aluminum-dim uppercase font-semibold">Brea</p>
              <p className="font-mono font-bold text-molten text-sm">{ctx.pitch.toFixed(0)} kg</p>
            </div>
            <div className="bg-foundry rounded-lg p-3 border border-surface-line text-center">
              <p className="text-[9px] text-aluminum-dim uppercase font-semibold">Ánodos</p>
              <p className="font-mono font-bold text-molten text-sm">{ctx.anodes}</p>
            </div>
          </div>
        </PhaseSummary>

        {/* Phase 2 */}
        <PhaseSummary
          title="Logística y Transporte"
          icon={Truck}
          status={ctx.running ? 'active' : 'idle'}
          to="/fase-2"
        >
          <ProgressBar
            label="Silo de alúmina"
            value={ctx.silo}
            max={50000}
            variant="electrolyte"
            valueLabel={`${ctx.silo.toLocaleString()} / 50,000 kg`}
          />
          <p className="text-[11px] text-aluminum-dim">
            {ctx.deliveries.length} transferencias realizadas en este ciclo
          </p>
        </PhaseSummary>

        {/* Phase 3 */}
        <PhaseSummary
          title="Celdas de Reducción"
          icon={Layers}
          status={ctx.running ? (lowCells > 0 ? 'warning' : 'active') : 'idle'}
          to="/fase-3"
        >
          <div className="grid grid-cols-5 gap-2">
            {ctx.cells.map(c => (
              <div key={c.id} className={`rounded-lg p-2 border text-center text-[10px] ${
                (c.alumina < 200 || c.carbon < 45) ? 'border-alarm/40 bg-alarm/5' : 'border-surface-line bg-foundry'
              }`}>
                <p className="font-bold text-aluminum-dim">#{c.id}</p>
                <p className="font-mono font-bold text-electrolyte">{c.aluminio} kg</p>
              </div>
            ))}
          </div>
          <div className="flex items-center justify-between text-[11px]">
            <span className="text-aluminum-dim">Aluminio total producido</span>
            <span className="font-mono font-bold text-molten">{totalAl.toLocaleString()} kg</span>
          </div>
        </PhaseSummary>

        {/* Phase 4 */}
        <PhaseSummary
          title="Reciclaje GTC"
          icon={Wind}
          status={ctx.gtcState === 'CAPTURANDO' ? 'active' : ctx.gtcState === 'MONITOREANDO' ? 'warning' : 'idle'}
          to="/fase-4"
        >
          <div className="grid grid-cols-3 gap-2">
            <div className="bg-foundry rounded-lg p-2.5 border border-surface-line text-center">
              <p className="text-[9px] text-aluminum-dim uppercase font-semibold">Gases Capturados</p>
              <p className="font-mono font-bold text-process-green text-xs">{ctx.capturedGas.toFixed(0)} kg</p>
            </div>
            <div className="bg-foundry rounded-lg p-2.5 border border-surface-line text-center">
              <p className="text-[9px] text-aluminum-dim uppercase font-semibold">Eco Al. Generada</p>
              <p className="font-mono font-bold text-electrolyte text-xs">{(ctx.enrichedAluminaProduced || 0).toFixed(0)} kg</p>
            </div>
            <div className="bg-foundry rounded-lg p-2.5 border border-surface-line text-center bg-molten/5 border-molten/20">
              <p className="text-[9px] text-molten uppercase font-semibold">Stock RAM</p>
              <p className="font-mono font-bold text-molten text-xs">{(ctx.enrichedAluminaStock || 0).toFixed(0)} kg</p>
            </div>
          </div>
        </PhaseSummary>

        {/* Phase 5 */}
        <PhaseSummary
          title="Trasiego del Crisol y Fundición"
          icon={Container}
          status={ctx.phase5State === 'OPERANDO' ? 'active' : 'idle'}
          to="/fase-5"
        >
          <div className="grid grid-cols-3 gap-2">
            {ctx.crucibles.map(cr => (
              <div key={cr.id} className="bg-foundry rounded-lg p-2.5 border border-surface-line text-center">
                <p className="text-[9px] text-aluminum-dim uppercase font-semibold">Crisol #{cr.id}</p>
                <p className="font-mono font-bold text-molten text-xs">{cr.nivel.toFixed(0)} kg</p>
              </div>
            ))}
            <div className="bg-foundry rounded-lg p-2.5 border border-surface-line text-center bg-molten/5 border-molten/20">
              <p className="text-[9px] text-molten uppercase font-semibold">Lingotes</p>
              <p className="font-mono font-bold text-process-green text-xs">{ctx.ingots || 0} pzs</p>
            </div>
          </div>
          <div className="flex items-center justify-between text-[11px]">
            <span className="text-aluminum-dim">Lingotes de Aluminio producidos</span>
            <span className="font-mono font-bold text-process-green">{((ctx.ingots || 0) * 100).toLocaleString()} kg</span>
          </div>
          <p className="text-[11px] text-aluminum-dim">
            {vaciados} vaciado{vaciados !== 1 ? 's' : ''} de celda y {ctx.ingots || 0} lingote{(ctx.ingots || 0) !== 1 ? 's' : ''} moldeado{(ctx.ingots || 0) !== 1 ? 's' : ''}
          </p>
        </PhaseSummary>
      </div>
    </div>
  );
}
