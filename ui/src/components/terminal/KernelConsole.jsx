import { useRef, useEffect, useState } from 'react';
import { Terminal, ChevronDown, ChevronUp } from 'lucide-react';
import { useSimCtx } from '../../context/SimulationContext';

/**
 * KernelConsole — Collapsible terminal footer showing raw simulation output.
 */
export default function KernelConsole() {
  const { terminal } = useSimCtx();
  const endRef = useRef(null);
  const [expanded, setExpanded] = useState(false);

  useEffect(() => {
    if (expanded && endRef.current) {
      endRef.current.scrollIntoView({ behavior: 'smooth' });
    }
  }, [terminal, expanded]);

  const lineColor = (line) => {
    if (line.t === 'sys') return 'text-molten';
    if (line.t === 'err') return 'text-alarm';
    if (line.text.includes('INICIANDO')) return 'text-process-green';
    if (line.text.includes('[Kernel]')) return 'text-aluminum-dim';
    return 'text-aluminum/70';
  };

  return (
    <div className="border-t border-surface-line bg-steel/50 backdrop-blur-sm">
      {/* Toggle bar */}
      <button
        onClick={() => setExpanded(e => !e)}
        className="w-full flex items-center justify-between px-5 py-2 text-xs font-semibold text-aluminum-dim hover:text-aluminum transition-colors cursor-pointer"
      >
        <div className="flex items-center gap-2">
          <Terminal className="w-3.5 h-3.5" />
          <span className="font-display uppercase tracking-wider">Consola del Kernel</span>
          {terminal.length > 0 && (
            <span className="font-mono text-[10px] bg-graphite px-1.5 py-0.5 rounded text-aluminum-dim">
              {terminal.length}
            </span>
          )}
        </div>
        {expanded ? <ChevronDown className="w-4 h-4" /> : <ChevronUp className="w-4 h-4" />}
      </button>

      {/* Terminal body */}
      {expanded && (
        <div className="px-5 pb-3 max-h-60 overflow-y-auto font-mono text-[11px] leading-relaxed space-y-0.5">
          {terminal.length === 0 ? (
            <p className="text-aluminum-dim/50 italic py-4 text-center">
              Consola vacía — inicia la simulación para ver la salida del kernel
            </p>
          ) : (
            terminal.map((line, i) => (
              <div key={i} className={`break-all ${lineColor(line)}`}>{line.text}</div>
            ))
          )}
          <div ref={endRef} />
        </div>
      )}
    </div>
  );
}
