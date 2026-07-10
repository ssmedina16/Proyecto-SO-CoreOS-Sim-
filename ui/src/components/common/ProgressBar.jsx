/**
 * ProgressBar — A themed progress bar with gradient fill.
 * variant: 'molten' | 'electrolyte' | 'green' | 'alarm'
 */
export default function ProgressBar({ value, max, variant = 'electrolyte', label, valueLabel, className = '' }) {
  const pct = Math.min((value / max) * 100, 100);
  const gradients = {
    molten:      'from-molten-dim to-molten',
    electrolyte: 'from-electrolyte-dim to-electrolyte',
    green:       'from-green-700 to-process-green',
    alarm:       'from-alarm-dim to-alarm',
  };
  const isLow = pct < 25;
  const grad = isLow ? gradients.alarm : (gradients[variant] || gradients.electrolyte);

  return (
    <div className={className}>
      {(label || valueLabel) && (
        <div className="flex justify-between text-xs mb-1">
          {label && <span className="text-aluminum-dim font-medium">{label}</span>}
          {valueLabel && <span className="font-mono font-semibold text-aluminum">{valueLabel}</span>}
        </div>
      )}
      <div className="w-full bg-foundry rounded-full h-1.5 overflow-hidden border border-surface-line">
        <div
          className={`bg-gradient-to-r ${grad} h-full rounded-full transition-all duration-500`}
          style={{ width: `${pct}%` }}
        />
      </div>
    </div>
  );
}
