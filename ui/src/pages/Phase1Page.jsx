import { Flame } from 'lucide-react';
import { useSimCtx } from '../context/SimulationContext';
import { MixerPanel, OvenPanel } from '../components/phase1/MixerPanel';
import MetricCard from '../components/common/MetricCard';

export default function Phase1Page() {
  const { mixer, oven, coke, pitch, anodes, phase1Events } = useSimCtx();

  return (
    <div className="max-w-5xl mx-auto space-y-6">
      <div>
        <div className="flex items-center gap-2">
          <Flame className="w-5 h-5 text-molten" />
          <h2 className="text-lg font-bold font-display text-white">Fase 1 — Planta de Carbón (Santiago)</h2>
        </div>
        <p className="text-xs text-aluminum-dim mt-1 ml-7">
          Mezcla de coque y brea, cocción de ánodos de carbón para alimentar las celdas de reducción
        </p>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-5">
        <MixerPanel state={mixer} coke={coke} pitch={pitch} />
        <OvenPanel state={oven} anodes={anodes} />
      </div>

      {/* Event Log */}
      <div className="bg-steel rounded-xl border border-surface-line p-5">
        <h3 className="text-sm font-bold font-display text-white uppercase tracking-wider mb-3">Registro de eventos</h3>
        <div className="bg-foundry rounded-lg border border-surface-line p-3 max-h-64 overflow-y-auto">
          {phase1Events.length === 0 ? (
            <p className="text-aluminum-dim/50 italic text-xs text-center py-6">Sin eventos en este ciclo</p>
          ) : (
            <table className="w-full text-[11px] font-mono">
              <thead>
                <tr className="text-aluminum-dim border-b border-surface-line">
                  <th className="text-left py-1 font-semibold">Tiempo</th>
                  <th className="text-left py-1 font-semibold">Evento</th>
                  <th className="text-right py-1 font-semibold">Coque</th>
                  <th className="text-right py-1 font-semibold">Brea</th>
                  <th className="text-right py-1 font-semibold">Ánodos</th>
                </tr>
              </thead>
              <tbody>
                {phase1Events.map((e, i) => (
                  <tr key={i} className="border-b border-surface-line/50 last:border-0">
                    <td className="py-1.5 text-aluminum-dim">{e.ts} ms</td>
                    <td className={`py-1.5 ${
                      e.event.includes('FALTA') ? 'text-alarm' :
                      e.event.includes('LISTO') || e.event.includes('ANODO') ? 'text-process-green' :
                      'text-molten'
                    }`}>{e.event}</td>
                    <td className="py-1.5 text-right text-aluminum">{e.coque} kg</td>
                    <td className="py-1.5 text-right text-aluminum">{e.brea} kg</td>
                    <td className="py-1.5 text-right text-molten font-bold">{e.anodos}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          )}
        </div>
      </div>
    </div>
  );
}
