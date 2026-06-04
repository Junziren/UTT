import React, { useState, useRef } from 'react';
import { 
  Zap, 
  ArrowLeftRight, 
  Scaling, 
  MinusCircle, 
  Waves,
  Disc,
  Settings2,
  Copy,
  Download,
  Upload,
  Save
} from 'lucide-react';
import { Knob } from './components/Knob';
import { Visualizer } from './components/Visualizer';
import { LoudnessMonitor } from './components/LoudnessMonitor';
import { AppState, ChannelState, INITIAL_STATE, ImagerBand } from './types';

// Styled Components / Helpers

const TooltipWrapper = ({ children, label, description }: any) => (
  <div className="group relative">
    {children}
    <div className="opacity-0 group-hover:opacity-100 transition-opacity duration-200 absolute bottom-full left-1/2 -translate-x-1/2 mb-2 bg-white text-gray-900 text-[10px] px-3 py-2 rounded-md shadow-[0_4px_12px_rgba(0,0,0,0.1)] border border-gray-200 z-50 pointer-events-none min-w-[120px] text-center w-max max-w-[150px]">
      <div className="font-bold mb-0.5 text-blue-600">{label}</div>
      <div className="text-gray-500 leading-tight">{description}</div>
      <div className="absolute bottom-[-4px] left-1/2 -translate-x-1/2 w-2 h-2 bg-white border-r border-b border-gray-200 transform rotate-45"></div>
    </div>
  </div>
);

const IconButton = ({ active, onClick, icon: Icon, label, description }: any) => (
  <TooltipWrapper label={label} description={description}>
    <button
      onClick={onClick}
      className={`h-9 w-full rounded transition-all flex items-center justify-center border ${
        active 
          ? 'bg-gray-800 border-gray-900 text-white shadow-inner' 
          : 'bg-white border-gray-200 text-gray-500 hover:border-gray-300 hover:text-gray-700 shadow-sm'
      }`}
    >
      <Icon size={16} strokeWidth={2} />
    </button>
  </TooltipWrapper>
);

const ToggleSwitch = ({ active, onClick, label, description }: any) => (
  <TooltipWrapper label={label} description={description}>
    <button 
      onClick={onClick}
      className="flex flex-col items-center gap-2 group"
    >
      <div className={`w-9 h-5 rounded-full p-0.5 transition-colors border shadow-inner ${active ? 'bg-gray-800 border-gray-900' : 'bg-gray-200 border-gray-300'}`}>
        <div className={`w-3.5 h-3.5 bg-white rounded-full shadow-md transform transition-transform duration-200 ${active ? 'translate-x-4' : 'translate-x-0'}`} />
      </div>
      <span className={`text-[9px] uppercase font-bold tracking-wider ${active ? 'text-gray-900' : 'text-gray-400 group-hover:text-gray-500'}`}>
        {label}
      </span>
    </button>
  </TooltipWrapper>
);

const ChannelStrip = ({ 
  label, 
  state, 
  onChange, 
}: { 
  label: string; 
  state: ChannelState; 
  onChange: (s: ChannelState) => void;
}) => {
  const update = (key: keyof ChannelState, val: any) => onChange({ ...state, [key]: val });

  return (
    <div className="flex flex-col items-center gap-3 p-3 bg-white/60 rounded border border-gray-200 shadow-sm hover:shadow-md transition-shadow">
      <div className="text-[10px] font-bold text-gray-800 bg-white/80 px-3 py-0.5 rounded-full border border-gray-200 shadow-sm tracking-wide">{label}</div>
      
      {/* Mute/Solo/Phase Grid */}
      <div className="grid grid-cols-3 gap-1 mb-2 w-full">
         <TooltipWrapper label="Mute" description={`Silence ${label}`}>
           <button 
              onClick={() => update('muted', !state.muted)}
              className={`w-full h-6 text-[9px] font-bold rounded flex items-center justify-center border shadow-sm transition-all ${
                 state.muted ? 'bg-red-500 border-red-600 text-white shadow-inner' : 'bg-white border-gray-200 text-gray-400 hover:border-gray-300'
              }`}
           >M</button>
         </TooltipWrapper>
         <TooltipWrapper label="Solo" description={`Solo ${label}`}>
           <button 
              onClick={() => update('solo', !state.solo)}
              className={`w-full h-6 text-[9px] font-bold rounded flex items-center justify-center border shadow-sm transition-all ${
                 state.solo ? 'bg-yellow-400 border-yellow-500 text-black shadow-inner' : 'bg-white border-gray-200 text-gray-400 hover:border-gray-300'
              }`}
           >S</button>
         </TooltipWrapper>
         <TooltipWrapper label="Phase" description="Invert Polarity">
           <button 
              onClick={() => update('phaseInvert', !state.phaseInvert)}
              className={`w-full h-6 rounded flex items-center justify-center border shadow-sm transition-all ${
                 state.phaseInvert ? 'bg-gray-800 border-gray-900 text-white shadow-inner' : 'bg-white border-gray-200 text-gray-400 hover:border-gray-300'
              }`}
           >
             <Zap size={10} fill={state.phaseInvert ? "currentColor" : "none"}/>
           </button>
         </TooltipWrapper>
      </div>

      <div className="flex flex-col gap-4 py-2 border-t border-gray-100/50 w-full items-center">
        <Knob 
            label="Pan" value={state.pan} min={-100} max={100} defaultValue={0} 
            onChange={(v) => update('pan', v)} size="sm" color="#2563eb"
            description="Stereo Balance"
        />
        <Knob 
            label="Gain" value={state.gain} min={-60} max={12} defaultValue={0} unit="dB"
            onChange={(v) => update('gain', v)} color="#2563eb"
            description="Channel Volume"
        />
      </div>
    </div>
  );
};

const ModuleHeader = ({ icon: Icon, title, rightElement }: any) => (
    <div className="flex justify-between items-center mb-4 pb-2 border-b border-gray-200/60">
        <div className="flex items-center gap-2 text-gray-700">
            <Icon size={14} className="text-gray-400" />
            <span className="text-[10px] font-bold uppercase tracking-widest">{title}</span>
        </div>
        {rightElement}
    </div>
);

export default function App() {
  // A/B System State
  const [activeSlot, setActiveSlot] = useState<'A' | 'B'>('A');
  const [bankA, setBankA] = useState<AppState>(INITIAL_STATE);
  const [bankB, setBankB] = useState<AppState>(INITIAL_STATE);
  const fileInputRef = useRef<HTMLInputElement>(null);

  // Helper to get current state
  const state = activeSlot === 'A' ? bankA : bankB;

  // Helper to update current state
  const setState = (updater: (prev: AppState) => AppState) => {
    if (activeSlot === 'A') setBankA(updater);
    else setBankB(updater);
  };

  const copyState = () => {
    if (activeSlot === 'A') setBankB(bankA);
    else setBankA(bankB);
  };

  // Preset Management
  const handleSavePreset = () => {
    const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(state, null, 2));
    const downloadAnchorNode = document.createElement('a');
    downloadAnchorNode.setAttribute("href", dataStr);
    downloadAnchorNode.setAttribute("download", `utt_preset_${Date.now()}.json`);
    document.body.appendChild(downloadAnchorNode);
    downloadAnchorNode.click();
    downloadAnchorNode.remove();
  };

  const handleLoadPreset = (event: React.ChangeEvent<HTMLInputElement>) => {
    const fileReader = new FileReader();
    if (event.target.files && event.target.files[0]) {
      fileReader.readAsText(event.target.files[0], "UTF-8");
      fileReader.onload = (e) => {
        if(e.target?.result) {
            try {
                const parsed = JSON.parse(e.target.result as string);
                // Basic validation could go here
                setState(() => parsed);
            } catch (err) {
                alert("Invalid preset file");
            }
        }
      };
    }
  };

  // State Updaters
  const updateChannel = (channel: keyof AppState['channels'], newState: ChannelState) => {
    setState(prev => ({ ...prev, channels: { ...prev.channels, [channel]: newState } }));
  };

  const updateRouting = (key: keyof AppState['routing']) => {
    setState(prev => ({ ...prev, routing: { ...prev.routing, [key]: !prev.routing[key] } }));
  };

  const updateAmp = (key: keyof AppState['amp'], val: number) => {
    setState(prev => ({ ...prev, amp: { ...prev.amp, [key]: val }}));
  };

  const updateImagerBand = (id: number, key: keyof ImagerBand, val: number) => {
    setState(prev => ({
      ...prev,
      imager: {
        ...prev.imager,
        bands: prev.imager.bands.map(b => b.id === id ? { ...b, [key]: val } : b)
      }
    }));
  };

  const Header = () => (
    <div className="flex flex-wrap justify-between items-center mb-6 px-1 gap-4">
      <div className="flex items-center gap-3">
        {/* Logo Mark */}
        <div className="w-10 h-10 bg-gray-900 rounded-lg flex items-center justify-center shadow-lg border border-gray-700">
            <div className="w-5 h-5 border-2 border-white rounded-full"></div>
        </div>
        <div>
            <h1 className="text-xl font-black tracking-tight text-gray-800 leading-none">UTT</h1>
            <span className="text-[10px] text-gray-500 font-bold tracking-[0.2em]">MODULAR TERMINAL</span>
        </div>
      </div>
      
      <div className="flex gap-4 items-center">
        {/* Presets */}
        <div className="flex items-center gap-1">
            <input type="file" ref={fileInputRef} onChange={handleLoadPreset} className="hidden" accept=".json"/>
            <TooltipWrapper label="Import" description="Load Preset File">
                <button onClick={() => fileInputRef.current?.click()} className="text-gray-500 hover:text-gray-900 p-2 hover:bg-white rounded transition-colors"><Upload size={16}/></button>
            </TooltipWrapper>
            <TooltipWrapper label="Export" description="Save Preset File">
                <button onClick={handleSavePreset} className="text-gray-500 hover:text-gray-900 p-2 hover:bg-white rounded transition-colors"><Save size={16}/></button>
            </TooltipWrapper>
        </div>
        
        <div className="w-[1px] h-6 bg-gray-300"></div>

        {/* A/B System */}
        <div className="flex items-center gap-1 bg-gray-200/50 p-1 rounded-lg border border-gray-200 shadow-inner">
            <TooltipWrapper label="Bank A" description="Select Preset A">
                <button 
                    onClick={() => setActiveSlot('A')}
                    className={`w-8 h-8 text-[10px] font-bold rounded flex items-center justify-center transition-all ${
                        activeSlot === 'A' 
                        ? 'bg-white shadow-sm text-gray-900 border border-gray-200/50' 
                        : 'text-gray-400 hover:text-gray-600'
                    }`}
                >A</button>
            </TooltipWrapper>
            <TooltipWrapper label="Bank B" description="Select Preset B">
                <button 
                    onClick={() => setActiveSlot('B')}
                    className={`w-8 h-8 text-[10px] font-bold rounded flex items-center justify-center transition-all ${
                        activeSlot === 'B' 
                        ? 'bg-white shadow-sm text-gray-900 border border-gray-200/50' 
                        : 'text-gray-400 hover:text-gray-600'
                    }`}
                >B</button>
            </TooltipWrapper>
            <div className="w-[1px] h-4 bg-gray-300 mx-1"></div>
            <TooltipWrapper label="Copy" description={`Copy ${activeSlot} to ${activeSlot === 'A' ? 'B' : 'A'}`}>
                <button 
                    onClick={copyState}
                    className="w-8 h-8 flex items-center justify-center text-gray-500 hover:text-gray-900 hover:bg-white hover:shadow-sm rounded transition-all"
                >
                    <Copy size={12} />
                </button>
            </TooltipWrapper>
        </div>
      </div>
    </div>
  );

  return (
    <div className="min-h-screen bg-[#e5e5e5] p-4 md:p-8 font-sans flex items-center justify-center">
      {/* Main Chassis - Responsive Width */}
      <div className="w-full max-w-[1400px] bg-matte panel-shadow rounded-xl p-6 md:p-8 relative border border-white/50">
        
        {/* Industrial Screws */}
        <div className="absolute top-3 left-3 w-3 h-3 rounded-full bg-gradient-to-br from-gray-200 to-gray-400 border border-gray-400 shadow-sm flex items-center justify-center"><div className="w-1.5 h-[1px] bg-gray-500 rotate-45 opacity-50"></div></div>
        <div className="absolute top-3 right-3 w-3 h-3 rounded-full bg-gradient-to-br from-gray-200 to-gray-400 border border-gray-400 shadow-sm flex items-center justify-center"><div className="w-1.5 h-[1px] bg-gray-500 rotate-12 opacity-50"></div></div>
        <div className="absolute bottom-3 left-3 w-3 h-3 rounded-full bg-gradient-to-br from-gray-200 to-gray-400 border border-gray-400 shadow-sm flex items-center justify-center"><div className="w-1.5 h-[1px] bg-gray-500 -rotate-45 opacity-50"></div></div>
        <div className="absolute bottom-3 right-3 w-3 h-3 rounded-full bg-gradient-to-br from-gray-200 to-gray-400 border border-gray-400 shadow-sm flex items-center justify-center"><div className="w-1.5 h-[1px] bg-gray-500 rotate-90 opacity-50"></div></div>

        <Header />

        {/* SIGNAL FLOW LAYOUT */}
        <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
          
          {/* --- STAGE 1: INPUT & SHAPING (Left Column) --- */}
          <div className="lg:col-span-3 flex flex-col gap-6">
             
             {/* 1. Routing Matrix */}
             <div className="bg-matte panel-shadow p-5 rounded-lg border border-gray-200/50">
                <ModuleHeader icon={ArrowLeftRight} title="Input Routing" />
                <div className="grid grid-cols-2 gap-3">
                    <IconButton label="L → R" icon={ArrowLeftRight} active={state.routing.lToR} onClick={() => updateRouting('lToR')} description="Copy Left to Right" />
                    <IconButton label="R → L" icon={ArrowLeftRight} active={state.routing.rToL} onClick={() => updateRouting('rToL')} description="Copy Right to Left" />
                    <IconButton label="M → S" icon={Scaling} active={state.routing.mToS} onClick={() => updateRouting('mToS')} description="Mid to Side" />
                    <IconButton label="S → M" icon={MinusCircle} active={state.routing.sToM} onClick={() => updateRouting('sToM')} description="Side to Mid" />
                    <IconButton label="Swap" icon={ArrowLeftRight} active={state.routing.swapLR} onClick={() => updateRouting('swapLR')} description="Swap L/R" />
                    <IconButton label="Phase" icon={Zap} active={state.routing.phaseInvert} onClick={() => updateRouting('phaseInvert')} description="Phase Invert Right" />
                </div>
             </div>

             {/* 2. Mono Control */}
             <div className="bg-matte panel-shadow p-5 rounded-lg border border-gray-200/50 relative overflow-hidden group hover:border-gray-300 transition-colors">
              <div className="absolute top-0 left-0 w-1 h-full bg-purple-600 opacity-0 group-hover:opacity-100 transition-opacity"></div>
              <ModuleHeader icon={Waves} title="Low End Control" />
              
              <div className="flex flex-col gap-6">
                  <div className="bg-white/50 p-3 rounded border border-gray-200 shadow-sm">
                    <div className="flex justify-between items-center mb-4">
                        <span className="text-[9px] text-gray-500 font-bold tracking-wide">BASS MONO</span>
                        <ToggleSwitch 
                            active={state.bassMono.enabled} 
                            onClick={() => setState(s => ({...s, bassMono: {...s.bassMono, enabled: !s.bassMono.enabled}}))}
                            label="Enable"
                        />
                    </div>
                    <div className="flex justify-center">
                        <Knob 
                        label="X-Over" value={state.bassMono.freq} min={20} max={500} defaultValue={120} unit="Hz"
                        onChange={v => setState(s => ({...s, bassMono: {...s.bassMono, freq: v}}))} 
                        color="#7c3aed" size="md" description="Crossover Frequency"
                        />
                    </div>
                  </div>

                  <div className="flex justify-between items-center px-2 py-3 border-t border-gray-200/60">
                    <div className="flex items-center gap-2 text-gray-600">
                        <Disc size={14} />
                        <span className="text-[10px] font-bold">SUM TO MONO</span>
                    </div>
                    <ToggleSwitch 
                        active={state.master.mono} 
                        onClick={() => setState(s => ({...s, master: {...s.master, mono: !s.master.mono}}))}
                        label="Active"
                    />
                  </div>
              </div>
            </div>

             {/* 3. Saturation */}
            <div className="bg-matte panel-shadow p-5 rounded-lg border border-gray-200/50 relative overflow-hidden group hover:border-gray-300 transition-colors">
              <div className="absolute top-0 left-0 w-1 h-full bg-orange-500 opacity-0 group-hover:opacity-100 transition-opacity"></div>
              <ModuleHeader icon={Zap} title="Analog Color" />
              
              <div className="grid grid-cols-2 gap-y-8 mb-4">
                <Knob 
                  label="Drive" value={state.amp.drive} min={0} max={100} defaultValue={0} 
                  onChange={v => updateAmp('drive', v)} color="#f97316"
                  description="Tube Saturation"
                />
                <Knob 
                  label="Harmonics" value={state.amp.color} min={0} max={100} defaultValue={50} 
                  onChange={v => updateAmp('color', v)} color="#f97316"
                  description="Harmonic Balance"
                />
              </div>
              <div className="flex justify-center pt-4 border-t border-gray-200/60">
                 <Knob 
                    label="Mix" value={state.amp.mix} min={0} max={100} defaultValue={100} unit="%"
                    onChange={v => updateAmp('mix', v)} size="sm" color="#9ca3af"
                    description="Parallel Mix"
                 />
              </div>
            </div>
          </div>

          {/* --- STAGE 2: IMAGING & VISUALIZATION (Center Column) --- */}
          <div className="lg:col-span-6 flex flex-col gap-6">
             
             {/* 1. Visualizer (The Eye) */}
             <div className="recessed p-1 rounded-lg border border-gray-300/50 shadow-inner flex items-center justify-center relative min-h-[350px] flex-grow">
                <div className="w-full h-full bg-white rounded overflow-hidden relative">
                     <div className="absolute inset-0">
                        <Visualizer 
                            width={0} 
                            height={0} 
                            stereoWidth={state.imager.bands[1].width} 
                            gain={state.master.outputGain + state.channels.mid.gain}
                            isMono={state.master.mono || state.routing.lToR || state.routing.rToL}
                            phaseInvert={state.routing.phaseInvert !== (state.channels.left.phaseInvert !== state.channels.right.phaseInvert)}
                        />
                     </div>
                </div>
             </div>

             {/* 2. Multiband Imager */}
             <div className="bg-matte panel-shadow p-5 rounded-lg border border-gray-200/50 relative overflow-hidden group hover:border-gray-300 transition-colors">
                <div className="absolute top-0 left-0 w-1 h-full bg-blue-500 opacity-0 group-hover:opacity-100 transition-opacity"></div>
                <div className="flex justify-between items-center mb-4 border-b border-gray-200/60 pb-2">
                   <div className="flex items-center gap-2">
                        <Settings2 size={14} className="text-gray-400" />
                        <span className="text-[10px] font-bold text-gray-800 uppercase tracking-widest">Stereo Width</span>
                   </div>
                   <ToggleSwitch 
                      active={state.imager.enabled}
                      onClick={() => setState(s => ({...s, imager: {...s.imager, enabled: !s.imager.enabled}}))}
                      label="Active"
                   />
                </div>
                <div className="grid grid-cols-3 gap-4">
                   {state.imager.bands.map((band, idx) => (
                      <div key={band.id} className="bg-white/50 rounded-lg p-3 flex flex-col items-center border border-gray-200 shadow-sm">
                         <span className="text-[9px] font-bold text-gray-500 mb-2 uppercase tracking-wide">
                            {idx === 0 ? 'Low (300Hz)' : idx === 1 ? 'Mid (2.5k)' : 'High (8k)'}
                         </span>
                         <Knob 
                            label="Spread" value={band.width} min={0} max={200} defaultValue={100} unit="%"
                            onChange={(v) => updateImagerBand(band.id, 'width', v)}
                            size="sm" color="#3b82f6"
                         />
                      </div>
                   ))}
                </div>
             </div>
          </div>

          {/* --- STAGE 3: OUTPUT & METERING (Right Column) --- */}
          <div className="lg:col-span-3 flex flex-col gap-6">
             
             {/* 1. Channel Mixer */}
             <div className="grid grid-cols-2 gap-4">
                <ChannelStrip label="LEFT" state={state.channels.left} onChange={(s) => updateChannel('left', s)} />
                <ChannelStrip label="RIGHT" state={state.channels.right} onChange={(s) => updateChannel('right', s)} />
                <ChannelStrip label="MID" state={state.channels.mid} onChange={(s) => updateChannel('mid', s)} />
                <ChannelStrip label="SIDE" state={state.channels.side} onChange={(s) => updateChannel('side', s)} />
             </div>
             
             {/* 2. Loudness Monitor */}
             <LoudnessMonitor gain={state.master.outputGain} />

             {/* 3. Master Output */}
             <div className="bg-gray-900 p-5 rounded-lg border border-gray-800 shadow-lg flex justify-between items-center relative overflow-hidden mt-auto">
                <div className="absolute inset-0 opacity-10 bg-[url('https://www.transparenttextures.com/patterns/dark-leather.png')]"></div>
                
                <div className="flex flex-col relative z-10">
                    <span className="text-[10px] font-bold text-gray-400 tracking-widest mb-1">MASTER</span>
                    <span className="text-lg font-mono text-white tracking-tighter">{state.master.outputGain > 0 ? '+' : ''}{state.master.outputGain.toFixed(1)} <span className="text-sm text-gray-500">dB</span></span>
                </div>
                <div className="relative z-10">
                    <Knob 
                    label="Output" value={state.master.outputGain} min={-60} max={12} defaultValue={0} unit=""
                    onChange={v => setState(s => ({...s, master: {...s.master, outputGain: v}}))}
                    color="#ffffff" size="md" description="Main Output Level"
                    />
                </div>
             </div>
          </div>

        </div>
      </div>
    </div>
  );
}