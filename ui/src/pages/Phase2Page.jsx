import { Truck } from 'lucide-react';
import { useSimCtx } from '../context/SimulationContext';
import { SiloGauge, TransferLog } from '../components/phase2/SiloGauge';

export default function Phase2Page() {
  const { silo, deliveries, cells } = useSimCtx();

  return (
    <div className="max-w-5xl mx-auto space-y-6">
      <div>
        <div className="flex items-center gap-2">
          <Truck className="w-5 h-5 text-electrolyte" />
          <h2 className="text-lg font-bold font-display text-white">Fase 2 — Logística y Transporte de Alúmina</h2>
        </div>
        <p className="text-xs text-aluminum-dim mt-1 ml-7">
          Distribución neumática de alúmina desde el silo central hacia las tolvas de cada celda de reducción
        </p>
      </div>

      <SiloGauge level={silo} />

      {/* Tolva levels per cell */}
      <div className="bg-steel rounded-xl border border-surface-line p-5">
        <h3 className="text-sm font-bold font-display text-white uppercase tracking-wider mb-3">Nivel de tolvas por celda</h3>
        <div className="grid grid-cols-5 gap-4">
          {cells.map(c => (
            <div key={c.id} className="bg-foundry rounded-lg border border-surface-line p-3 text-center">
              <p className="text-[10px] font-bold text-aluminum-dim mb-2">Celda #{c.id}</p>
              <div className="w-full bg-steel rounded-full h-16 relative overflow-hidden border border-surface-line">
                <div
                  className={`absolute bottom-0 w-full rounded-b-full transition-all duration-500 ${
                    c.alumina < 200 ? 'bg-alarm/50' : 'bg-electrolyte/40'
                  }`}
                  style={{ height: `${Math.min((c.alumina / 1000) * 100, 100)}%` }}
                />
              </div>
              <p className="font-mono font-bold text-electrolyte text-xs mt-2">{c.alumina.toFixed(0)} kg</p>
            </div>
          ))}
        </div>
      </div>

      <TransferLog deliveries={deliveries} />
    </div>
  );
}
