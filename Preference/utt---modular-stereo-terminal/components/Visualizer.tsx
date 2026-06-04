import React, { useEffect, useRef } from 'react';

interface VisualizerProps {
  width?: number;
  height?: number;
  stereoWidth: number;
  gain: number;
  isMono: boolean;
  phaseInvert: boolean;
}

export const Visualizer: React.FC<VisualizerProps> = ({ 
  width = 400, 
  height = 240, 
  stereoWidth, 
  gain, 
  isMono,
  phaseInvert
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const dpr = window.devicePixelRatio || 1;
    const rect = canvas.getBoundingClientRect();
    
    // Safety check for zero dimensions
    if (rect.width === 0 || rect.height === 0) return;

    canvas.width = rect.width * dpr;
    canvas.height = rect.height * dpr;
    ctx.scale(dpr, dpr);

    let animationId: number;
    let time = 0;

    const render = () => {
      time += 0.015;
      
      // Clear with White
      ctx.fillStyle = '#ffffff'; 
      ctx.fillRect(0, 0, rect.width, rect.height);

      const w = rect.width;
      const h = rect.height;
      
      // Safety check inside render loop
      if (w === 0 || h === 0) {
        animationId = requestAnimationFrame(render);
        return;
      }

      const originX = w / 2;
      const originY = h - 25; 
      // Ensure maxRadius is never negative. If h < 35, it defaults to 0.
      const maxRadius = Math.max(0, Math.min(w / 2, h - 35));

      // Only attempt to draw gradients if we have a valid radius
      if (maxRadius > 0) {
        // Draw Background Fan
        const grad = ctx.createRadialGradient(originX, originY, 0, originX, originY, maxRadius);
        grad.addColorStop(0, 'rgba(243, 244, 246, 0.8)'); // gray-100
        grad.addColorStop(1, 'rgba(243, 244, 246, 0.2)');
        ctx.fillStyle = grad;
        ctx.beginPath();
        ctx.arc(originX, originY, maxRadius, Math.PI, 0);
        ctx.fill();

        // --- Draw Grid (Dark Gray Lines) ---
        ctx.lineWidth = 1;

        // 1. Arcs
        const rings = [0.25, 0.5, 0.75, 1.0];
        rings.forEach((r, i) => {
          ctx.beginPath();
          ctx.strokeStyle = '#e5e7eb'; // Light gray-200
          if (i === rings.length - 1) ctx.strokeStyle = '#d1d5db'; // gray-300
          
          ctx.arc(originX, originY, maxRadius * r, Math.PI, 0);
          ctx.stroke();
          
          if (i > 0) {
              ctx.fillStyle = '#9ca3af'; // gray-400
              ctx.font = '500 9px JetBrains Mono';
              ctx.textAlign = 'right';
              const label = i === 3 ? '0dB' : i === 2 ? '-6dB' : '-12dB';
              ctx.fillText(label, originX - 4, originY - (maxRadius * r) + 10);
          }
        });

        // 2. Rays
        for (let i = 0; i <= 180; i += 15) {
          const rad = (i * Math.PI) / 180;
          const isMajor = i % 45 === 0;
          
          ctx.beginPath();
          ctx.strokeStyle = isMajor ? '#d1d5db' : '#f3f4f6'; 
          
          const x = originX + Math.cos(Math.PI - rad) * maxRadius;
          const y = originY - Math.sin(Math.PI - rad) * maxRadius;
          
          ctx.moveTo(originX, originY);
          ctx.lineTo(x, y);
          ctx.stroke();

          if (isMajor) {
              const labelDist = maxRadius + 15;
              const lx = originX + Math.cos(Math.PI - rad) * labelDist;
              const ly = originY - Math.sin(Math.PI - rad) * labelDist;
              
              ctx.fillStyle = '#6b7280'; // gray-500
              ctx.font = '600 10px JetBrains Mono';
              ctx.textAlign = 'center';
              ctx.textBaseline = 'middle';
              
              let text = '';
              if (i === 180) text = 'L';
              else if (i === 135) text = '45°';
              else if (i === 90) text = 'M';
              else if (i === 45) text = '45°';
              else if (i === 0) text = 'R';
              
              ctx.fillText(text, lx, ly);
          }
        }

        // --- Draw Audio Vector Cloud (Dark Ink Style) ---
        const gainLinear = Math.pow(10, gain / 20);
        const widthFactor = isMono ? 0 : (stereoWidth / 100); 

        // Use Multiply for ink effect on white
        ctx.globalCompositeOperation = 'multiply';
        
        const numParticles = 120;
        let prevX = originX;
        let prevY = originY;

        for (let i = 0; i < numParticles; i++) {
          const tOffset = i * 0.04;
          const t = time + tOffset;

          let L = (Math.sin(t * 3) * 0.5 + Math.cos(t * 5.2) * 0.3 + Math.sin(t * 11) * 0.15) * gainLinear;
          let R = (Math.sin(t * 3.1) * 0.5 + Math.cos(t * 5.1) * 0.3 + Math.sin(t * 11.2) * 0.15) * gainLinear;
          
          if (isMono) { R = L; } 
          else {
              const M = (L + R) / 2;
              const S = (L - R) / 2 * widthFactor;
              L = M + S;
              R = M - S;
          }

          if (phaseInvert) { R = -R; }

          const vecL_x = L * Math.cos(3 * Math.PI / 4);
          const vecL_y = L * Math.sin(3 * Math.PI / 4);
          const vecR_x = R * Math.cos(Math.PI / 4);
          const vecR_y = R * Math.sin(Math.PI / 4);
          
          let sumX = vecL_x + vecR_x;
          let sumY = vecL_y + vecR_y;

          sumX *= maxRadius * 0.85;
          sumY *= maxRadius * 0.85;

          const drawX = originX + sumX;
          const drawY = originY - sumY;

          // Color Logic for Light Mode
          // Center (In Phase): Dark Slate/Blue
          // Sides (Out of Phase): Deep Maroon/Red
          const angle = Math.atan2(sumY, sumX);
          const deviation = Math.abs(angle - Math.PI / 2);
          const corr = Math.max(0, 1 - (deviation / (Math.PI / 3))); 
          
          // RGB values for ink
          // High correlation (1) -> Dark Blue/Gray: 30, 41, 59
          // Low correlation (0) -> Dark Red: 153, 27, 27
          
          const rVal = Math.floor(30 * corr + 153 * (1 - corr));
          const gVal = Math.floor(41 * corr + 27 * (1 - corr));
          const bVal = Math.floor(59 * corr + 27 * (1 - corr));

          const opacity = 0.6 - (i/numParticles)*0.5;

          ctx.strokeStyle = `rgba(${rVal}, ${gVal}, ${bVal}, ${opacity})`;
          
          if (i > 0) {
              ctx.beginPath();
              ctx.lineWidth = 2.5;
              ctx.lineCap = 'round';
              ctx.lineJoin = 'round';
              ctx.moveTo(prevX, prevY);
              ctx.lineTo(drawX, drawY);
              ctx.stroke();
          }
          
          prevX = drawX;
          prevY = drawY;

          // Tip
          if (i > numParticles - 5) {
              ctx.fillStyle = `rgba(${rVal}, ${gVal}, ${bVal}, 0.8)`;
              ctx.beginPath();
              ctx.arc(drawX, drawY, 2, 0, Math.PI * 2);
              ctx.fill();
          }
        }
      }
      
      ctx.globalCompositeOperation = 'source-over';
      animationId = requestAnimationFrame(render);
    };

    render();

    return () => cancelAnimationFrame(animationId);
  }, [width, height, stereoWidth, gain, isMono, phaseInvert]);

  return (
    <div className="w-full h-full rounded-md overflow-hidden bg-white relative">
      <canvas 
        ref={canvasRef} 
        style={{ width: '100%', height: '100%' }}
        className="block"
      />
      <div className="absolute top-3 left-3 flex flex-col gap-0.5 pointer-events-none">
        <span className="text-[10px] font-bold text-gray-800 tracking-wider">POLAR FIELD</span>
        <span className="text-[9px] text-gray-400 font-mono">RMS VIEW</span>
      </div>
    </div>
  );
};