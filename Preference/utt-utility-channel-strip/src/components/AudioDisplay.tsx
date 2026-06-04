import React, { useEffect, useRef, useState } from 'react';
import { AudioStats } from '../types';

interface AudioDisplayProps {
  stats: AudioStats;
  analL: AnalyserNode | null;
  analR: AnalyserNode | null;
}

type ScreenMode = 'GONIOMETER' | 'OSCILLOSCOPE' | 'METRICS';

export const AudioDisplay: React.FC<AudioDisplayProps> = ({ stats, analL, analR }) => {
  const [screenMode, setScreenMode] = useState<ScreenMode>('GONIOMETER');
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const animationRef = useRef<number | null>(null);

  // Buffer state to keep highest values for Peak indicators
  const [peakHoldL, setPeakHoldL] = useState(-120);
  const [peakHoldR, setPeakHoldR] = useState(-120);

  useEffect(() => {
    // Reset peak holds if statistics reset or go to inf
    if (stats.peakL > peakHoldL) setPeakHoldL(stats.peakL);
    if (stats.peakR > peakHoldR) setPeakHoldR(stats.peakR);

    // Fast decay peak hold
    const timer = setInterval(() => {
      setPeakHoldL(p => Math.max(-120, p - 0.1));
      setPeakHoldR(p => Math.max(-120, p - 0.1));
    }, 100);

    return () => clearInterval(timer);
  }, [stats.peakL, stats.peakR, peakHoldL, peakHoldR]);

  // Real-time canvas drawings (Oscilloscope and Goniometer/Vectorscope)
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    // Local buffer arrays
    let bufL = new Uint8Array(0);
    let bufR = new Uint8Array(0);

    const draw = () => {
      if (!ctx || !canvas) return;

      const W = canvas.width;
      const H = canvas.height;

      if (screenMode === 'GONIOMETER') {
        // CRT-style phosphor fade out effect
        ctx.fillStyle = 'rgba(10, 10, 10, 0.15)';
        ctx.fillRect(0, 0, W, H);

        if (analL && analR) {
          const size = analL.frequencyBinCount;
          if (bufL.length !== size) {
            bufL = new Uint8Array(size);
            bufR = new Uint8Array(size);
          }
          analL.getByteTimeDomainData(bufL);
          analR.getByteTimeDomainData(bufR);

          // Draw radar circle reference lines
          ctx.strokeStyle = '#1e293b';
          ctx.lineWidth = 1;
          ctx.beginPath();
          ctx.arc(W / 2, H / 2, Math.min(W, H) * 0.4, 0, 2 * Math.PI);
          ctx.stroke();

          // Draw 45deg diagonal bounds for Left and Right channels
          ctx.beginPath();
          ctx.moveTo(W / 2 - W * 0.35, H / 2 + H * 0.35);
          ctx.lineTo(W / 2 + W * 0.35, H / 2 - H * 0.35);
          ctx.moveTo(W / 2 - W * 0.35, H / 2 - H * 0.35);
          ctx.lineTo(W / 2 + W * 0.35, H / 2 + H * 0.35);
          ctx.stroke();

          // Crosshairs
          ctx.strokeStyle = '#0f172a';
          ctx.beginPath();
          ctx.moveTo(W / 2, 0);
          ctx.lineTo(W / 2, H);
          ctx.moveTo(0, H / 2);
          ctx.lineTo(W, H / 2);
          ctx.stroke();

          // Labelling axis guides
          ctx.fillStyle = '#64748b';
          ctx.font = '8px monospace';
          ctx.fillText('L', W / 2 - W * 0.38, H / 2 - H * 0.38);
          ctx.fillText('R', W / 2 + W * 0.33, H / 2 - H * 0.38);
          ctx.fillText('M (Mid)', W / 2 - 18, 12);
          ctx.fillText('S (Side)', W - 42, H / 2 + 3);

          // Render Goniometer Plot points
          ctx.strokeStyle = 'rgba(0, 255, 65, 0.75)'; // glowing matrix green points
          ctx.lineWidth = 1.75;
          ctx.shadowBlur = 6;
          ctx.shadowColor = '#00FF41';
          ctx.beginPath();

          const scale = Math.min(W, H) * 0.45;
          for (let i = 0; i < bufL.length; i += 2) {
            // Normalized values -1 to 1 (from 0 - 255 time domain byte data)
            const sL = (bufL[i] - 128) / 128;
            const sR = (bufR[i] - 128) / 128;

            // Goniometer vectors mapping formula:
            // X = (L - R) / sqrt(2)
            // Y = (L + R) / sqrt(2)
            const x = (sL - sR) / Math.sqrt(2);
            const y = (sL + sR) / Math.sqrt(2);

            const dx = W / 2 + x * scale;
            const dy = H / 2 - y * scale; // invert Y for standard visual positioning

            if (i === 0) {
              ctx.moveTo(dx, dy);
            } else {
              ctx.lineTo(dx, dy);
            }
          }
          ctx.stroke();
          ctx.shadowBlur = 0; // disable shadow for future frames speed optimization
        } else {
          // Display inactive fallback instructions
          ctx.fillStyle = '#1c1917';
          ctx.fillRect(0, 0, W, H);
          ctx.fillStyle = '#64748b';
          ctx.font = '11px monospace';
          ctx.fillText('NO AUDIBLE INPUT SIGNAL', W / 2 - 75, H / 2);
        }
      } else if (screenMode === 'OSCILLOSCOPE') {
        // Space grid drawing
        ctx.fillStyle = '#0a0a0a';
        ctx.fillRect(0, 0, W, H);

        ctx.strokeStyle = '#1e293b';
        ctx.lineWidth = 1;
        ctx.beginPath();
        for (let x = 0; x < W; x += 40) {
          ctx.moveTo(x, 0);
          ctx.lineTo(x, H);
        }
        for (let y = 0; y < H; y += 40) {
          ctx.moveTo(0, y);
          ctx.lineTo(W, y);
        }
        ctx.stroke();

        if (analL && analR) {
          const size = analL.frequencyBinCount;
          if (bufL.length !== size) {
            bufL = new Uint8Array(size);
            bufR = new Uint8Array(size);
          }
          analL.getByteTimeDomainData(bufL);
          analR.getByteTimeDomainData(bufR);

          // Draw channel 1 (Left: Cyan/Blue Hue)
          ctx.strokeStyle = 'rgba(6, 182, 212, 0.85)';
          ctx.shadowColor = '#06b6d4';
          ctx.shadowBlur = 6;
          ctx.lineWidth = 1.5;
          ctx.beginPath();
          const sliceWidth = W / bufL.length;
          let x = 0;
          for (let i = 0; i < bufL.length; i++) {
            const v = bufL[i] / 128.0;
            // Draw in top half
            const y = (v * H) / 2 - H / 4;
            if (i === 0) ctx.moveTo(x, y);
            else ctx.lineTo(x, y);
            x += sliceWidth;
          }
          ctx.stroke();

          // Draw channel 2 (Right: Green Hue)
          ctx.strokeStyle = 'rgba(0, 255, 65, 0.85)';
          ctx.shadowColor = '#00FF41';
          ctx.shadowBlur = 6;
          ctx.beginPath();
          x = 0;
          for (let i = 0; i < bufR.length; i++) {
            const v = bufR[i] / 128.0;
            // Draw in bottom half
            const y = (v * H) / 2 + H / 4;
            if (i === 0) ctx.moveTo(x, y);
            else ctx.lineTo(x, y);
            x += sliceWidth;
          }
          ctx.stroke();

          // Labelling channels
          ctx.shadowBlur = 0;
          ctx.fillStyle = '#06b6d4';
          ctx.font = '10px monospace';
          ctx.fillText('L-CH WAVEFORM', 10, H / 4 - 20);
          ctx.fillStyle = '#00FF41';
          ctx.fillText('R-CH WAVEFORM', 10, (3 * H) / 4 - 20);
        }
      }

      animationRef.current = requestAnimationFrame(draw);
    };

    draw();

    return () => {
      if (animationRef.current) cancelAnimationFrame(animationRef.current);
    };
  }, [screenMode, analL, analR]);

  // Utility to convert raw decibels to percentage heights
  const dbToPercent = (db: number) => {
    if (db <= -60) return 0;
    if (db >= 0) return 100;
    // Map -60 to 0dB linearly or scaled curves
    return ((db + 60) / 60) * 100;
  };

  const correlationColor = () => {
    const c = stats.correlation;
    if (c < 0) return 'bg-red-500 shadow-[0_0_8px_rgba(239,68,68,0.5)]'; // Phase inversion issue warning
    if (c > 0.8) return 'bg-emerald-500 shadow-[0_0_8px_rgba(16,185,129,0.5)]'; // High correlation
    return 'bg-[#00FF41] shadow-[0_0_8px_rgba(0,255,65,0.5)]'; // Stereo width content active
  };

  return (
    <div className="flex flex-col h-full bg-neutral-950 rounded-md border-2 border-neutral-800 p-3 shadow-md" id="plugin-right-screen">
      {/* Screen Header tabs */}
      <div className="flex items-center justify-between border-b border-neutral-900 pb-2 mb-2" id="screen-navigation">
        <div className="flex space-x-1.5 bg-neutral-900 p-0.5 rounded border border-neutral-800">
          <button
            id="tab-goniometer"
            onClick={() => setScreenMode('GONIOMETER')}
            className={`px-3 py-1 font-mono text-[10px] tracking-wide rounded uppercase ${
              screenMode === 'GONIOMETER'
                ? 'bg-[#00FF41] text-black font-extrabold shadow-sm'
                : 'text-neutral-400 hover:text-[#00FF41]'
            } transition-colors`}
          >
            矢量相位 (Goniometer)
          </button>
          <button
            id="tab-oscilloscope"
            onClick={() => setScreenMode('OSCILLOSCOPE')}
            className={`px-3 py-1 font-mono text-[10px] tracking-wide rounded uppercase ${
              screenMode === 'OSCILLOSCOPE'
                ? 'bg-[#00FF41] text-black font-extrabold shadow-sm'
                : 'text-neutral-400 hover:text-[#00FF41]'
            } transition-colors`}
          >
            示波器 (Scope)
          </button>
          <button
            id="tab-metrics"
            onClick={() => setScreenMode('METRICS')}
            className={`px-3 py-1 font-mono text-[10px] tracking-wide rounded uppercase ${
              screenMode === 'METRICS'
                ? 'bg-[#00FF41] text-black font-extrabold shadow-sm'
                : 'text-neutral-400 hover:text-[#00FF41]'
            } transition-colors`}
          >
            指标监控 (Metrics)
          </button>
        </div>
        <div className="flex items-center space-x-1.5 font-mono text-[9px] text-[#00FF41]/70">
          <span className="w-1.5 h-1.5 rounded-full bg-[#00FF41] animate-pulse" />
          <span>UTT MONITOR ACTIVE</span>
        </div>
      </div>

      {/* Primary Display Content Container */}
      <div className="flex-1 flex flex-col md:flex-row gap-4 min-h-[220px]" id="screen-main-content">
        {/* Render Graphical screen */}
        {screenMode !== 'METRICS' ? (
          <div className="flex-1 relative aspect-square md:aspect-auto bg-neutral-950 rounded border border-neutral-900 overflow-hidden">
            <canvas
              ref={canvasRef}
              width={320}
              height={260}
              className="w-full h-full block"
              id="oscilloscope-phase-canvas"
            />
          </div>
        ) : (
          /* Numeric & Loudness Metrics view */
          <div className="flex-1 bg-neutral-900/60 rounded border border-neutral-800 p-3.5 flex flex-col justify-between font-mono text-xs text-neutral-300">
            {/* LUFS Segment */}
            <div className="space-y-2">
              <h4 className="text-[10px] uppercase tracking-wide text-[#00FF41] border-b border-neutral-800 pb-1 font-bold">Loudness Measurement (LUFS)</h4>
              <div className="grid grid-cols-3 gap-2 py-1">
                <div className="bg-neutral-950/80 p-2 rounded border border-neutral-800 text-center">
                  <div className="text-[9px] text-neutral-500 mb-0.5">M-LUFS (瞬间)</div>
                  <div className="text-sm font-semibold text-[#00FF41]">
                    {stats.lufsMomentary <= -120 ? '-INF' : stats.lufsMomentary.toFixed(1)}
                  </div>
                </div>
                <div className="bg-neutral-950/80 p-2 rounded border border-neutral-800 text-center">
                  <div className="text-[9px] text-neutral-500 mb-0.5">S-LUFS (短期)</div>
                  <div className="text-sm font-semibold text-[#00FF41]">
                    {stats.lufsShortTerm <= -120 ? '-INF' : stats.lufsShortTerm.toFixed(1)}
                  </div>
                </div>
                <div className="bg-neutral-950/80 p-2 rounded border border-neutral-800 text-center">
                  <div className="text-[9px] text-neutral-500 mb-0.5">I-LUFS (积分)</div>
                  <div className="text-sm font-semibold text-white">
                    {stats.lufsIntegrated <= -120 ? '-INF' : stats.lufsIntegrated.toFixed(1)}
                  </div>
                </div>
              </div>
            </div>

            {/* Signal Values detail segment */}
            <div className="space-y-2 mt-2">
              <h4 className="text-[10px] uppercase tracking-wide text-[#00FF41] border-b border-neutral-800 pb-1 font-bold">Signal Level & DC Component</h4>
              <div className="grid grid-cols-2 gap-3 pt-1">
                <div>
                  <div className="text-[9px] text-neutral-500">PEAK PEAKS (dBFS)</div>
                  <div className="text-xs font-semibold text-neutral-300 flex justify-between pr-2">
                    <span>L: {stats.peakL <= -120 ? '-INF' : stats.peakL.toFixed(1)}</span>
                    <span>R: {stats.peakR <= -120 ? '-INF' : stats.peakR.toFixed(1)}</span>
                  </div>
                </div>
                <div>
                  <div className="text-[9px] text-neutral-500">RMS ENERGY (dBFS)</div>
                  <div className="text-xs font-semibold text-neutral-300 flex justify-between pr-2">
                    <span>L: {stats.rmsL <= -120 ? '-INF' : stats.rmsL.toFixed(1)}</span>
                    <span>R: {stats.rmsR <= -120 ? '-INF' : stats.rmsR.toFixed(1)}</span>
                  </div>
                </div>
                <div className="col-span-2">
                  <div className="text-[9px] text-neutral-500 mb-1">DC BIAS CURRENT (DC Offset)</div>
                  <div className="grid grid-cols-2 gap-2 text-xs font-semibold bg-neutral-950 p-1.5 rounded border border-neutral-800/80 text-center">
                    <span className={Math.abs(stats.dcOffsetL_current) > 0.01 ? 'text-red-400 font-bold' : 'text-neutral-400'}>
                      L-CH: {stats.dcOffsetL_current.toFixed(4)}
                    </span>
                    <span className={Math.abs(stats.dcOffsetR_current) > 0.01 ? 'text-red-400 font-bold' : 'text-neutral-400'}>
                      R-CH: {stats.dcOffsetR_current.toFixed(4)}
                    </span>
                  </div>
                  { (Math.abs(stats.dcOffsetL_current) > 0.05 || Math.abs(stats.dcOffsetR_current) > 0.05) && (
                    <div className="text-[9px] text-red-500 animate-pulse mt-1">⚠️ 发现高等级 DC Offset 偏置电流！请开启 DC 滤波器。</div>
                  )}
                </div>
              </div>
            </div>
          </div>
        )}

        {/* Real-time vertical DB Peak / RMS meters (always displayed) */}
        <div className="w-[84px] shrink-0 bg-neutral-900 border border-neutral-800 rounded p-2 flex flex-col justify-between" id="meter-panel">
          <div className="text-[9px] font-mono text-center text-neutral-500 mb-1 tracking-widest font-bold">LEVEL (dB)</div>
          
          <div className="flex-1 flex gap-2.5 h-full relative" id="meters-bar-container">
            {/* Decibel Label markers on side */}
            <div className="absolute right-0 top-0 bottom-0 flex flex-col justify-between text-[7px] font-mono text-neutral-600 select-none pb-1 pointer-events-none">
              <span>0</span>
              <span>-6</span>
              <span>-12</span>
              <span>-18</span>
              <span>-24</span>
              <span>-36</span>
              <span>-48</span>
              <span>-60</span>
            </div>

            {/* Left Channel Bar */}
            <div className="flex-1 flex flex-col justify-end bg-neutral-950 rounded border border-neutral-800 relative overflow-hidden h-[155px]">
              {/* RMS glowing bar */}
              <div
                className="w-full bg-gradient-to-t from-emerald-500 via-amber-500 to-red-500 opacity-80"
                style={{ height: `${dbToPercent(stats.peakL)}%`, transition: 'height 50ms linear' }}
              />
              {/* Peak Value notch marker */}
              <div
                className="absolute left-0 right-0 h-0.5 bg-white border-t border-neutral-950 z-20"
                style={{ bottom: `calc(${dbToPercent(peakHoldL)}% - 2px)`, transition: 'bottom 150ms ease-out' }}
              />
              <span className="absolute bottom-1 left-0 right-0 text-center text-[7px] font-mono text-white/50 z-10 pointer-events-none">L</span>
            </div>

            {/* Right Channel Bar */}
            <div className="flex-1 flex flex-col justify-end bg-neutral-950 rounded border border-neutral-800 relative overflow-hidden h-[155px]">
              {/* RMS glowing bar */}
              <div
                className="w-full bg-gradient-to-t from-emerald-500 via-amber-500 to-red-500 opacity-80"
                style={{ height: `${dbToPercent(stats.peakR)}%`, transition: 'height 50ms linear' }}
              />
              {/* Peak Value notch marker */}
              <div
                className="absolute left-0 right-0 h-0.5 bg-white border-t border-neutral-950 z-20"
                style={{ bottom: `calc(${dbToPercent(peakHoldR)}% - 2px)`, transition: 'bottom 150ms ease-out' }}
              />
              <span className="absolute bottom-1 left-0 right-0 text-center text-[7px] font-mono text-white/50 z-10 pointer-events-none">R</span>
            </div>
          </div>
        </div>
      </div>

      {/* Bottom Phase Correlation Index display */}
      <div className="mt-3 bg-neutral-900 border border-neutral-850 p-2 rounded" id="phase-correlation-bar">
        <div className="flex items-center justify-between font-mono text-[9px] text-neutral-500 mb-1 uppercase font-bold tracking-widest">
          <span>相位相关系数 (Phase Correlation)</span>
          <span className="font-semibold text-neutral-300">{stats.correlation.toFixed(2)}</span>
        </div>
        <div className="relative h-2.5 bg-neutral-950 rounded overflow-hidden border border-neutral-800 flex items-center">
          {/* Middle 0 center tick line */}
          <div className="absolute left-1/2 top-0 bottom-0 w-0.5 bg-neutral-800 z-10" />
          
          {/* Correlation Glowing Dot indicator */}
          <div
            className={`absolute w-3 h-3 rounded-full ${correlationColor()} transition-all duration-100 ease-out`}
            style={{ left: `calc(${((stats.correlation + 1) / 2) * 100}% - 6px)` }}
          />

          {/* Left/Right extreme phase labels */}
          <div className="absolute left-1 text-[7px] font-mono text-red-500 font-bold z-10">-1 (反相)</div>
          <div className="absolute right-1 text-[7px] font-mono text-emerald-500 font-bold z-10">+1 (同相)</div>
        </div>
      </div>
    </div>
  );
};
