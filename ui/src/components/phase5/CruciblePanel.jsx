import MetricCard from '../common/MetricCard';
import ProgressBar from '../common/ProgressBar';
import { Container } from 'lucide-react';

/**
 * CruciblePanel — Displays the two crucibles (crisoles) with vertical fill bars
 * showing the current aluminum level, and aggregate metrics.
 */
export function CruciblePanel({ crucibles, capacityMax = 300 }) {
  return (
    <div className="bg-steel rounded-xl border border-surface-line p-5 flex flex-col gap-4">
      <h3 className="text-sm font-bold font-display text-white uppercase tracking-wider">
        Estado de los crisoles
      </h3>
      <div className="grid grid-cols-2 gap-5">
        {crucibles.map(cr => {
          const pct = Math.min((cr.nivel / capacityMax) * 100, 100);
          const isFull = pct >= 90;
          return (
            <div
              key={cr.id}
              className="bg-foundry rounded-xl border border-surface-line p-4 flex flex-col items-center gap-3"
            >
              <div className="flex items-center gap-2">
                <Container className={`w-4 h-4 ${isFull ? 'text-alarm' : 'text-molten'}`} />
                <span className="text-xs font-bold font-display text-white">
                  Crisol #{cr.id}
                </span>
              </div>

              {/* Vertical fill bar */}
              <div className="w-full h-28 bg-steel rounded-lg relative overflow-hidden border border-surface-line">
                <div
                  className={`absolute bottom-0 w-full rounded-b-lg transition-all duration-700 ${
                    isFull
                      ? 'bg-gradient-to-t from-alarm-dim to-alarm animate-pulse'
                      : 'bg-gradient-to-t from-molten-dim to-molten'
                  }`}
                  style={{ height: `${pct}%` }}
                />
                {/* Level label inside bar */}
                <div className="absolute inset-0 flex items-center justify-center">
                  <span className="font-mono font-bold text-white text-sm drop-shadow-lg">
                    {cr.nivel.toFixed(0)} kg
                  </span>
                </div>
              </div>

              <ProgressBar
                value={cr.nivel}
                max={capacityMax}
                variant="molten"
                label="Capacidad"
                valueLabel={`${pct.toFixed(0)}%`}
              />
            </div>
          );
        })}
      </div>
    </div>
  );
}

/**
 * TransferEventLog — Table showing the history of cell drain events and deadlocks.
 */
export function TransferEventLog({ events }) {
  return (
    <div className="bg-steel rounded-xl border border-surface-line p-5 flex flex-col gap-3">
      <h3 className="text-sm font-bold font-display text-white uppercase tracking-wider">
        Historial de vaciados
      </h3>
      <div className="bg-foundry rounded-lg border border-surface-line p-3 max-h-64 overflow-y-auto">
        {events.length === 0 ? (
          <p className="text-aluminum-dim/50 italic text-xs text-center py-6">
            Sin operaciones de trasiego en este ciclo
          </p>
        ) : (
          <table className="w-full text-[11px] font-mono">
            <thead>
              <tr className="text-aluminum-dim border-b border-surface-line">
                <th className="text-left py-1 font-semibold">Tiempo</th>
                <th className="text-left py-1 font-semibold">Evento</th>
                <th className="text-center py-1 font-semibold">Crisol</th>
                <th className="text-center py-1 font-semibold">Celda</th>
                <th className="text-right py-1 font-semibold">Transferido</th>
              </tr>
            </thead>
            <tbody>
              {events.map((e, i) => (
                <tr key={i} className="border-b border-surface-line/50 last:border-0">
                  <td className="py-1.5 text-aluminum-dim">{e.ts} ms</td>
                  <td className={`py-1.5 ${
                    e.tipo === 'deadlock' ? 'text-alarm' :
                    e.tipo === 'moldeo' ? 'text-electrolyte font-bold' :
                    'text-process-green'
                  }`}>
                    {e.tipo === 'deadlock' ? 'DEADLOCK PREVENIDO' :
                     e.tipo === 'moldeo' ? 'MOLDEO DE LINGOTES' : 'VACIADO DE CELDA'}
                  </td>
                  <td className="py-1.5 text-center text-molten font-bold">#{e.crisolId}</td>
                  <td className="py-1.5 text-center text-aluminum font-bold">
                    {e.celdaId > 0 ? `#${e.celdaId}` : 'Fundición'}
                  </td>
                  <td className="py-1.5 text-right text-aluminum">
                    {e.tipo === 'deadlock' ? '—' : `${e.kg.toFixed(0)} kg`}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>
    </div>
  );
}
