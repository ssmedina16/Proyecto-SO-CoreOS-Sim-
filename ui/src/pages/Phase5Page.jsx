import { Container, Boxes, ShieldCheck } from 'lucide-react';
import { useSimCtx } from '../context/SimulationContext';
import { CruciblePanel, TransferEventLog } from '../components/phase5/CruciblePanel';
import MetricCard from '../components/common/MetricCard';

export default function Phase5Page() {
  const { crucibles, phase5Events, phase5State, ingots } = useSimCtx();

  const totalLiquidoCrisoles = crucibles.reduce((a, c) => a + c.nivel, 0);
  const vaciados = phase5Events.filter(e => e.tipo === 'vaciado').length;
  const deadlocks = phase5Events.filter(e => e.tipo === 'deadlock').length;
  const totalKgLingotes = (ingots || 0) * 100;

  return (
    <div className="max-w-5xl mx-auto space-y-6">
      <div>
        <div className="flex items-center gap-2">
          <Container className="w-5 h-5 text-molten" />
          <h2 className="text-lg font-bold font-display text-white">
            Fase 5 — Trasiego del Crisol y Fundición de Lingotes
          </h2>
        </div>
        <p className="text-xs text-aluminum-dim mt-1 ml-7">
          Succión del aluminio líquido con exclusión mutua (KMutex) y moldeo en Lingotes de Aluminio (100 kg/lingote)
        </p>
      </div>

      {/* Estado del sistema + Métricas agregadas */}
      <div className="bg-steel rounded-xl border border-surface-line p-5 flex flex-col gap-4">
        <div className="flex items-center justify-between">
          <h3 className="text-sm font-bold font-display text-white uppercase tracking-wider">
            Sistema de trasiego y fundición
          </h3>
          <span className={`text-[10px] font-bold font-mono px-2 py-0.5 rounded-full border ${
            phase5State === 'OPERANDO' ? 'bg-process-green/10 text-process-green border-process-green/30 animate-pulse' :
            phase5State === 'DETENIDO' ? 'bg-graphite text-aluminum-dim border-surface-line' :
            'bg-graphite text-aluminum-dim border-surface-line'
          }`}>{phase5State}</span>
        </div>
        <div className="grid grid-cols-4 gap-4">
          <MetricCard label="Lingotes Moldeados" value={ingots || 0} unit="piezas" accent="molten" icon={Boxes} />
          <MetricCard label="Aluminio Moldeado" value={totalKgLingotes.toFixed(0)} unit="kg" accent="green" />
          <MetricCard label="Aluminio en Crisoles" value={totalLiquidoCrisoles.toFixed(0)} unit="kg" accent="blue" icon={Container} />
          <MetricCard label="Deadlocks prevenidos" value={deadlocks} unit="bloqueos" accent="alarm" icon={ShieldCheck} />
        </div>
      </div>

      {/* Panel de crisoles con barras de nivel */}
      <CruciblePanel crucibles={crucibles} />

      {/* Historial de operaciones */}
      <TransferEventLog events={phase5Events} />
    </div>
  );
}
