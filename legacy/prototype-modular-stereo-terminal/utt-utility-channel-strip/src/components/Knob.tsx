import React, { useRef, useState, useEffect } from 'react';

interface KnobProps {
  id: string;
  label: string;
  value: number;
  min: number;
  max: number;
  defaultValue: number;
  unit?: string;
  step?: number;
  onChange: (value: number) => void;
  formatValue?: (val: number) => string;
}

export const Knob: React.FC<KnobProps> = ({
  id,
  label,
  value,
  min,
  max,
  defaultValue,
  unit = '',
  step = 0.01,
  onChange,
  formatValue
}) => {
  const knobRef = useRef<HTMLDivElement>(null);
  const [isDragging, setIsDragging] = useState(false);
  const startYRef = useRef(0);
  const startValRef = useRef(0);

  const displayVal = formatValue ? formatValue(value) : value.toFixed(2);

  // Convert current value (min to max) to degrees (-135 to 135)
  const range = max - min;
  const pct = (value - min) / range;
  const degrees = pct * 270 - 135;

  const handlePointerDown = (e: React.PointerEvent) => {
    setIsDragging(true);
    startYRef.current = e.clientY;
    startValRef.current = value;
    knobRef.current?.setPointerCapture(e.pointerId);
  };

  const handlePointerMove = (e: React.PointerEvent) => {
    if (!isDragging) return;
    const dy = startYRef.current - e.clientY; // drag UP to increase, DOWN to decrease
    const pixelRange = 220; // pixels of vertical drag to traverse full range
    const valChange = (dy / pixelRange) * range;
    let newVal = startValRef.current + valChange;

    // Apply snap to steps
    newVal = Math.round(newVal / step) * step;
    newVal = Math.max(min, Math.min(max, newVal));

    onChange(newVal);
  };

  const handlePointerUp = (e: React.PointerEvent) => {
    setIsDragging(false);
    knobRef.current?.releasePointerCapture(e.pointerId);
  };

  const handleDoubleClick = () => {
    onChange(defaultValue);
  };

  const handleWheel = (e: React.WheelEvent) => {
    e.preventDefault();
    const direction = e.deltaY > 0 ? -1 : 1;
    let newVal = value + direction * step * 3; // speed up step on wheel scroll
    newVal = Math.max(min, Math.min(max, newVal));
    onChange(newVal);
  };

  return (
    <div className="flex flex-col items-center justify-center select-none p-2" id={`knob-container-${id}`}>
      {/* Knob Label */}
      <span className="text-[10px] font-mono tracking-widest text-neutral-400 font-bold uppercase mb-1.5 leading-none">
        {label}
      </span>

      {/* Interactive Knob Body */}
      <div
        ref={knobRef}
        id={id}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        onDoubleClick={handleDoubleClick}
        onWheel={handleWheel}
        className="relative w-16 h-16 cursor-ns-resize flex items-center justify-center active:scale-98 transition-transform"
      >
        {/* Background glow arc ring */}
        <svg className="absolute inset-0 w-full h-full" viewBox="0 0 100 100">
          {/* Static grey arc */}
          <circle
            cx="50"
            cy="50"
            r="40"
            fill="none"
            stroke="#1f1f1f"
            strokeWidth="6"
            strokeDasharray="188.5 251.3"
            strokeLinecap="round"
            transform="rotate(135 50 50)"
          />
          {/* Glowing dynamic arc path */}
          <circle
            cx="50"
            cy="50"
            r="40"
            fill="none"
            stroke="url(#green-glow-grad)"
            strokeWidth="6"
            strokeDasharray={`${pct * 188.5} 251.3`}
            strokeLinecap="round"
            transform="rotate(135 50 50)"
            className="transition-all duration-75"
          />
          <defs>
            <linearGradient id="green-glow-grad" x1="0%" y1="0%" x2="100%" y2="100%">
              <stop offset="0%" stopColor="#00FF41" />
              <stop offset="100%" stopColor="#059669" />
            </linearGradient>
          </defs>
        </svg>

        {/* Vintage metallic Knob wheel */}
        <div 
          className="relative w-11 h-11 rounded-full bg-radial from-neutral-700 via-neutral-800 to-neutral-950 shadow-inner flex items-center justify-center border border-neutral-700/50"
          style={{ transform: `rotate(${degrees}deg)` }}
        >
          {/* Brushed metallic faceplate */}
          <div className="w-9 h-9 rounded-full bg-neutral-900 border border-neutral-800/80 relative flex items-center justify-center">
            {/* Center vintage brass cap */}
            <div className="w-3.5 h-3.5 rounded-full bg-emerald-500/10 border border-emerald-600/30 flex items-center justify-center" />
            {/* Green Pointer Dot/notch */}
            <div className="absolute top-1 w-1.5 h-1.5 rounded-full bg-[#00FF41] border border-emerald-600 shadow-[0_0_5px_#00FF41]" />
          </div>
        </div>
      </div>

      {/* Value Readout */}
      <div className="mt-1 flex items-baseline justify-center font-mono text-[11px] font-medium bg-neutral-950 px-2 py-0.5 rounded-sm border border-neutral-800/40 min-w-16 text-center text-[#00FF41] shadow-[inset_0_1px_3px_rgba(0,0,0,0.8)]">
        <span>{displayVal}</span>
        {unit && <span className="text-[9px] text-neutral-500 ml-0.5">{unit}</span>}
      </div>
    </div>
  );
};
