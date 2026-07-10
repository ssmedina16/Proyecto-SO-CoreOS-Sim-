import { Layers } from 'lucide-react';
import { useSimCtx } from '../context/SimulationContext';
import { CellGrid } from '../components/phase3/CellCard';
import MetricCard from '../components/common/MetricCard';

export default function Phase3Page() {
  const { cells, gases } = useSimCtx();
  const totalAl = cells.reduce((a, c) => a + c.aluminio, 0);
  const totalAlumina = cells.reduce((a, c) => a + c.alumina, 0);

  return (
    <div className="max-w-6xl mx-auto space-y-6">
      <div>
        <div className="flex items-center gap-2">
          <Layers className="w-5 h-5 text-electrolyte" />
          <h2 className="text-lg font-bold font-display text-white">Fase 3 — Celdas de Reducción Electrolítica</h2>
        </div>
        <p className="text-xs text-aluminum-dim mt-1 ml-7">
          Proceso Hall-Héroult: reducción de alúmina a aluminio líquido mediante electrólisis a 958 °C
        </p>
      </div>

      {/* Aggregate metrics */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
        <MetricCard label="Aluminio total" value={totalAl} unit="kg" accent="molten" />
        <MetricCard label="Gases acumulados" value={gases.toFixed(0)} unit="kg" accent="alarm" />
        <MetricCard label="Alúmina en tolvas" value={totalAlumina.toFixed(0)} unit="kg" accent="electrolyte" />
        <MetricCard label="Celdas activas" value={cells.filter(c => c.alumina >= 200 && c.carbon >= 45).length} unit={`/ ${cells.length}`} accent="green" />
      </div>

      {/* Cell grid — expanded mode for detail page */}
      <CellGrid cells={cells} expanded={true} />
    </div>
  );
}
