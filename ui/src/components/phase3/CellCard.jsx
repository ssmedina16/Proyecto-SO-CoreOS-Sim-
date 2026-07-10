import { AlertTriangle } from 'lucide-react';
import ProgressBar from '../common/ProgressBar';

export function CellCard({ cell, expanded = false }) {
  const lowAlumina = cell.alumina < 200;
  const lowCarbon = cell.carbon < 45;
  const hasWarning = lowAlumina || lowCarbon;

  return (
    <div className={`bg-steel rounded-xl border p-4 flex flex-col gap-3 transition-all duration-200 card-hover ${
      hasWarning ? 'border-alarm/40' : 'border-surface-line'
    }`}>
      {/* Header */}
      <div className="flex items-center justify-between">
        <h4 className="text-xs font-bold font-display text-white">Celda {cell.id}</h4>
        {hasWarning && <AlertTriangle className="w-3.5 h-3.5 text-alarm animate-pulse" />}
      </div>

      {/* Temperature */}
      <div className={`rounded-lg py-2 text-center border ${
        cell.temp > 960 ? 'bg-alarm/10 border-alarm/30 text-alarm' :
        cell.temp < 950 && cell.temp > 0 ? 'bg-molten/10 border-molten/30 text-molten' :
        'bg-process-green/8 border-process-green/20 text-process-green'
      }`}>
        <span className="text-[9px] font-bold uppercase tracking-widest opacity-80">Temperatura</span>
        <p className="text-lg font-extrabold font-mono">{cell.temp} °C</p>
      </div>

      {/* Resources */}
      <ProgressBar label="Alúmina" value={cell.alumina} max={1000} variant={lowAlumina ? 'alarm' : 'electrolyte'} valueLabel={`${cell.alumina.toFixed(0)} kg`} />
      <ProgressBar label="Ánodo carbón" value={cell.carbon} max={200} variant={lowCarbon ? 'alarm' : 'molten'} valueLabel={`${cell.carbon.toFixed(0)} kg`} />

      {/* Output */}
      <div className="flex justify-between items-center pt-2 border-t border-surface-line/60 text-[11px]">
        <span className="text-aluminum-dim font-medium">Aluminio líquido</span>
        <span className="font-mono font-bold text-molten">{cell.aluminio} kg</span>
      </div>

      {expanded && (
        <div className="grid grid-cols-2 gap-2 pt-2 border-t border-surface-line/60 text-[11px]">
          <div className="bg-foundry rounded-lg p-2 border border-surface-line">
            <p className="text-[9px] text-aluminum-dim uppercase">Consumo alúmina</p>
            <p className="font-mono font-bold text-aluminum">200 kg/ciclo</p>
          </div>
          <div className="bg-foundry rounded-lg p-2 border border-surface-line">
            <p className="text-[9px] text-aluminum-dim uppercase">Consumo ánodo</p>
            <p className="font-mono font-bold text-aluminum">45 kg/ciclo</p>
          </div>
        </div>
      )}
    </div>
  );
}

export function CellGrid({ cells, expanded = false }) {
  return (
    <div className={`grid gap-4 ${expanded ? 'grid-cols-1 sm:grid-cols-2 lg:grid-cols-3' : 'grid-cols-2 sm:grid-cols-3 lg:grid-cols-5'}`}>
      {cells.map(cell => <CellCard key={cell.id} cell={cell} expanded={expanded} />)}
    </div>
  );
}
