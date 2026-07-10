import ProgressBar from '../common/ProgressBar';

export function MixerPanel({ state, coke, pitch }) {
  const isActive = state !== 'INACTIVO' && state !== 'DETENIDO';
  return (
    <div className="bg-steel rounded-xl border border-surface-line p-5 flex flex-col gap-4">
      <div className="flex items-center justify-between">
        <h3 className="text-sm font-bold font-display text-white uppercase tracking-wider">Mezcladora</h3>
        <span className={`text-[10px] font-bold font-mono px-2 py-0.5 rounded-full border ${
          isActive ? 'bg-molten/10 text-molten border-molten/30' : 'bg-graphite text-aluminum-dim border-surface-line'
        }`}>{state}</span>
      </div>
      <ProgressBar label="Coque metalúrgico" value={coke} max={500} variant="molten" valueLabel={`${coke.toFixed(0)} / 500 kg`} />
      <ProgressBar label="Brea de alquitrán" value={pitch} max={120} variant="molten" valueLabel={`${pitch.toFixed(0)} / 120 kg`} />
    </div>
  );
}

export function OvenPanel({ state, anodes }) {
  const colorMap = {
    'HORNEANDO': 'text-molten',
    'ÁNODO LISTO': 'text-process-green',
    'SIN RECURSOS': 'text-alarm',
  };
  return (
    <div className="bg-steel rounded-xl border border-surface-line p-5 flex flex-col gap-4">
      <div className="flex items-center justify-between">
        <h3 className="text-sm font-bold font-display text-white uppercase tracking-wider">Horno de cocción</h3>
        <span className={`text-[10px] font-bold font-mono px-2 py-0.5 rounded-full border ${
          state === 'HORNEANDO' ? 'bg-molten/10 text-molten border-molten/30 animate-pulse' :
          state === 'SIN RECURSOS' ? 'bg-alarm/10 text-alarm border-alarm/30' :
          'bg-graphite text-aluminum-dim border-surface-line'
        }`}>{state}</span>
      </div>
      <div className="flex items-center justify-between bg-foundry rounded-lg p-4 border border-surface-line">
        <span className="text-xs text-aluminum-dim font-medium">Ánodos de carbón producidos</span>
        <span className="text-2xl font-extrabold font-mono text-molten">{anodes}</span>
      </div>
    </div>
  );
}
