import React, { useEffect, useState, useRef } from 'react';
import { AlertTriangle, RefreshCw } from 'lucide-react';

interface LoudnessMonitorProps {
  gain: number;
}

const STANDARDS = [
  { id: 'shorts', label: 'SHORT VIDEO', threshold: -14, desc: 'TikTok/Reels' },
  { id: 'stream', label: 'STREAMING', threshold: -14, desc: 'Spotify/Apple' },
  { id: 'cast', label: 'BROADCAST', threshold: -23, desc: 'TV/EBU R128' },
  { id: 'film', label: 'FILM / CINE', threshold: -27, desc: 'Theatrical' },
];

export const LoudnessMonitor: React.FC<LoudnessMonitorProps> = ({ gain }) => {
  // Peak meters need fast attack, slow release
  const [peakL, setPeakL] = useState(-60);
  const [peakR, setPeakR] = useState(-60);
  const [lufs, setLufs] = useState(-99);
  const [alerts, setAlerts] = useState<Record<string, boolean>>({});
  
  // Internal refs for physics
  const currentPeakL = useRef(-60);
  const currentPeakR = useRef(-60);

  useEffect(() => {
    let animationFrame: number;

    const updateMeters = () => {
      const time = Date.now() / 1000;
      const signalBase = -20 + gain; 
      
      // Simulate erratic signal (Peak)
      const noiseL = Math.sin(time * 11) * 5 + (Math.random() * 10) - 5;
      const noiseR = Math.sin(time * 13) * 5 + (Math.random() * 10) - 5;
      
      const targetL = Math.min(Math.max(signalBase + noiseL, -60), 3);
      const targetR = Math.min(Math.max(signalBase + noiseR, -60), 3);

      // Attack / Release Logic for Peak Meter
      // Instant attack (go up immediately), slow release (go down slowly)
      if (targetL > currentPeakL.current) {
        currentPeakL.current = targetL;
      } else {
        currentPeakL.current -= 0.5; // Release speed
      }

      if (targetR > currentPeakR.current) {
        currentPeakR.current = targetR;
      } else {
        currentPeakR.current -= 0.5;
      }

      setPeakL(currentPeakL.current);
      setPeakR(currentPeakR.current);

      // Simulate Integrated LUFS (more stable)
      const currentLufs = Math.max(-60, signalBase + (Math.sin(time) * 2));
      setLufs(currentLufs);

      // Check thresholds
      const newAlerts = { ...alerts };
      let changed = false;
      STANDARDS.forEach(std => {
        if (!newAlerts[std.id] && currentLufs > std.threshold) {
          newAlerts[std.id] = true;
          changed = true;
        }
      });
      if (changed) setAlerts(newAlerts);

      animationFrame = requestAnimationFrame(updateMeters);
    };

    animationFrame = requestAnimationFrame(updateMeters);
    return () => cancelAnimationFrame(animationFrame);
  }, [gain, alerts]);

  const resetAlerts = () => setAlerts({});

  // Render a single DB Bar
  const DBBar = ({ level, label }: { level: number, label: string }) => {
    // Map -60dB -> +3dB to 0% -> 100% width
    const min = -60;
    const max = 3;
    const range = max - min;
    const percent = Math.min(100, Math.max(0, ((level - min) / range) * 100));

    return (
      <div className="flex items-center gap-2 w-full">
        <span className="text-[9px] font-mono text-gray-500 w-3">{label}</span>
        <div className="flex-1 h-3 bg-gray-800 rounded-sm relative overflow-hidden">
          {/* Grid lines */}
          <div className="absolute top-0 left-0 w-full h-full z-10 flex justify-between px-[10%] opacity-30">
             <div className="w-[1px] h-full bg-gray-600"></div>
             <div className="w-[1px] h-full bg-gray-600"></div>
             <div className="w-[1px] h-full bg-gray-600"></div>
             <div className="w-[1px] h-full bg-red-500"></div> {/* 0dB */}
          </div>
          
          {/* The Meter Fill */}
          <div 
            style={{ width: `${percent}%` }}
            className="h-full bg-gradient-to-r from-green-500 via-yellow-400 to-red-500 transition-all duration-75 ease-out"
          ></div>
        </div>
        <span className="text-[9px] font-mono text-gray-600 w-6 text-right">{level.toFixed(0)}</span>
      </div>
    );
  };

  return (
    <div className="bg-matte panel-shadow p-4 rounded-lg border border-gray-200/50 flex flex-col gap-4 relative overflow-hidden">
       {/* Header */}
       <div className="flex justify-between items-center border-b border-gray-200/60 pb-2">
         <div className="flex items-center gap-2">
           <AlertTriangle size={14} className="text-gray-500" />
           <span className="text-[10px] font-bold uppercase tracking-widest text-gray-700">Loudness Monitor</span>
         </div>
         <button onClick={resetAlerts} className="text-gray-400 hover:text-gray-900 transition-colors bg-white p-1 rounded shadow-sm border border-gray-100" title="Reset Warnings">
           <RefreshCw size={12} />
         </button>
       </div>

       <div className="flex gap-4">
         {/* Standards / Alerts */}
         <div className="flex flex-col justify-between w-[110px] shrink-0">
            {STANDARDS.map(std => (
              <div key={std.id} className="flex justify-between items-center bg-white/60 p-1.5 rounded border border-gray-100 shadow-sm">
                <div className="flex flex-col">
                  <span className="text-[8px] font-bold text-gray-600 leading-none tracking-tight">{std.label}</span>
                  <span className="text-[7px] text-gray-400 leading-none mt-0.5 font-mono">Max: {std.threshold}</span>
                </div>
                {/* Warning LED */}
                <div 
                  className={`w-2.5 h-2.5 rounded-full border shadow-inner transition-all duration-300 ${
                    alerts[std.id] 
                      ? 'bg-red-500 border-red-600 shadow-[0_0_6px_rgba(239,68,68,0.8)]' 
                      : 'bg-gray-200 border-gray-300'
                  }`}
                />
              </div>
            ))}
         </div>

         {/* Digital Meters */}
         <div className="flex-1 flex flex-col justify-center gap-3 bg-white/40 rounded p-2 border border-gray-200/50">
             {/* Scale Labels */}
             <div className="flex justify-between px-[22px] text-[7px] font-mono text-gray-400">
                <span>-60</span>
                <span>-40</span>
                <span>-20</span>
                <span>0</span>
                <span>+3</span>
             </div>
             
             <DBBar level={peakL} label="L" />
             <DBBar level={peakR} label="R" />
             
             {/* LUFS Digital Readout */}
             <div className="mt-1 flex justify-between items-center bg-gray-900 rounded px-2 py-1">
                 <span className="text-[9px] text-gray-500 font-bold tracking-wider">INTEGRATED</span>
                 <span className={`text-[11px] font-mono font-bold ${lufs > -14 ? 'text-red-400' : 'text-green-400'}`}>
                    {lufs.toFixed(1)} LUFS
                 </span>
             </div>
         </div>
       </div>
    </div>
  );
};