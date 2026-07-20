import MetricCard from '../common/MetricCard';
import { Wind, Beaker } from 'lucide-react';

export function GasRecoveryPanel({ capturedGas, enrichedAluminaProduced = 0, enrichedAluminaStock = 0, gtcState }) {
  return (
    <div className="bg-steel rounded-xl border border-surface-line p-5 flex flex-col gap-4">
      <div className="flex items-center justify-between">
        <h3 className="text-sm font-bold font-display text-white uppercase tracking-wider">Sistema GTC</h3>
        <span className={`text-[10px] font-bold font-mono px-2 py-0.5 rounded-full border ${gtcState === 'CAPTURANDO' ? 'bg-process-green/10 text-process-green border-process-green/30 animate-pulse' :
            gtcState === 'MONITOREANDO' ? 'bg-molten/10 text-molten border-molten/30' :
              'bg-graphite text-aluminum-dim border-surface-line'
          }`}>{gtcState}</span>
      </div>
      <div className="grid grid-cols-3 gap-4">
        <MetricCard label="Gases capturados" value={capturedGas.toFixed(0)} unit="kg" accent="green" icon={Wind} />
        <MetricCard label="Alúmina Enriquecida Generada" value={enrichedAluminaProduced.toFixed(0)} unit="kg" accent="electrolyte" icon={Beaker} />
        <MetricCard label="Stock Disponible" value={enrichedAluminaStock.toFixed(0)} unit="kg" accent="molten" icon={Beaker} />
      </div>
    </div>
  );
}

export function GtcEventLog({ events }) {
  return (
    <div className="bg-steel rounded-xl border border-surface-line p-5 flex flex-col gap-3">
      <h3 className="text-sm font-bold font-display text-white uppercase tracking-wider">Registro de capturas</h3>
      <div className="bg-foundry rounded-lg border border-surface-line p-3 max-h-48 overflow-y-auto">
        {events.length === 0 ? (
          <p className="text-aluminum-dim/50 italic text-xs text-center py-6">
            Sin capturas de gases en este ciclo
          </p>
        ) : (
          <table className="w-full text-[11px] font-mono">
            <thead>
              <tr className="text-aluminum-dim border-b border-surface-line">
                <th className="text-left py-1 font-semibold">Tiempo</th>
                <th className="text-right py-1 font-semibold">Gases</th>
                <th className="text-right py-1 font-semibold">Al. Enriquecida</th>
              </tr>
            </thead>
            <tbody>
              {events.map((e, i) => (
                <tr key={i} className="border-b border-surface-line/50 last:border-0">
                  <td className="py-1.5 text-aluminum-dim">{e.ts} ms</td>
                  <td className="py-1.5 text-right text-process-green">{e.gasCap} kg</td>
                  <td className="py-1.5 text-right text-electrolyte">+{e.alEnr} kg</td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>
    </div>
  );
}
