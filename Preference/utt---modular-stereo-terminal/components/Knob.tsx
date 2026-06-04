import React, { useState, useRef, useEffect, useCallback } from 'react';

interface KnobProps {
  label: string;
  value: number;
  min: number;
  max: number;
  defaultValue: number;
  unit?: string;
  onChange: (val: number) => void;
  size?: 'sm' | 'md' | 'lg';
  color?: string;
  steps?: number;
  description?: string;
}

export const Knob: React.FC<KnobProps> = ({
  label,
  value,
  min,
  max,
  defaultValue,
  unit = '',
  onChange,
  size = 'md',
  color = '#1f2937', // Default to dark gray/black
  steps,
  description
}) => {
  const [isDragging, setIsDragging] = useState(false);
  const [startY, setStartY] = useState(0);
  const [startVal, setStartVal] = useState(0);
  const [isEditing, setIsEditing] = useState(false);
  const [editValue, setEditValue] = useState(value.toString());
  const inputRef = useRef<HTMLInputElement>(null);

  // Size configurations
  const sizes = {
    sm: { size: 36, stroke: 3, fontSize: 'text-[9px]' },
    md: { size: 48, stroke: 4, fontSize: 'text-[10px]' },
    lg: { size: 64, stroke: 5, fontSize: 'text-xs' },
  };
  const { size: dim, stroke, fontSize } = sizes[size];
  const radius = (dim - stroke) / 2;
  const circumference = 2 * Math.PI * radius;

  // Calculate rotation (270 degrees range, from -135 to +135)
  const percent = (value - min) / (max - min);
  const angle = percent * 270 - 135;
  
  const arcLength = circumference * 0.75;
  const dashOffset = arcLength * (1 - percent); 
  const dashArray = `${arcLength} ${circumference}`;

  const handleMouseDown = (e: React.MouseEvent) => {
    if (e.button !== 0) return;
    e.preventDefault();
    setIsDragging(true);
    setStartY(e.clientY);
    setStartVal(value);
    document.body.style.cursor = 'ns-resize';
  };

  const handleGlobalMouseMove = useCallback((e: MouseEvent) => {
    if (!isDragging) return;
    const deltaY = startY - e.clientY;
    const range = max - min;
    const sensitivity = 200;
    let newValue = startVal + (deltaY / sensitivity) * range;

    if (newValue < min) newValue = min;
    if (newValue > max) newValue = max;

    if (steps) {
      newValue = Math.round(newValue / steps) * steps;
    }

    onChange(Number(newValue.toFixed(2)));
  }, [isDragging, startY, startVal, max, min, onChange, steps]);

  const handleGlobalMouseUp = useCallback(() => {
    setIsDragging(false);
    document.body.style.cursor = 'default';
  }, []);

  useEffect(() => {
    if (isDragging) {
      window.addEventListener('mousemove', handleGlobalMouseMove);
      window.addEventListener('mouseup', handleGlobalMouseUp);
    } else {
      window.removeEventListener('mousemove', handleGlobalMouseMove);
      window.removeEventListener('mouseup', handleGlobalMouseUp);
    }
    return () => {
      window.removeEventListener('mousemove', handleGlobalMouseMove);
      window.removeEventListener('mouseup', handleGlobalMouseUp);
    };
  }, [isDragging, handleGlobalMouseMove, handleGlobalMouseUp]);

  const handleDoubleClick = () => {
    onChange(defaultValue);
  };

  const handleContextMenu = (e: React.MouseEvent) => {
    e.preventDefault();
    setIsEditing(true);
    setEditValue(value.toString());
    setTimeout(() => inputRef.current?.focus(), 50);
  };

  const handleInputBlur = () => {
    setIsEditing(false);
    let val = parseFloat(editValue);
    if (isNaN(val)) val = defaultValue;
    if (val < min) val = min;
    if (val > max) val = max;
    onChange(Number(val.toFixed(2)));
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter') {
      handleInputBlur();
    }
    if (e.key === 'Escape') {
      setIsEditing(false);
    }
  };

  return (
    <div className="flex flex-col items-center gap-1.5 relative group">
      <span className={`text-gray-500 font-bold uppercase tracking-widest ${fontSize} select-none`}>
        {label}
      </span>

      <div 
        className="relative select-none"
        onMouseDown={handleMouseDown}
        onDoubleClick={handleDoubleClick}
        onContextMenu={handleContextMenu}
      >
        {isEditing ? (
          <div 
            className="absolute inset-0 flex items-center justify-center z-10" 
            style={{ width: dim, height: dim }}
          >
            <input
              ref={inputRef}
              type="text"
              value={editValue}
              onChange={(e) => setEditValue(e.target.value)}
              onBlur={handleInputBlur}
              onKeyDown={handleKeyDown}
              className="w-full text-center bg-white text-gray-900 border border-blue-500 rounded p-1 text-[10px] outline-none shadow-lg font-mono"
            />
          </div>
        ) : null}

        <svg width={dim} height={dim} className="transform rotate-90" style={{ transform: 'rotate(90deg) scaleY(-1)'}}>
          {/* Track background */}
          <circle
            cx={dim / 2}
            cy={dim / 2}
            r={radius}
            fill="none"
            stroke="#e5e7eb"
            strokeWidth={stroke}
            strokeDasharray={dashArray}
            strokeDashoffset={0}
            strokeLinecap="round"
            className="origin-center rotate-[135deg]"
          />
          {/* Active value */}
          <circle
            cx={dim / 2}
            cy={dim / 2}
            r={radius}
            fill="none"
            stroke={color}
            strokeWidth={stroke}
            strokeDasharray={dashArray}
            strokeDashoffset={dashOffset}
            strokeLinecap="round"
            className="origin-center rotate-[135deg]"
          />
        </svg>

        {/* Marker Line */}
        <div 
           className="absolute top-0 left-0 w-full h-full pointer-events-none flex items-center justify-center"
           style={{ transform: `rotate(${angle}deg)` }}
        >
           <div className="w-[2px] h-1/2 bg-transparent relative">
              <div 
                className="absolute top-1 left-0 w-full rounded-full" 
                style={{ height: stroke + 1, backgroundColor: color }}
              ></div>
           </div>
        </div>
      </div>

      <div className={`font-mono font-medium text-gray-700 ${size === 'sm' ? 'text-[9px]' : 'text-[10px]'}`}>
        {value.toFixed(1)}{unit}
      </div>
      
      {/* Light Theme Tooltip */}
      <div className="opacity-0 group-hover:opacity-100 transition-opacity duration-200 absolute -bottom-12 left-1/2 -translate-x-1/2 bg-white text-gray-900 text-[10px] px-3 py-2 rounded-md shadow-[0_4px_12px_rgba(0,0,0,0.1)] border border-gray-200 z-50 pointer-events-none min-w-[120px] text-center">
         <div className="font-bold mb-0.5 text-blue-600">{label}</div>
         <div className="text-gray-500 leading-tight">{description || "Double-click to reset"}</div>
      </div>
    </div>
  );
};