/**
 * UTT (Universal Utility Tool) VST3 Plugin Types
 */

export type MonoMode = 'STEREO' | 'MONO' | 'LEFT_ONLY' | 'RIGHT_ONLY' | 'MID_ONLY' | 'SIDE_ONLY';
export type MonoToStereoMode = 'OFF' | 'L_TO_BOTH' | 'R_TO_BOTH';

export interface PluginParameters {
  gain: number;          // -48 dB to +24 dB
  pan: number;           // -1 to 1 (left to right)
  width: number;         // 0 to 2 (0: Mono, 1: Normal, 2: 200% super-wide)
  phaseInvL: boolean;    // Invert left channel phase
  phaseInvR: boolean;    // Invert right channel phase
  phaseOffset: number;   // Delay left (+) or right (-) channel in milliseconds (-50ms to 50ms)
  swapChannels: boolean; // Swap Left and Right channels
  monoMode: MonoMode;    // Channel routing / separation mode
  monoToStereo: MonoToStereoMode; // Copy left/right to both
  dcFilter: boolean;     // 10Hz highpass filter to remove DC offset
  dcOffsetL: number;     // Mock/Add continuous DC current level left (-0.5 to 0.5)
  dcOffsetR: number;     // Mock/Add continuous DC current level right (-0.5 to 0.5)
  mute: boolean;
  bypass: boolean;
}

export interface AudioStats {
  peakL: number;         // dBFS (-inf to 0)
  peakR: number;         // dBFS
  rmsL: number;          // dBFS
  rmsR: number;          // dBFS
  correlation: number;   // -1 to +1 (phase correlation index)
  lufsMomentary: number; // LUFS
  lufsShortTerm: number; // LUFS
  lufsIntegrated: number;// LUFS
  dcOffsetL_current: number; // calculated DC offset L
  dcOffsetR_current: number; // calculated DC offset R
}

export type AutomationTarget = keyof Pick<PluginParameters, 'gain' | 'pan' | 'width' | 'phaseOffset'>;

export interface AutomationPoint {
  time: number; // normalized 0 to 1 inside the loop
  value: number; // mapped parameter value
}

export interface AutomationLane {
  target: AutomationTarget;
  enabled: boolean;
  points: AutomationPoint[];
}

export interface LFOConfig {
  target: AutomationTarget | 'none';
  shape: 'SINE' | 'TRIANGLE' | 'SAW' | 'SQUARE' | 'RANDOM';
  frequency: number; // Hz (0.1 to 10)
  amount: number; // Modulation depth (0 to 1)
  active: boolean;
}

export interface RecordingState {
  isRecording: boolean;
  duration: number; // in seconds
  audioUrl: string | null;
  recordedBuffer: AudioBuffer | null;
}

export interface BuiltInAudioSource {
  id: string;
  name: string;
  type: 'synth' | 'noise' | 'mic' | 'file';
}
