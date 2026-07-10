/**
 * MetricCard — Displays a numeric metric with label and optional unit.
 * accent: 'molten' | 'electrolyte' | 'green' | 'alarm'
 */
export default function MetricCard({ label, value, unit, accent = 'molten', icon: Icon }) {
  const accents = {
    molten:      'text-molten border-molten/20 bg-molten/5',
    electrolyte: 'text-electrolyte border-electrolyte/20 bg-electrolyte/5',
    green:       'text-process-green border-process-green/20 bg-process-green/5',
    alarm:       'text-alarm border-alarm/20 bg-alarm/5',
  };
  const cls = accents[accent] || accents.molten;

  return (
    <div className={`rounded-xl border p-4 flex flex-col gap-1 ${cls}`}>
      <div className="flex items-center gap-2">
        {Icon && <Icon className="w-3.5 h-3.5 opacity-70" />}
        <span className="text-[11px] font-semibold text-aluminum-dim uppercase tracking-wider">{label}</span>
      </div>
      <div className="flex items-baseline gap-1.5 mt-1">
        <span className="text-xl font-bold font-mono">{typeof value === 'number' ? value.toLocaleString() : value}</span>
        {unit && <span className="text-xs font-medium opacity-60">{unit}</span>}
      </div>
    </div>
  );
}
