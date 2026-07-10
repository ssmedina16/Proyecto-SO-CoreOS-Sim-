import ProgressBar from '../common/ProgressBar';

export function SiloGauge({ level, max = 50000 }) {
  const pct = ((level / max) * 100).toFixed(1);
  return (
    <div className="bg-steel rounded-xl border border-surface-line p-5 flex flex-col gap-4">
      <h3 className="text-sm font-bold font-display text-white uppercase tracking-wider">Silo principal de alúmina</h3>
      <div className="flex items-end justify-between gap-4">
        <div className="flex-1">
          <ProgressBar value={level} max={max} variant="electrolyte" />
        </div>
        <div className="text-right">
          <span className="text-2xl font-extrabold font-mono text-electrolyte">{level.toLocaleString()}</span>
          <span className="text-xs text-aluminum-dim ml-1">/ {max.toLocaleString()} kg</span>
          <p className="text-[10px] text-aluminum-dim mt-0.5">{pct}% capacidad</p>
        </div>
      </div>
    </div>
  );
}

export function TransferLog({ deliveries }) {
  return (
    <div className="bg-steel rounded-xl border border-surface-line p-5 flex flex-col gap-3">
      <h3 className="text-sm font-bold font-display text-white uppercase tracking-wider">Historial de transferencias</h3>
      <div className="bg-foundry rounded-lg border border-surface-line p-3 max-h-64 overflow-y-auto">
        {deliveries.length === 0 ? (
          <p className="text-aluminum-dim/50 italic text-xs text-center py-6">
            Aún no hay transferencias en este ciclo
          </p>
        ) : (
          <table className="w-full text-[11px] font-mono">
            <thead>
              <tr className="text-aluminum-dim border-b border-surface-line">
                <th className="text-left py-1 font-semibold">Tiempo</th>
                <th className="text-left py-1 font-semibold">Destino</th>
                <th className="text-right py-1 font-semibold">Cantidad</th>
              </tr>
            </thead>
            <tbody>
              {deliveries.map((d, i) => (
                <tr key={i} className="border-b border-surface-line/50 last:border-0">
                  <td className="py-1.5 text-aluminum-dim">{d.ts} ms</td>
                  <td className="py-1.5 text-electrolyte">Celda #{d.celda}</td>
                  <td className="py-1.5 text-right text-aluminum">+{d.qty} kg</td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>
    </div>
  );
}
