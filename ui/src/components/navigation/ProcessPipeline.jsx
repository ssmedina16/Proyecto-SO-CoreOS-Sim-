import { NavLink } from 'react-router-dom';
import { LayoutDashboard, Flame, Truck, Layers, Wind, Container } from 'lucide-react';
import { useSimCtx } from '../../context/SimulationContext';
import StatusIndicator from '../common/StatusIndicator';

const tabs = [
  { to: '/',        label: 'Dashboard', icon: LayoutDashboard, phaseKey: null },
  { to: '/fase-1',  label: 'Carbón',    icon: Flame,           phaseKey: 'p1' },
  { to: '/fase-2',  label: 'Logística',  icon: Truck,           phaseKey: 'p2' },
  { to: '/fase-3',  label: 'Celdas',     icon: Layers,          phaseKey: 'p3' },
  { to: '/fase-4',  label: 'GTC',        icon: Wind,            phaseKey: 'p4' },
  { to: '/fase-5',  label: 'Crisol',     icon: Container,       phaseKey: 'p5' },
];

function getPhaseStatus(key, ctx) {
  if (!ctx.running) return 'idle';
  switch (key) {
    case 'p1':
      if (ctx.oven === 'SIN RECURSOS') return 'warning';
      if (ctx.mixer === 'DETENIDO') return 'idle';
      return 'active';
    case 'p2': return ctx.running ? 'active' : 'idle';
    case 'p3': {
      const lowRes = ctx.cells.some(c => c.alumina < 200 || c.carbon < 45);
      return lowRes ? 'warning' : 'active';
    }
    case 'p4':
      if (ctx.gtcState === 'CAPTURANDO') return 'active';
      if (ctx.gtcState === 'MONITOREANDO') return 'warning';
      return 'idle';
    case 'p5':
      if (ctx.phase5State === 'OPERANDO') return 'active';
      if (ctx.phase5State === 'DETENIDO') return 'idle';
      return 'idle';
    default: return ctx.running ? 'active' : 'idle';
  }
}

/**
 * ProcessPipeline — Industrial navigation bar showing the material flow.
 * Each tab is a phase in the production pipeline, connected by flow lines.
 */
export default function ProcessPipeline() {
  const ctx = useSimCtx();

  return (
    <nav className="flex items-center gap-0 overflow-x-auto px-2 py-1">
      {tabs.map((tab, i) => {
        const status = tab.phaseKey ? getPhaseStatus(tab.phaseKey, ctx) : (ctx.running ? 'active' : 'idle');
        const Icon = tab.icon;

        return (
          <div key={tab.to} className="flex items-center">
            {/* Flow connector line */}
            {i > 0 && (
              <div className="w-6 h-px bg-surface-line relative mx-0.5 flex-shrink-0">
                {ctx.running && (
                  <span className="absolute -top-[3px] left-1/2 -translate-x-1/2 w-1.5 h-1.5 rounded-full bg-molten/60 pipeline-particle" style={{ animationDelay: `${i * 0.3}s` }} />
                )}
              </div>
            )}

            <NavLink
              to={tab.to}
              end={tab.to === '/'}
              className={({ isActive }) =>
                `flex items-center gap-2 px-3 py-2 rounded-lg text-xs font-semibold transition-all duration-200 whitespace-nowrap ${
                  isActive
                    ? 'bg-graphite text-molten border border-molten/20'
                    : 'text-aluminum-dim hover:text-aluminum hover:bg-steel border border-transparent'
                }`
              }
            >
              <StatusIndicator status={status} />
              <Icon className="w-3.5 h-3.5" />
              <span className="font-display">{tab.label}</span>
            </NavLink>
          </div>
        );
      })}
    </nav>
  );
}
