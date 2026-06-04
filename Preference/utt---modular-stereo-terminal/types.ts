export interface ChannelState {
  gain: number; // -60 to +12 dB
  pan: number; // -100 to +100
  muted: boolean;
  solo: boolean;
  phaseInvert: boolean;
}

export interface ImagerBand {
  id: number;
  freq: number; // Crossover frequency
  width: number; // 0% to 200%
  gain: number; // +/- 12dB
}

export interface AppState {
  // Routing
  routing: {
    lToR: boolean; // Left replaces Right
    rToL: boolean; // Right replaces Left
    mToS: boolean; // Mid copies to Side
    sToM: boolean; // Side copies to Mid
    swapLR: boolean; // Swap Left and Right
    phaseInvert: boolean; // Global phase invert (polarity flip one channel)
  };
  
  // Channels
  channels: {
    left: ChannelState;
    right: ChannelState;
    mid: ChannelState;
    side: ChannelState;
  };

  // Processing
  bassMono: {
    enabled: boolean;
    freq: number; // Hz, e.g., 20Hz - 500Hz
  };
  
  amp: {
    drive: number; // 0 - 100
    color: number; // 0 - 100
    mix: number; // 0 - 100
  };

  imager: {
    enabled: boolean;
    bands: ImagerBand[];
  };

  master: {
    inputGain: number;
    outputGain: number;
    mono: boolean;
  };
}

export const INITIAL_STATE: AppState = {
  routing: {
    lToR: false,
    rToL: false,
    mToS: false,
    sToM: false,
    swapLR: false,
    phaseInvert: false,
  },
  channels: {
    left: { gain: 0, pan: -50, muted: false, solo: false, phaseInvert: false },
    right: { gain: 0, pan: 50, muted: false, solo: false, phaseInvert: false },
    mid: { gain: 0, pan: 0, muted: false, solo: false, phaseInvert: false },
    side: { gain: 0, pan: 0, muted: false, solo: false, phaseInvert: false },
  },
  bassMono: {
    enabled: true,
    freq: 120,
  },
  amp: {
    drive: 0,
    color: 50,
    mix: 100,
  },
  imager: {
    enabled: true,
    bands: [
      { id: 1, freq: 300, width: 100, gain: 0 },
      { id: 2, freq: 2500, width: 100, gain: 0 },
      { id: 3, freq: 8000, width: 100, gain: 0 },
    ],
  },
  master: {
    inputGain: 0,
    outputGain: 0,
    mono: false,
  },
};