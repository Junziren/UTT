import React, { useState, useEffect, useRef } from 'react';
import { AutomationLane, AutomationTarget, LFOConfig, AutomationPoint } from '../types';
import { Play, Square, Circle, RefreshCw, Trash2, Sliders } from 'lucide-react';

interface AutomationConsoleProps {
  currentLFO: LFOConfig;
  onUpdateLFO: (lfo: Partial<LFOConfig>) => void;
  automationLanes: AutomationLane[];
  onUpdateLanes: (lanes: AutomationLane[]) => void;
  isPlayingAutomation: boolean;
  onTogglePlayAutomation: (play: boolean) => void;
  isRecordingAutomation: boolean;
  onToggleRecordAutomation: (record: boolean) => void;
  currentTimeNormalized: number; // 0 to 1 value of playback head
}

export const AutomationConsole: React.FC<AutomationConsoleProps> = ({
  currentLFO,
  onUpdateLFO,
  automationLanes,
  onUpdateLanes,
  isPlayingAutomation,
  onTogglePlayAutomation,
  isRecordingAutomation,
  onToggleRecordAutomation,
  currentTimeNormalized
}) => {
  const [selectedTarget, setSelectedTarget] = useState<AutomationTarget>('gain');
  const [editorMode, setEditorMode] = useState<'CURVE' | 'LFO'>('CURVE');
  const curveTimelineRef = useRef<HTMLDivElement>(null);

  // Targets configurations definitions helper
  const targetConfig = {
    gain: { label: '增益 (Gain)', min: -48, max: 24, unit: 'dB', defaultValue: 0 },
    pan: { label: '声像 (Pan)', min: -1, max: 1, unit: '%', defaultValue: 0 },
    width: { label: '立体声宽度 (Width)', min: 0, max: 2, unit: 'x', defaultValue: 1 },
    phaseOffset: { label: '相位偏移 (Phase Offset)', min: -50, max: 50, unit: 'ms', defaultValue: 0 }
  };

  const activeLane = automationLanes.find(lane => lane.target === selectedTarget);

  // Quick helper to draw automation curves on canvas
  const handleTimelineClick = (e: React.MouseEvent<HTMLDivElement>) => {
    if (!curveTimelineRef.current || !activeLane) return;

    const rect = curveTimelineRef.current.getBoundingClientRect();
    const x = (e.clientX - rect.left) / rect.width; // 0 to 1
    const y = 1 - (e.clientY - rect.top) / rect.height; // 0 to 1 (bottom up)

    const cfg = targetConfig[selectedTarget];
    // Map normalized y to parameter range
    const val = cfg.min + y * (cfg.max - cfg.min);

    // Add or replace node point in sequence
    const newPoint: AutomationPoint = { time: parseFloat(x.toFixed(3)), value: parseFloat(val.toFixed(2)) };
    
    // Sort and integrate points
    let updatedPoints = [...activeLane.points];
    // If point already matches approximate time, update or delete
    const existingIndex = updatedPoints.findIndex(pt => Math.abs(pt.time - x) < 0.03);
    if (existingIndex > -1) {
      if (e.shiftKey) {
        // Shift+Click deletes point
        updatedPoints.splice(existingIndex, 1);
      } else {
        updatedPoints[existingIndex] = newPoint;
      }
    } else {
      updatedPoints.push(newPoint);
    }

    updatedPoints.sort((a, b) => a.time - b.time);

    // Always keep boundaries
    if (updatedPoints.length === 0 || updatedPoints[0].time > 0.05) {
      updatedPoints.unshift({ time: 0, value: cfg.defaultValue });
    }
    if (updatedPoints[updatedPoints.length - 1].time < 0.95) {
      updatedPoints.push({ time: 1, value: cfg.defaultValue });
    }

    const nextLanes = automationLanes.map(lane => 
      lane.target === selectedTarget ? { ...lane, points: updatedPoints, enabled: true } : lane
    );
    onUpdateLanes(nextLanes);
  };

  const clearLanePoints = () => {
    const cfg = targetConfig[selectedTarget];
    const defaultPoints = [
      { time: 0, value: cfg.defaultValue },
      { time: 1, value: cfg.defaultValue }
    ];
    const nextLanes = automationLanes.map(lane => 
      lane.target === selectedTarget ? { ...lane, points: defaultPoints, enabled: false } : lane
    );
    onUpdateLanes(nextLanes);
  };

  const toggleLaneStatus = () => {
    if (!activeLane) return;
    const nextLanes = automationLanes.map(lane => 
      lane.target === selectedTarget ? { ...lane, enabled: !lane.enabled } : lane
    );
    onUpdateLanes(nextLanes);
  };

  // Convert exact parameter value into a height percentage representation
  const valueToPercentHeight = (val: number, target: AutomationTarget) => {
    const cfg = targetConfig[target];
    const rawRatio = (val - cfg.min) / (cfg.max - cfg.min);
    return Math.max(0, Math.min(100, rawRatio * 100));
  };

  return (
    <div className="bg-neutral-900 border-2 border-neutral-800 rounded-md p-4 mt-4 shadow-md" id="plugin-automation-console">
      {/* Title block with control toggles */}
      <div className="flex flex-col md:flex-row md:items-center justify-between border-b border-neutral-800 pb-3 mb-3 gap-2" id="automation-headers">
        <div id="labels-title">
          <h3 className="text-sm font-mono text-white flex items-center font-bold tracking-widest gap-2">
            <Sliders className="w-4 h-4 text-[#00FF41]" />
            UTT 自动化与低频调制引擎 (Automation & Modulation)
          </h3>
          <p className="text-[10px] text-neutral-500 font-mono">
            支持即时手拟录制，或通过 LFO 低阶正弦/方波振荡器进行参数映射控制
          </p>
        </div>

        {/* Global PLAYBACK & RECORD Automation Buttons */}
        <div className="flex items-center space-x-2 bg-neutral-950 p-1.5 rounded border border-neutral-850" id="automation-master-controls">
          <button
            onClick={() => onTogglePlayAutomation(!isPlayingAutomation)}
            id="btn-play-automation"
            className={`px-3 py-1 text-[10px] font-mono font-bold tracking-wide flex items-center gap-1.5 rounded transition ${
              isPlayingAutomation 
                ? 'bg-emerald-500 text-neutral-950 shadow-md' 
                : 'text-neutral-400 hover:text-white hover:bg-neutral-900'
            }`}
          >
            {isPlayingAutomation ? <Square className="w-3 h-3 fill-current" /> : <Play className="w-3 h-3 fill-current" />}
            {isPlayingAutomation ? '停止重放' : '重放曲线'}
          </button>

          <button
            onClick={() => onToggleRecordAutomation(!isRecordingAutomation)}
            id="btn-record-automation"
            className={`px-3 py-1 text-[10px] font-mono font-bold tracking-wide flex items-center gap-1.5 rounded transition ${
              isRecordingAutomation 
                ? 'bg-red-500 text-neutral-950 shadow-lg animate-pulse' 
                : 'text-neutral-400 hover:text-white hover:bg-neutral-900'
            }`}
          >
            <Circle className={`w-3 h-3 ${isRecordingAutomation ? 'fill-current' : ''}`} />
            {isRecordingAutomation ? '正录制(拧动旋钮)...' : '录制旋钮动作'}
          </button>
        </div>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-12 gap-4" id="modulation-workspace">
        {/* Left target selection list */}
        <div className="lg:col-span-3 flex flex-col space-y-2 border-r border-neutral-800 pr-2" id="targets-selection-panel">
          <span className="text-[10px] font-mono text-neutral-500 tracking-wider uppercase font-bold">选择操作参数 / Select Target</span>
          {Object.entries(targetConfig).map(([key, cfg]) => {
            const laneObj = automationLanes.find(l => l.target === key);
            const isTargetActive = selectedTarget === key;
            return (
              <button
                key={key}
                id={`target-select-${key}`}
                onClick={() => setSelectedTarget(key as AutomationTarget)}
                className={`text-left p-2 rounded border font-mono text-xs flex justify-between items-center transition ${
                  isTargetActive
                    ? 'bg-[#00FF41]/10 border-[#00FF41]/30 text-[#00FF41]'
                    : 'bg-neutral-950/40 border-neutral-850 text-neutral-400 hover:border-neutral-700 hover:text-neutral-350'
                }`}
              >
                <div>
                  <div className="font-semibold">{cfg.label}</div>
                  <div className="text-[9px] text-neutral-500 mt-0.5">({cfg.min} ~ {cfg.max} {cfg.unit})</div>
                </div>
                {laneObj?.enabled && (
                  <span className="w-2.5 h-2.5 rounded-full bg-emerald-500 shadow-[0_0_4px_rgba(16,185,129,0.5)]" title="Active automation curve detected!" />
                )}
              </button>
            );
          })}
        </div>

        {/* Right workspace detailing Curve Editing or LFO Modulation */}
        <div className="lg:col-span-9 flex flex-col justify-between" id="automation-subsections">
          {/* Tab Selection */}
          <div className="flex border-b border-neutral-850 pb-2 mb-2 justify-between items-center" id="mode-selector">
            <div className="flex space-x-1.5">
              <button
                onClick={() => setEditorMode('CURVE')}
                id="toggle-curve-mode"
                className={`py-1 px-3 text-[11px] font-mono rounded font-bold transition ${
                  editorMode === 'CURVE' ? 'bg-neutral-950 border border-neutral-805 text-[#00FF41] shadow-inner' : 'text-neutral-500 hover:text-neutral-300'
                }`}
              >
                手绘自动曲线 (Drawing Curve)
              </button>
              <button
                onClick={() => setEditorMode('LFO')}
                id="toggle-lfo-mode"
                className={`py-1 px-3 text-[11px] font-mono rounded font-bold transition ${
                  editorMode === 'LFO' ? 'bg-neutral-950 border border-neutral-805 text-[#00FF41] shadow-inner' : 'text-neutral-500 hover:text-neutral-300'
                }`}
              >
                LFO 自动振荡调制 (LFO Modulation)
              </button>
            </div>

            {editorMode === 'CURVE' && activeLane && (
              <div className="flex items-center space-x-2">
                <button
                  id="btn-toggle-lane-active"
                  onClick={toggleLaneStatus}
                  className={`px-2 py-0.5 text-[10px] font-mono border rounded ${
                    activeLane.enabled 
                      ? 'border-emerald-600/40 bg-emerald-500/10 text-emerald-400' 
                      : 'border-neutral-700 bg-neutral-950 text-neutral-500 hover:text-neutral-400'
                  }`}
                >
                  {activeLane.enabled ? '曲线使能: 开' : '曲线使能: 关'}
                </button>
                <button
                  id="btn-clear-curve-points"
                  onClick={clearLanePoints}
                  className="p-1 text-neutral-500 hover:text-red-400 hover:bg-neutral-950 border border-transparent hover:border-neutral-800 rounded transition"
                  title="Clear all points"
                >
                  <Trash2 className="w-3.5 h-3.5" />
                </button>
              </div>
            )}
          </div>

          {/* Curve Editor Interface */}
          {editorMode === 'CURVE' ? (
            <div className="space-y-2">
              <div
                ref={curveTimelineRef}
                onClick={handleTimelineClick}
                className="relative h-32 bg-neutral-950 rounded-md border-2 border-dashed border-neutral-850/80 cursor-crosshair select-none group overflow-hidden"
                id="canvas-curve-timeline"
              >
                {/* Horizontal reference midpoint helper lines */}
                <div className="absolute top-1/2 left-0 right-0 h-[1px] bg-neutral-900 border-t border-dashed border-neutral-800 pointer-events-none" />

                {/* SVG Automation Graph Lines */}
                {activeLane && activeLane.points.length > 0 && (
                  <svg className="absolute inset-0 w-full h-full pointer-events-none">
                    <polyline
                      fill="none"
                      stroke="#00FF41"
                      strokeWidth="2.5"
                      strokeLinecap="round"
                      points={activeLane.points
                        .map(pt => {
                          const xPos = pt.time * 100; // percent width
                          const yPos = 100 - valueToPercentHeight(pt.value, selectedTarget); // invert for SVG height coordinates
                          return `${xPos}%,${yPos}%`;
                        })
                        .join(' ')}
                      className="origin-top-left"
                      style={{ width: '100%', height: '100%' }}
                    />
                    {/* Glowing background filled surface under automation */}
                    <polygon
                      fill="rgba(0, 255, 65, 0.04)"
                      points={`0%,100% ` + activeLane.points
                        .map(pt => {
                          const xPos = pt.time * 100;
                          const yPos = 100 - valueToPercentHeight(pt.value, selectedTarget);
                          return `${xPos}%,${yPos}%`;
                        })
                        .join(' ') + ` 100%,100%`}
                      style={{ width: '100%', height: '100%' }}
                    />
                  </svg>
                )}

                {/* Draw interactively small circular dot handles for each keyframe point */}
                {activeLane?.points.map((pt, i) => {
                  const leftPct = pt.time * 100;
                  const bottomPct = valueToPercentHeight(pt.value, selectedTarget);
                  return (
                    <div
                      key={i}
                      className="absolute w-3 h-3 -ml-1.5 -mb-1.5 bg-[#00FF41] rounded-full border-2 border-neutral-950 shadow-[0_0_6px_#00FF41] flex items-center justify-center cursor-ns-resize group-hover:scale-110 transition-transform"
                      style={{ left: `${leftPct}%`, bottom: `${bottomPct}%` }}
                      title={`Time: ${(pt.time * 100).toFixed(0)}%, Val: ${pt.value}`}
                    />
                  );
                })}

                {/* Automation playhead loop bar tracker */}
                {isPlayingAutomation && (
                  <div
                    className="absolute top-0 bottom-0 w-0.5 bg-cyan-400 shadow-[0_0_6px_rgba(6,182,212,0.8)] z-10 pointer-events-none"
                    style={{ left: `${currentTimeNormalized * 100}%` }}
                  />
                )}
              </div>
              <div className="flex justify-between items-center text-[9px] font-mono text-neutral-500">
                <span>提示: 点击虚线版面新增关键帧。Shift+点击已有节点可删除该关键帧。双击面板外部恢复默认参数。</span>
                <span>LOOP TIMELINE (0% to 100%)</span>
              </div>
            </div>
          ) : (
            /* LFO Synthesizer Modulation Interface */
            <div className="bg-neutral-950/80 border border-neutral-850 rounded p-4 flex flex-col md:flex-row items-center justify-between gap-4" id="lfo-parameters-matrix">
              {/* LFO State Activation and Target Settings */}
              <div className="w-full md:w-1/3 space-y-3">
                <div className="flex justify-between items-center bg-neutral-900 px-3 py-2 rounded border border-neutral-800">
                  <span className="text-[11px] font-mono text-white">低频发生器状态</span>
                  <button
                    id="btn-toggle-lfo"
                    onClick={() => onUpdateLFO({ active: !currentLFO.active })}
                    className={`px-3 py-1 text-[10px] font-mono font-bold tracking-wide rounded ${
                      currentLFO.active 
                        ? 'bg-[#00FF41] text-neutral-950 font-bold' 
                        : 'bg-neutral-805 text-neutral-400 hover:text-white'
                    }`}
                  >
                    {currentLFO.active ? 'LFO_ON' : 'LFO_OFF'}
                  </button>
                </div>

                <div className="space-y-1">
                  <span className="text-[10px] font-mono text-neutral-500">调制参数目标 / Destination</span>
                  <select
                    id="select-lfo-target"
                    value={currentLFO.target}
                    onChange={(e) => onUpdateLFO({ target: e.target.value as any })}
                    className="w-full bg-neutral-900 border border-neutral-800 text-xs font-mono p-2 text-white rounded outline-none focus:border-[#00FF41]/50"
                  >
                    <option value="none">未连接 (Unmapped)</option>
                    <option value="gain">增益 (Gain db)</option>
                    <option value="pan">声像 (Pan L/R)</option>
                    <option value="width">立体声宽度 (Width)</option>
                    <option value="phaseOffset">相位延迟偏移 (Sample Offset)</option>
                  </select>
                </div>
              </div>

              {/* Slider variables values adjustments */}
              <div className="w-full md:w-2/3 grid grid-cols-2 gap-4">
                {/* Modulation Shape selection */}
                <div className="space-y-1">
                  <span className="text-[10px] font-mono text-neutral-500">波形形状 / Wave Shape</span>
                  <div className="grid grid-cols-2 gap-1.5">
                    {['SINE', 'TRIANGLE', 'SQUARE', 'RANDOM'].map((sh) => (
                      <button
                        key={sh}
                        id={`btn-lfo-shape-${sh}`}
                        onClick={() => onUpdateLFO({ shape: sh as any })}
                        className={`py-1.5 px-2 text-[10px] uppercase font-mono rounded border text-center transition ${
                          currentLFO.shape === sh
                            ? 'bg-[#00FF41]/10 border-[#00FF41]/30 text-[#00FF41] font-bold'
                            : 'bg-neutral-900 border-neutral-805 text-neutral-400 hover:text-white'
                        }`}
                      >
                        {sh === 'RANDOM' ? 'S/H' : sh}
                      </button>
                    ))}
                  </div>
                </div>

                {/* Speed / Frequency slider */}
                <div className="space-y-1">
                  <div className="flex justify-between text-[10px] font-mono">
                    <span className="text-neutral-500">振荡频率 / Rate:</span>
                    <span className="text-[#00FF41]">{currentLFO.frequency.toFixed(2)} Hz</span>
                  </div>
                  <input
                    id="slider-lfo-freq"
                    type="range"
                    min={0.1}
                    max={10.0}
                    step={0.05}
                    value={currentLFO.frequency}
                    onChange={(e) => onUpdateLFO({ frequency: parseFloat(e.target.value) })}
                    className="w-full accent-[#00FF41] cursor-ew-resize bg-neutral-900 border border-neutral-800 rounded h-1.5"
                  />
                  
                  <div className="flex justify-between text-[10px] font-mono pt-1.5">
                    <span className="text-neutral-500">增幅幅度 / Depth:</span>
                    <span className="text-[#00FF41]">{(currentLFO.amount * 100).toFixed(0)}%</span>
                  </div>
                  <input
                    id="slider-lfo-depth"
                    type="range"
                    min={0.0}
                    max={1.0}
                    step={0.05}
                    value={currentLFO.amount}
                    onChange={(e) => onUpdateLFO({ amount: parseFloat(e.target.value) })}
                    className="w-full accent-[#00FF41] cursor-ew-resize bg-neutral-900 border border-neutral-800 rounded h-1.5"
                  />
                </div>
              </div>
            </div>
          )}
        </div>
      </div>
    </div>
  );
};
