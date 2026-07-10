/**
 * StatusIndicator — A small PLC-style status light.
 * status: 'active' | 'warning' | 'error' | 'idle'
 */
export default function StatusIndicator({ status = 'idle', size = 'sm' }) {
  const sizeClass = size === 'lg' ? 'w-3 h-3' : 'w-2 h-2';
  const colors = {
    active:  'bg-process-green text-process-green indicator-active',
    warning: 'bg-molten text-molten indicator-active',
    error:   'bg-alarm text-alarm indicator-active',
    idle:    'bg-aluminum-dim text-aluminum-dim',
  };
  return <span className={`inline-block rounded-full ${sizeClass} ${colors[status] || colors.idle}`} />;
}
