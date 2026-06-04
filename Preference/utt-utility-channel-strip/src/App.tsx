import React, { useState, useEffect, useRef } from 'react';
import { MonoMode, MonoToStereoMode, PluginParameters, AudioStats, AutomationLane, LFOConfig, AutomationTarget } from './types';
import { UTTAudioEngine } from './audioEngine';
import { Knob } from './components/Knob';
import { AudioDisplay } from './components/AudioDisplay';
import { AutomationConsole } from './components/AutomationConsole';
import { TapeRecorder } from './components/TapeRecorder';
import { 
  Play, 
  Volume2, 
  VolumeX, 
  Upload, 
  Settings, 
  HelpCircle, 
  ToggleLeft, 
  ToggleRight, 
  ShieldAlert, 
  Activity, 
  Database,
  Radio,
  FileAudio
} from 'lucide-react';

export default function App() {
  const [params, setParams] = useState<PluginParameters>({
    gain: 0,
    pan: 0,
    width: 1,
    phaseInvL: false,
    phaseInvR: false,
    phaseOffset: 0,
    swapChannels: false,
    monoMode: 'STEREO',
    monoToStereo: 'OFF',
    dcFilter: false,
    dcOffsetL: 0,
    dcOffsetR: 0,
    mute: false,
    bypass: false,
  });

  const [stats, setStats] = useState<AudioStats>({
    peakL: -120,
    peakR: -120,
    rmsL: -120,
    rmsR: -120,
    correlation: 1.0,
    lufsMomentary: -120,
    lufsShortTerm: -120,
    lufsIntegrated: -120,
    dcOffsetL_current: 0,
    dcOffsetR_current: 0,
  });

  // Built-in loop play options
  const [activeSourceType, setActiveSourceType] = useState<'silent' | 'sine' | 'noise' | 'synth' | 'mic' | 'file'>('silent');
  const [audioFileName, setAudioFileName] = useState<string | null>(null);
  const [isDragOver, setIsDragOver] = useState(false);

  // Automation / Modulation States
  const [isPlayingAutomation, setIsPlayingAutomation] = useState(false);
  const [isRecordingAutomation, setIsRecordingAutomation] = useState(false);
  const [currentTimeNormalized, setCurrentTimeNormalized] = useState(0);

  const [automationLanes, setAutomationLanes] = useState<AutomationLane[]>([
    { target: 'gain', enabled: false, points: [{ time: 0, value: 0 }, { time: 1, value: 0 }] },
    { target: 'pan', enabled: false, points: [{ time: 0, value: 0 }, { time: 1, value: 0 }] },
    { target: 'width', enabled: false, points: [{ time: 0, value: 1 }, { time: 1, value: 1 }] },
    { target: 'phaseOffset', enabled: false, points: [{ time: 0, value: 0 }, { time: 1, value: 0 }] },
  ]);

  const [lfo, setLfo] = useState<LFOConfig>({
    target: 'none',
    shape: 'SINE',
    frequency: 1.0,
    amount: 0.3,
    active: false,
  });

  // Tape recording states
  const [isTapeRecording, setIsTapeRecording] = useState(false);
  const [recordedTapeUrl, setRecordedTapeUrl] = useState<string | null>(null);
  const [recordedTapeDuration, setRecordedTapeDuration] = useState(0);

  // Audio Engine Lifecycle instances with browser lazy loading guards
  const engineRef = useRef<UTTAudioEngine | null>(null);
  const playbackHeadRef = useRef(0); // in seconds
  const animationFrameRef = useRef<number | null>(null);
  const prevLfoPhaseRef = useRef(0);
  const currentLfoValRef = useRef(0); // hold last value for Sample & Hold LFO

  useEffect(() => {
    // Initialise audio engine instance safely
    const engine = new UTTAudioEngine();
    engineRef.current = engine;

    engine.registerStatsCallback((realtimeStats) => {
      setStats(realtimeStats);
    });

    return () => {
      engine.destroy();
      if (animationFrameRef.current) cancelAnimationFrame(animationFrameRef.current);
    };
  }, []);

  // Sync parameter values to engine dynamically on update
  const handleParamChange = (pKey: keyof PluginParameters, val: any) => {
    const nextParams = { ...params, [pKey]: val };
    setParams(nextParams);

    // If recording automation on manual knobs twist, capture changes
    if (isRecordingAutomation) {
      captureAutomationPoint(pKey as AutomationTarget, val);
    }

    if (engineRef.current) {
      engineRef.current.updateParams(nextParams);
    }
  };

  // Helper inside central frame loop to interpolate curve points smoothly
  const interpolateCurve = (points: { time: number; value: number }[], t: number): number | null => {
    if (points.length === 0) return null;
    if (points.length === 1) return points[0].value;

    for (let i = 0; i < points.length - 1; i++) {
      const p1 = points[i];
      const p2 = points[i + 1];
      if (t >= p1.time && t <= p2.time) {
        const denom = p2.time - p1.time;
        if (denom === 0) return p1.value;
        const progress = (t - p1.time) / denom;
        return p1.value + progress * (p2.value - p1.value);
      }
    }
    return points[points.length - 1].value;
  };

  // Automation capture during manual tweaks
  const captureAutomationPoint = (target: AutomationTarget, exactValue: number) => {
    if (!['gain', 'pan', 'width', 'phaseOffset'].includes(target)) return;

    setAutomationLanes(prevLanes => {
      return prevLanes.map(lane => {
        if (lane.target === target) {
          const tN = parseFloat(currentTimeNormalized.toFixed(3));
          let updatedPoints = [...lane.points];
          const existingIdx = updatedPoints.findIndex(pt => Math.abs(pt.time - tN) < 0.02);

          if (existingIdx > -1) {
            updatedPoints[existingIdx] = { time: tN, value: exactValue };
          } else {
            updatedPoints.push({ time: tN, value: exactValue });
          }

          updatedPoints.sort((a, b) => a.time - b.time);
          return { ...lane, points: updatedPoints, enabled: true };
        }
        return lane;
      });
    });
  };

  // Central frame loop for clock, automation, and LFO modulations calculation
  useEffect(() => {
    const loopDuration = 6.0; // 6-second master timeline loop representing 1 bar loop inside DAW host
    let lastTime = performance.now();

    const tick = () => {
      const now = performance.now();
      const deltaSecs = (now - lastTime) / 1000;
      lastTime = now;

      if (isPlayingAutomation || isRecordingAutomation) {
        // Playback tracking
        playbackHeadRef.current = (playbackHeadRef.current + deltaSecs) % loopDuration;
        const tNormalized = playbackHeadRef.current / loopDuration;
        setCurrentTimeNormalized(tNormalized);

        // Apply automation curves
        const evaluatedParams: Partial<PluginParameters> = {};

        automationLanes.forEach(lane => {
          if (lane.enabled && isPlayingAutomation && !isRecordingAutomation) {
            const val = interpolateCurve(lane.points, tNormalized);
            if (val !== null) {
              evaluatedParams[lane.target as any] = val;
            }
          }
        });

        if (Object.keys(evaluatedParams).length > 0) {
          setParams(prev => ({ ...prev, ...evaluatedParams }));
          engineRef.current?.updateParams(evaluatedParams);
        }
      }

      // Apply LFO modulations
      if (lfo.active && lfo.target !== 'none') {
        const shapeFreq = lfo.frequency;
        const speedMultiplier = 2 * Math.PI * shapeFreq * deltaSecs;
        
        prevLfoPhaseRef.current = (prevLfoPhaseRef.current + speedMultiplier) % (2 * Math.PI);
        const phi = prevLfoPhaseRef.current;

        let outputLFO = 0;
        switch (lfo.shape) {
          case 'SINE':
            outputLFO = Math.sin(phi);
            break;
          case 'TRIANGLE':
            outputLFO = Math.abs((phi / Math.PI) - 1) * 2 - 1; // triangle oscillation
            break;
          case 'SQUARE':
            outputLFO = Math.sin(phi) >= 0 ? 1 : -1;
            break;
          case 'RANDOM':
            // Sample and Hold: change on phase reset boundary approx
            if (Math.floor(phi) === 0 || Math.random() < 0.01) {
              currentLfoValRef.current = Math.random() * 2 - 1;
            }
            outputLFO = currentLfoValRef.current;
            break;
        }

        // Apply depth / target bounds mapping
        const depth = lfo.amount;
        const targetKey = lfo.target;

        let baseVal = 0;
        let pMin = 0;
        let pMax = 1;

        if (targetKey === 'gain') {
          baseVal = params.gain;
          pMin = -48; pMax = 24;
          const range = pMax - pMin;
          const delta = outputLFO * depth * (range / 4); // modulate output db within reasonable scale
          const modulatedGain = Math.max(pMin, Math.min(pMax, baseVal + delta));
          engineRef.current?.updateParams({ gain: modulatedGain });
        } else if (targetKey === 'pan') {
          baseVal = params.pan;
          pMin = -1; pMax = 1;
          const delta = outputLFO * depth * 0.8;
          const modulatedPan = Math.max(pMin, Math.min(pMax, baseVal + delta));
          engineRef.current?.updateParams({ pan: modulatedPan });
        } else if (targetKey === 'width') {
          baseVal = params.width;
          pMin = 0; pMax = 2;
          const delta = outputLFO * depth * 1.5;
          const modulatedWidth = Math.max(pMin, Math.min(pMax, baseVal + delta));
          engineRef.current?.updateParams({ width: modulatedWidth });
        } else if (targetKey === 'phaseOffset') {
          baseVal = params.phaseOffset;
          pMin = -50; pMax = 50;
          const delta = outputLFO * depth * 25;
          const modulatedOffset = Math.max(pMin, Math.min(pMax, baseVal + delta));
          engineRef.current?.updateParams({ phaseOffset: modulatedOffset });
        }
      }

      animationFrameRef.current = requestAnimationFrame(tick);
    };

    animationFrameRef.current = requestAnimationFrame(tick);

    return () => {
      if (animationFrameRef.current) cancelAnimationFrame(animationFrameRef.current);
    };
  }, [isPlayingAutomation, isRecordingAutomation, automationLanes, lfo, params]);

  // Handle music preview files loading
  const handleLocalFileDrop = async (e: React.DragEvent) => {
    e.preventDefault();
    setIsDragOver(false);
    const files = e.dataTransfer.files;
    if (files.length > 0) {
      await loadCustomAudioFile(files[0]);
    }
  };

  const handleLocalFileSelect = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const files = e.target.files;
    if (files && files.length > 0) {
      await loadCustomAudioFile(files[0]);
    }
  };

  const loadCustomAudioFile = async (file: File) => {
    if (!engineRef.current) return;
    try {
      setAudioFileName('正在解码音频...');
      const buffer = await engineRef.current.uploadAudioFile(file);
      setAudioFileName(file.name);
      setActiveSourceType('file');
      engineRef.current.playBuffer(buffer);
    } catch (e) {
      console.error(e);
      alert('解码本地音频失败，请确保格式支持 (.mp3 / .wav / .m4a / etc)');
      setAudioFileName(null);
    }
  };

  const selectSource = (type: 'silent' | 'sine' | 'noise' | 'synth' | 'mic' | 'file') => {
    if (!engineRef.current) return;
    engineRef.current.resume();
    
    if (type === 'file' && !audioFileName) {
      alert('请先使用下方框片上传本地音频文件！');
      return;
    }

    setActiveSourceType(type);
    engineRef.current.playSource(type);
  };

  // Tape Recorder Callbacks
  const startTapeRecording = () => {
    if (!engineRef.current) return;
    setRecordedTapeUrl(null);
    setIsTapeRecording(true);
    engineRef.current.startRecording();
  };

  const stopTapeRecording = async () => {
    if (!engineRef.current) return;
    setIsTapeRecording(false);
    try {
      const result = await engineRef.current.stopRecording();
      setRecordedTapeUrl(result.url);
      setRecordedTapeDuration(result.duration);
    } catch (e) {
      console.error(e);
    }
  };

  const playTapeRecording = () => {
    if (!recordedTapeUrl || !engineRef.current) return;
    // Playback directly via general audio engine to route back into visualizers
    engineRef.current.playSource('silent'); // silence external synths
    const fakeAudio = new Audio(recordedTapeUrl);
    fakeAudio.play().catch(err => console.error(err));
  };

  return (
    <div className="min-h-screen bg-neutral-950 text-slate-100 flex flex-col justify-between py-6 px-4 md:px-8 selection:bg-[#00FF41] selection:text-neutral-900" id="vst-plugin-wrapper">
      
      {/* Upper Top Navbar Details */}
      <header className="max-w-7xl w-full mx-auto flex flex-col sm:flex-row items-center justify-between border-b border-neutral-850 pb-4 mb-6 gap-3" id="plugin-topbar">
        <div className="flex items-center space-x-3" id="vst-title-group">
          <div className="w-10 h-10 rounded-lg bg-gradient-to-br from-[#00FF41] to-emerald-700 flex items-center justify-center border border-[#00FF41] font-mono text-neutral-950 font-black text-xl shadow-md">
            U
          </div>
          <div>
            <h1 className="text-lg font-mono font-black text-white tracking-widest flex items-center gap-1.5 leading-none">
              UTT UTILITY STRIP <span className="text-[10px] bg-[#00FF41] text-black px-1 py-0.5 rounded font-black font-sans shadow-sm">VST3</span>
            </h1>
            <p className="text-[10px] text-neutral-500 font-mono mt-1 uppercase tracking-wide">
              Universal Channel Utility Tool - Mastering & Diagnostic Assistant
            </p>
          </div>
        </div>
        
        {/* Bypass/Mute state indicators on top strip */}
        <div className="flex items-center space-x-3 bg-neutral-900 border border-neutral-805 px-3.5 py-1.5 rounded" id="vst-quick-bypasses">
          <button
            id="btn-bypass"
            onClick={() => handleParamChange('bypass', !params.bypass)}
            className={`px-3 py-1 font-mono text-[10px] rounded transition duration-75 uppercase font-bold tracking-widest ${
              params.bypass 
                ? 'bg-red-500 text-neutral-950 font-black shadow-[0_0_6px_rgba(239,68,68,0.4)]' 
                : 'text-neutral-400 hover:text-white bg-neutral-950'
            }`}
          >
            BYPASS (直通)
          </button>
          <button
            id="btn-mute"
            onClick={() => handleParamChange('mute', !params.mute)}
            className={`px-3 py-1 font-mono text-[10px] rounded transition duration-75 uppercase font-bold tracking-widest ${
              params.mute 
                ? 'bg-red-500 text-white font-black shadow-[0_0_6px_rgba(239,68,68,0.4)]' 
                : 'text-neutral-400 hover:text-white bg-neutral-950'
            }`}
          >
            MUTE (静音)
          </button>
        </div>
      </header>

      {/* Main interface layout: Channel Strip + Screen visualizer */}
      <main className="max-w-7xl w-full mx-auto flex-1 grid grid-cols-1 lg:grid-cols-12 gap-6" id="plugin-core-grid">
        
        {/* LEFT COMPACT HARDWARE PANEL CHANNEL STRIP */}
        <section className="lg:col-span-12 xl:col-span-5 bg-neutral-900 border-2 border-neutral-800 rounded-lg p-5 shadow-2xl relative overflow-hidden" id="plugin-left-rack">
          <div className="absolute top-0 right-0 w-32 h-32 bg-[#00FF41]/2 rounded-full blur-3xl pointer-events-none" />
          
          {/* Audio Source Test Generator Block */}
          <div className="space-y-3 border-b border-neutral-800 pb-4 mb-4" id="audio-generators-panel">
            <span className="text-[10px] font-mono text-neutral-500 uppercase tracking-widest font-black flex items-center gap-1.5">
              <Radio className="w-3.5 h-3.5 text-[#00FF41]" />
              音频源选择 / Audio Tester Input
            </span>
            <div className="grid grid-cols-3 gap-1.5 text-center">
              {[
                { id: 'silent', label: '无 (Silent)' },
                { id: 'sine', label: '正弦 1kHz (Sine)' },
                { id: 'noise', label: '白噪声 (Noise)' },
                { id: 'synth', label: '相移谱曲 (A-Synth)' },
                { id: 'mic', label: '麦克风 (Mic)' },
                { id: 'file', label: '本地音乐 (Loop)' },
              ].map(src => (
                <button
                  key={src.id}
                  id={`src-select-${src.id}`}
                  onClick={() => selectSource(src.id as any)}
                  className={`py-1.5 px-2 font-mono text-[9px] rounded-sm transition uppercase border tracking-wide font-semibold ${
                    activeSourceType === src.id
                      ? 'bg-[#00FF41] text-black border-[#00FF41] font-bold shadow-md'
                      : 'bg-neutral-950 border-neutral-850 text-neutral-400 hover:text-white hover:border-neutral-700'
                  }`}
                >
                  {src.label}
                </button>
              ))}
            </div>

            {/* Local uploader drop panel inside card */}
            <div
              onDragOver={(e) => { e.preventDefault(); setIsDragOver(true); }}
              onDragLeave={() => setIsDragOver(false)}
              onDrop={handleLocalFileDrop}
              className={`p-3 border rounded-md transition text-center flex flex-col items-center justify-center group relative cursor-pointer ${
                isDragOver ? 'border-[#00FF41] bg-[#00FF41]/5' : 'border-neutral-800/80 bg-neutral-950/40 hover:border-neutral-750'
              }`}
              id="file-dropzone-box"
            >
              <input
                id="file-hidden-input"
                type="file"
                accept="audio/*"
                onChange={handleLocalFileSelect}
                className="absolute inset-0 opacity-0 cursor-pointer w-full h-full"
              />
              <Upload className="w-5 h-5 text-neutral-500 mb-1 group-hover:text-[#00FF41] transition-colors" />
              <div className="font-mono text-[9px] text-neutral-300 font-medium">
                {audioFileName ? `当前文件: ${audioFileName}` : '拖放音频文件至此处，或点击浏览'}
              </div>
              <div className="font-mono text-[8px] text-neutral-500 mt-0.5">(.wav, .mp3, .m4a loop support)</div>
            </div>
          </div>

          {/* Analog dial/Knobs matrix controls */}
          <div className="grid grid-cols-2 sm:grid-cols-4 lg:grid-cols-2 gap-4" id="knobs-matrix-controls">
            <Knob
              id="knob-gain"
              label="增益 Gain"
              value={params.gain}
              min={-48}
              max={24}
              defaultValue={0}
              unit="dB"
              step={0.1}
              onChange={(val) => handleParamChange('gain', val)}
              formatValue={(val) => `${val > 0 ? '+' : ''}${val.toFixed(1)}`}
            />

            <Knob
              id="knob-pan"
              label="声像 Panning"
              value={params.pan}
              min={-1}
              max={1}
              defaultValue={0}
              step={0.01}
              onChange={(val) => handleParamChange('pan', val)}
              formatValue={(val) => {
                if (val === 0) return 'C';
                return val > 0 ? `${(val * 100).toFixed(0)}% R` : `${(Math.abs(val) * 100).toFixed(0)}% L`;
              }}
            />

            <Knob
              id="knob-width"
              label="左右分离 Width"
              value={params.width}
              min={0}
              max={2}
              defaultValue={1}
              step={0.02}
              onChange={(val) => handleParamChange('width', val)}
              formatValue={(val) => {
                if (val === 1) return '100% (ST)';
                if (val === 0) return '0% (MONO)';
                return `${(val * 100).toFixed(0)}%`;
              }}
            />

            <Knob
              id="knob-offset"
              label="通道偏移 Delay"
              value={params.phaseOffset}
              min={-50}
              max={50}
              defaultValue={0}
              unit="ms"
              step={0.5}
              onChange={(val) => handleParamChange('phaseOffset', val)}
              formatValue={(val) => {
                if (val === 0) return '0.0';
                return val > 0 ? `+${val.toFixed(1)} L` : `${val.toFixed(1)} R`;
              }}
            />
          </div>

          {/* Hardware Phase & DC Offset filter switches (Two row modules) */}
          <div className="mt-5 space-y-4" id="toggle-switches-rack">
            {/* Phase Inversion & Channels Swap */}
            <div className="bg-neutral-950 p-3 rounded-md border border-neutral-850 grid grid-cols-3 gap-2" id="phase-switches-rack">
              <div className="flex flex-col items-center">
                <span className="text-[8px] font-mono text-neutral-500 mb-1 font-bold">相位反转 L</span>
                <button
                  id="btn-phase-l"
                  onClick={() => handleParamChange('phaseInvL', !params.phaseInvL)}
                  className={`w-full py-1.5 text-[10px] font-mono border rounded ${
                    params.phaseInvL 
                      ? 'bg-[#00FF41]/20 text-[#00FF41] border-[#00FF41]/55 font-bold shadow-[inset_0_1px_3px_rgba(0,255,65,0.2)]' 
                      : 'bg-neutral-900 text-neutral-400 border-neutral-800 hover:text-white'
                  }`}
                >
                  Ø - LEFT
                </button>
              </div>

              <div className="flex flex-col items-center">
                <span className="text-[8px] font-mono text-neutral-500 mb-1 font-bold">相位反转 R</span>
                <button
                  id="btn-phase-r"
                  onClick={() => handleParamChange('phaseInvR', !params.phaseInvR)}
                  className={`w-full py-1.5 text-[10px] font-mono border rounded ${
                    params.phaseInvR 
                      ? 'bg-[#00FF41]/20 text-[#00FF41] border-[#00FF41]/55 font-bold shadow-[inset_0_1px_3px_rgba(0,255,65,0.2)]' 
                      : 'bg-neutral-900 text-neutral-400 border-neutral-800 hover:text-white'
                  }`}
                >
                  Ø - RIGHT
                </button>
              </div>

              <div className="flex flex-col items-center">
                <span className="text-[8px] font-mono text-neutral-500 mb-1 font-bold">左右通道交换</span>
                <button
                  id="btn-swap"
                  onClick={() => handleParamChange('swapChannels', !params.swapChannels)}
                  className={`w-full py-1.5 text-[10px] font-mono border rounded ${
                    params.swapChannels 
                      ? 'bg-[#00FF41]/20 text-[#00FF41] border-[#00FF41]/55 font-bold shadow-[inset_0_1px_3px_rgba(0,255,65,0.2)]' 
                      : 'bg-neutral-900 text-neutral-400 border-neutral-800 hover:text-white'
                  }`}
                >
                  SWAP L/R
                </button>
              </div>
            </div>

            {/* DC Offset filtration & injection simulation (Extremely helpful for engineering tests!) */}
            <div className="bg-neutral-950 p-3.5 rounded-md border border-neutral-850 space-y-2.5" id="dc-offset-rack">
              <div className="flex items-center justify-between border-b border-neutral-900 pb-1.5">
                <span className="text-[9px] font-mono text-neutral-400 uppercase tracking-wider font-bold">DC 偏移校正与偏置模拟</span>
                <button
                  id="btn-dc-filter"
                  onClick={() => handleParamChange('dcFilter', !params.dcFilter)}
                  className={`px-3 py-1 text-[9px] font-mono rounded border uppercase font-bold tracking-wide transition ${
                    params.dcFilter 
                      ? 'bg-[#00FF41]/20 text-[#00FF41] border-[#00FF41]/55' 
                      : 'bg-neutral-900 text-neutral-400 border-amber-800 hover:text-white'
                  }`}
                >
                  DC 15Hz 滤波器: {params.dcFilter ? '已开启' : '已旁路'}
                </button>
              </div>

              <div className="space-y-1.5" id="dc-injectors-sliders">
                <div className="flex justify-between items-center text-[8px] font-mono text-neutral-500">
                  <span>注入 Left DC 偏相模拟流</span>
                  <span className={Math.abs(params.dcOffsetL) > 0.01 ? 'text-[#00FF41]' : 'text-neutral-500'}>
                    {params.dcOffsetL > 0 ? '+' : ''}{params.dcOffsetL.toFixed(2)}
                  </span>
                </div>
                <input
                  id="slider-dc-offset-l"
                  type="range"
                  min={-0.3}
                  max={0.3}
                  step={0.01}
                  value={params.dcOffsetL}
                  onChange={(e) => handleParamChange('dcOffsetL', parseFloat(e.target.value))}
                  className="w-full accent-[#00FF41] h-1 bg-neutral-900 cursor-ew-resize opacity-80 hover:opacity-100"
                />

                <div className="flex justify-between items-center text-[8px] font-mono text-neutral-500 pt-1">
                  <span>注入 Right DC 偏相模拟流</span>
                  <span className={Math.abs(params.dcOffsetR) > 0.01 ? 'text-[#00FF41]' : 'text-neutral-500'}>
                    {params.dcOffsetR > 0 ? '+' : ''}{params.dcOffsetR.toFixed(2)}
                  </span>
                </div>
                <input
                  id="slider-dc-offset-r"
                  type="range"
                  min={-0.3}
                  max={0.3}
                  step={0.01}
                  value={params.dcOffsetR}
                  onChange={(e) => handleParamChange('dcOffsetR', parseFloat(e.target.value))}
                  className="w-full accent-[#00FF41] h-1 bg-neutral-900 cursor-ew-resize opacity-80 hover:opacity-100"
                />
              </div>
            </div>

            {/* Mono routing Mode & copy to both channels */}
            <div className="grid grid-cols-2 gap-3" id="mono-matrix-grids">
              {/* Mono to Stereo Copy Mode selector */}
              <div className="space-y-1">
                <span className="text-[8px] font-mono text-neutral-500 uppercase font-bold">声道单向复制 / Mirror</span>
                <select
                  id="select-mono-copy"
                  value={params.monoToStereo}
                  onChange={(e) => handleParamChange('monoToStereo', e.target.value as MonoToStereoMode)}
                  className="w-full bg-neutral-950 border border-neutral-850 font-mono text-[10px] p-2 text-white rounded outline-none focus:border-[#00FF41]/50"
                >
                  <option value="OFF">关闭复制 (OFF)</option>
                  <option value="L_TO_BOTH">左声道覆盖右 (L-&gt;R)</option>
                  <option value="R_TO_BOTH">右声道覆盖左 (R-&gt;L)</option>
                </select>
              </div>

              {/* Advanced Mono Submodes routing selector */}
              <div className="space-y-1">
                <span className="text-[8px] font-mono text-neutral-500 uppercase font-bold">输出路由模式 / Routing</span>
                <select
                  id="select-mono-mode"
                  value={params.monoMode}
                  onChange={(e) => handleParamChange('monoMode', e.target.value as MonoMode)}
                  className="w-full bg-neutral-950 border border-neutral-850 font-mono text-[10px] p-2 text-white rounded outline-none focus:border-[#00FF41]/50"
                >
                  <option value="STEREO">立体声标准 (Stereo)</option>
                  <option value="MONO">完全单声道化 (Mono sum)</option>
                  <option value="LEFT_ONLY">左声道独奏 (Solo Left)</option>
                  <option value="RIGHT_ONLY">右声道独奏 (Solo Right)</option>
                  <option value="MID_ONLY">中置Mid独奏 (Solo Mid)</option>
                  <option value="SIDE_ONLY">边置Side独奏 (Solo Side)</option>
                </select>
              </div>
            </div>
          </div>
        </section>

        {/* RIGHT METERS DISPLAY PANEL SCREENS */}
        <section className="lg:col-span-12 xl:col-span-7 flex flex-col h-full justify-between" id="plugin-right-screen-wrapper">
          <AudioDisplay
            stats={stats}
            analL={engineRef.current?.analL || null}
            analR={engineRef.current?.analR || null}
          />
        </section>

      </main>

      {/* LOWER STACK - AUTOMATION + RECORDING TAPE CONSOLE */}
      <footer className="max-w-7xl w-full mx-auto" id="plugin-footer-racks">
        <AutomationConsole
          currentLFO={lfo}
          onUpdateLFO={(updatedLfo) => setLfo(prev => ({ ...prev, ...updatedLfo }))}
          automationLanes={automationLanes}
          onUpdateLanes={(nextLanes) => setAutomationLanes(nextLanes)}
          isPlayingAutomation={isPlayingAutomation}
          onTogglePlayAutomation={setIsPlayingAutomation}
          isRecordingAutomation={isRecordingAutomation}
          onToggleRecordAutomation={setIsRecordingAutomation}
          currentTimeNormalized={currentTimeNormalized}
        />

        <TapeRecorder
          isRecording={isTapeRecording}
          onStartRecord={startTapeRecording}
          onStopRecord={stopTapeRecording}
          recordedUrl={recordedTapeUrl}
          recordedDuration={recordedTapeDuration}
          onPlayRecording={playTapeRecording}
          onDeleteRecording={() => setRecordedTapeUrl(null)}
        />
      </footer>
    </div>
  );
}
