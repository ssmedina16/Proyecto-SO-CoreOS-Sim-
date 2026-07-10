import { Wind } from 'lucide-react';
import { useSimCtx } from '../context/SimulationContext';
import { GasRecoveryPanel, GtcEventLog } from '../components/phase4/GasRecoveryPanel';

export default function Phase4Page() {
  const { gtcState, capturedGas, enrichedAlumina, phase4Events, gases } = useSimCtx();

  return (
    <div className="max-w-5xl mx-auto space-y-6">
      <div>
        <div className="flex items-center gap-2">
          <Wind className="w-5 h-5 text-process-green" />
          <h2 className="text-lg font-bold font-display text-white">Fase 4 — Reciclaje y Tratamiento de Gases (GTC)</h2>
        </div>
        <p className="text-xs text-aluminum-dim mt-1 ml-7">
          Captura de gases fluorados, depuración y regeneración de alúmina enriquecida para recircular a las celdas
        </p>
      </div>

      <GasRecoveryPanel capturedGas={capturedGas} enrichedAlumina={enrichedAlumina} gtcState={gtcState} />

      {/* Gases pending capture */}
      <div className="bg-steel rounded-xl border border-surface-line p-5">
        <h3 className="text-sm font-bold font-display text-white uppercase tracking-wider mb-3">Gases pendientes de captura</h3>
        <div className="flex items-center gap-4">
          <div className="flex-1">
            <div className="w-full bg-foundry rounded-full h-3 overflow-hidden border border-surface-line">
              <div
                className={`h-full rounded-full transition-all duration-500 ${
                  gases >= 100 ? 'bg-gradient-to-r from-alarm-dim to-alarm animate-pulse' : 'bg-gradient-to-r from-molten-dim to-molten'
                }`}
                style={{ width: `${Math.min((gases / 200) * 100, 100)}%` }}
              />
            </div>
            <p className="text-[10px] text-aluminum-dim mt-1">El GTC se activa cuando los gases acumulados superan 100 kg</p>
          </div>
          <span className={`text-xl font-extrabold font-mono ${gases >= 100 ? 'text-alarm' : 'text-molten'}`}>
            {gases.toFixed(0)} <span className="text-xs font-normal text-aluminum-dim">kg</span>
          </span>
        </div>
      </div>

      <GtcEventLog events={phase4Events} />
    </div>
  );
}
