import { PluginParameters, AudioStats, MonoToStereoMode, MonoMode } from './types';

export class UTTAudioEngine {
  public ctx: AudioContext | null = null;
  private isInitialized = false;

  // Nodes
  private inputNode: GainNode | null = null;
  private dcSourceL: ConstantSourceNode | null = null;
  private dcSourceR: ConstantSourceNode | null = null;
  private processorNode: ScriptProcessorNode | null = null;
  private outputNode: GainNode | null = null;

  // Audio Sources
  private sineOsc: OscillatorNode | null = null;
  private noiseNode: AudioWorkletNode | ScriptProcessorNode | null = null;
  private synthInterval: number | null = null;
  private fileSource: AudioBufferSourceNode | null = null;
  private micSource: MediaStreamAudioSourceNode | null = null;
  private activeMediaStream: MediaStream | null = null;

  private currentSourceType: 'silent' | 'sine' | 'noise' | 'synth' | 'mic' | 'file' = 'silent';
  private uploadedAudioBuffer: AudioBuffer | null = null;

  // DSP State Variables (stored for filtering and delay line)
  private sampleRate = 44100;
  private delayBufferL = new Float32Array(192000); // Support up to 4 seconds delay at 48k
  private delayBufferR = new Float32Array(192000);
  private delayWriteIndex = 0;

  // DC Filter memory (feedback state)
  private prevInputL = 0;
  private prevInputR = 0;
  private prevFilteredL = 0;
  private prevFilteredR = 0;

  // LUFS filter state (K-weighting approximation - shelving & highpass)
  // Stage 1 (High Shelving): Filter coefficients for HS
  // Stage 2 (Highpass): Filter coefficients for HP
  private lufsDbL = -120;
  private lufsDbR = -120;
  private lufsMomentaryAccumulator: number[] = [];
  private lufsShortTermAccumulator: number[] = [];
  private lufsShortTermMaxSamples = 120; // about 3 seconds (each block ~24ms at 2048 buffersize, 44.1k)
  private lufsMomentaryMaxSamples = 16;  // about 400ms

  // Instant Recording State
  private mediaRecorder: MediaRecorder | null = null;
  private recordedChunks: Blob[] = [];
  private recordingStartTime = 0;

  // Live Analysis buffers
  public analL: AnalyserNode | null = null;
  public analR: AnalyserNode | null = null;

  // Stats Callback
  private onStatsCallback: ((stats: AudioStats) => void) | null = null;

  // Current Parameters
  private params: PluginParameters = {
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
  };

  constructor() {
    // Lazy initialisation on first interaction (standard browser policy)
  }

  public init() {
    if (this.isInitialized) return;

    const AudioContextClass = window.AudioContext || (window as any).webkitAudioContext;
    if (!AudioContextClass) {
      console.error('Web Audio API is not supported in this browser.');
      return;
    }

    this.ctx = new AudioContextClass();
    this.sampleRate = this.ctx.sampleRate;

    // Create central processing chain
    this.inputNode = this.ctx.createGain();
    this.outputNode = this.ctx.createGain();

    // DC Injection Nodes
    this.dcSourceL = this.ctx.createConstantSource();
    this.dcSourceR = this.ctx.createConstantSource();
    this.dcSourceL.offset.value = 0;
    this.dcSourceR.offset.value = 0;
    this.dcSourceL.start();
    this.dcSourceR.start();

    // Split and merge DC to insert offset selectively
    const dcSplitter = this.ctx.createChannelSplitter(2);
    const dcMerger = this.ctx.createChannelMerger(2);
    this.dcSourceL.connect(dcMerger, 0, 0); // Connect L to left input
    this.dcSourceR.connect(dcMerger, 0, 1); // Connect R to right input
    dcMerger.connect(this.inputNode);

    // Main DSP block (using standard ScriptProcessorNode for synchronous float32 calculations without files)
    // 2048 buffer size is a perfect balance between responsiveness and DSP calculation accuracy
    this.processorNode = this.ctx.createScriptProcessor(2048, 2, 2);
    this.processorNode.onaudioprocess = (e) => this.process(e);

    // Post-DSP Analyzers for oscilloscope and goniometer
    this.analL = this.ctx.createAnalyser();
    this.analR = this.ctx.createAnalyser();
    this.analL.fftSize = 2048;
    this.analR.fftSize = 2048;

    const mainSplitter = this.ctx.createChannelSplitter(2);
    this.processorNode.connect(mainSplitter);

    mainSplitter.connect(this.analL, 0);
    mainSplitter.connect(this.analR, 1);

    // Reconstruct output
    const mainMerger = this.ctx.createChannelMerger(2);
    mainSplitter.connect(mainMerger, 0, 0);
    mainSplitter.connect(mainMerger, 1, 1);

    // Route fully through the output
    mainMerger.connect(this.outputNode);
    this.outputNode.connect(this.ctx.destination);

    // Connect input to processor
    this.inputNode.connect(this.processorNode);

    this.isInitialized = true;
    console.log('UTTAudioEngine Initialized. Sample Rate:', this.sampleRate);
  }

  public resume() {
    if (this.ctx && this.ctx.state === 'suspended') {
      this.ctx.resume();
    }
  }

  public updateParams(newParams: Partial<PluginParameters>) {
    this.params = { ...this.params, ...newParams };

    // Update DC Injection nodes dynamically
    if (this.dcSourceL) {
      this.dcSourceL.offset.setValueAtTime(this.params.dcOffsetL, this.ctx?.currentTime || 0);
    }
    if (this.dcSourceR) {
      this.dcSourceR.offset.setValueAtTime(this.params.dcOffsetR, this.ctx?.currentTime || 0);
    }
  }

  public registerStatsCallback(callback: (stats: AudioStats) => void) {
    this.onStatsCallback = callback;
  }

  // --- Core DSP process loop ---
  private process(e: AudioProcessingEvent) {
    const inputBuffer = e.inputBuffer;
    const outputBuffer = e.outputBuffer;

    const inL = inputBuffer.getChannelData(0);
    const inR = inputBuffer.getChannelData(1);
    const outL = outputBuffer.getChannelData(0);
    const outR = outputBuffer.getChannelData(1);

    const length = inputBuffer.length;

    // Gain Conversion (dB to linear multiplier)
    const linearGain = this.params.bypass ? 1 : Math.pow(10, this.params.gain / 20);

    // Pan Angle Calculators (using Constant Power Panning)
    const pVal = this.params.pan; // -1 to +1
    const angle = ((pVal + 1) * Math.PI) / 4;
    const panL = Math.cos(angle) * Math.sqrt(2);
    const panR = Math.sin(angle) * Math.sqrt(2);

    // DC High-pass Filter Parameter variables
    // Simple 1st-order Filter Alpha = RC / (RC + dt)
    // Cut-off ~ 15Hz
    const cutoff = 15;
    const RC = 1 / (2 * Math.PI * cutoff);
    const dt = 1 / this.sampleRate;
    const alpha = RC / (RC + dt);

    // Delay parameters for sample offset delay
    const msToSamples = this.sampleRate / 1000;
    const offsetSamples = Math.round(this.params.phaseOffset * msToSamples); // Positive: L delayed, Negative: R delayed
    const maxDelay = 192000;

    let peakL = 0;
    let peakR = 0;
    let sumL2 = 0;
    let sumR2 = 0;
    let sumLR = 0;

    // K-Weighting DSP Filter Memory
    // High-pass filter stage 1 approximation (100Hz)
    // High-shelving filter stage 2 approximation (1.5kHz, +4dB)
    // To minimize memory overhead, we implement simple 1st-order approximation loops
    // Store localized instances for speed optimization
    let kPrevInL = 0;
    let kPrevInR = 0;
    let kPrevOutL = 0;
    let kPrevOutR = 0;
    const kAlphaHP = 0.985; // Approx 100Hz cutoff at 44.1kHz
    const kShellGain = 1.58; // +4dB linear

    for (let i = 0; i < length; i++) {
      let xL = inL[i];
      let xR = inR[i];

      // Safe check for NaN/Infinity
      if (isNaN(xL) || !isFinite(xL)) xL = 0;
      if (isNaN(xR) || !isFinite(xR)) xR = 0;

      // 1. DC Filter (Bypassable 10Hz High-pass filtering)
      if (!this.params.bypass && this.params.dcFilter) {
        const xlFiltered = alpha * (this.prevFilteredL + xL - this.prevInputL);
        const xrFiltered = alpha * (this.prevFilteredR + xR - this.prevInputR);

        this.prevInputL = xL;
        this.prevInputR = xR;
        this.prevFilteredL = xlFiltered;
        this.prevFilteredR = xrFiltered;

        xL = xlFiltered;
        xR = xrFiltered;
      } else {
        this.prevInputL = xL;
        this.prevInputR = xR;
      }

      // 2. Channel phase delays (Sample Offset Delay Line)
      // Save current samples into delay buffers
      this.delayBufferL[this.delayWriteIndex] = xL;
      this.delayBufferR[this.delayWriteIndex] = xR;

      if (!this.params.bypass && offsetSamples !== 0) {
        if (offsetSamples > 0) {
          // Delay Left channel (Fetch from ring buffer behind)
          const readIndexL = (this.delayWriteIndex - offsetSamples + maxDelay) % maxDelay;
          xL = this.delayBufferL[readIndexL];
        } else {
          // Delay Right channel (offsetSamples is negative, hence absolute subtraction)
          const readIndexR = (this.delayWriteIndex - Math.abs(offsetSamples) + maxDelay) % maxDelay;
          xR = this.delayBufferR[readIndexR];
        }
      }

      this.delayWriteIndex = (this.delayWriteIndex + 1) % maxDelay;

      // Ensure no processing if bypassed
      if (this.params.bypass) {
        outL[i] = xL;
        outR[i] = xR;
        // Simple metric analysis even under bypass for accurate visual reflection
        const absL = Math.abs(xL);
        const absR = Math.abs(xR);
        if (absL > peakL) peakL = absL;
        if (absR > peakR) peakR = absR;
        sumL2 += xL * xL;
        sumR2 += xR * xR;
        sumLR += xL * xR;
        continue;
      }

      // 3. Phase Inversions
      if (this.params.phaseInvL) xL = -xL;
      if (this.params.phaseInvR) xR = -xR;

      // 4. L/R Swap Channels
      if (this.params.swapChannels) {
        const temp = xL;
        xL = xR;
        xR = temp;
      }

      // 5. Mono To Stereo Copy
      if (this.params.monoToStereo === 'L_TO_BOTH') {
        xR = xL;
      } else if (this.params.monoToStereo === 'R_TO_BOTH') {
        xL = xR;
      }

      // 6. Mono Modes / Mid-Side Splitting & Width Control
      const mid = (xL + xR) * 0.5;
      let side = (xL - xR) * 0.5;

      // Apply separation width (0 = Mono, 1 = Raw Stereo, 2 = Ultra Wide)
      side *= this.params.width;

      let processedL = mid + side;
      let processedR = mid - side;

      // Handle explicit mono sub-modes routing
      if (this.params.monoMode === 'MONO') {
        processedL = mid;
        processedR = mid;
      } else if (this.params.monoMode === 'LEFT_ONLY') {
        processedL = processedL;
        processedR = 0; // Solo left channel
      } else if (this.params.monoMode === 'RIGHT_ONLY') {
        processedL = 0;
        processedR = processedR; // Solo right channel
      } else if (this.params.monoMode === 'MID_ONLY') {
        processedL = mid;
        processedR = mid;
      } else if (this.params.monoMode === 'SIDE_ONLY') {
        processedL = side;
        processedR = -side; // Out of phase side signal routing
      }

      // 7. Pan & Master Gain
      let finalL = processedL * panL * linearGain;
      let finalR = processedR * panR * linearGain;

      // Apply dynamic Mute toggle
      if (this.params.mute) {
        finalL = 0;
        finalR = 0;
      }

      // Output values assignment
      outL[i] = finalL;
      outR[i] = finalR;

      // Peak & Energy analysis calculations
      const absL = Math.abs(finalL);
      const absR = Math.abs(finalR);

      if (absL > peakL) peakL = absL;
      if (absR > peakR) peakR = absR;

      sumL2 += finalL * finalL;
      sumR2 += finalR * finalR;
      sumLR += finalL * finalR;

      // K-Weighting filter approximation for LUFS
      // Step A: High-pass (approx Stage 2) to eliminate sub-lows
      const filteredKL = kAlphaHP * (kPrevOutL + finalL - kPrevInL);
      const filteredKR = kAlphaHP * (kPrevOutR + finalR - kPrevInR);
      kPrevInL = finalL;
      kPrevInR = finalR;
      kPrevOutL = filteredKL;
      kPrevOutR = filteredKR;

      // Step B: Shelving (Stage 1) - boost highs slightly
      const shellL = filteredKL * kShellGain;
      const shellR = filteredKR * kShellGain;

      // Accumulate energy sample block
      this.lufsDbL += shellL * shellL;
      this.lufsDbR += shellR * shellR;
    }

    // Peak levels to decibel conversion (dBFS)
    const dbToFS = (linear: number) => (linear > 0.00001 ? 20 * Math.log10(linear) : -120);
    const dbL = dbToFS(peakL);
    const dbR = dbToFS(peakR);

    // RMS to decibel conversion
    const rmsValL = Math.sqrt(sumL2 / length);
    const rmsValR = Math.sqrt(sumR2 / length);
    const dbRmsL = dbToFS(rmsValL);
    const dbRmsR = dbToFS(rmsValR);

    // Phase Correlation metric Calculation: SUM(L * R) / SQRT(SUM(L^2) * SUM(R^2))
    const denominator = Math.sqrt(sumL2 * sumR2);
    const corr = denominator > 0.00001 ? sumLR / denominator : 1;

    // LUFS calculations based on standard block energy sums
    const meanEnergyL = this.lufsDbL / length;
    const meanEnergyR = this.lufsDbR / length;
    // reset LUFS accumulators
    this.lufsDbL = 0;
    this.lufsDbR = 0;

    // ITU-R BS.1770 weight: L=1.0, R=1.0. Channel energy sum:
    const totalEnergy = meanEnergyL + meanEnergyR;
    // LUFS calculation: -0.691 + 10 * log10(TotalEnergy)
    const currentBlockLufs = totalEnergy > 0.000000001 ? -0.691 + 10 * Math.log10(totalEnergy) : -120;

    // Momentary LUFS sliding averaging
    this.lufsMomentaryAccumulator.push(currentBlockLufs);
    if (this.lufsMomentaryAccumulator.length > this.lufsMomentaryMaxSamples) {
      this.lufsMomentaryAccumulator.shift();
    }
    const momentaryMeanEnergy = this.lufsMomentaryAccumulator.reduce((sum, v) => sum + Math.pow(10, v / 10), 0) / this.lufsMomentaryAccumulator.length;
    const lufsMomentary = momentaryMeanEnergy > 0 ? 10 * Math.log10(momentaryMeanEnergy) : -120;

    // Short-term LUFS sliding averaging (over ~3 seconds)
    this.lufsShortTermAccumulator.push(currentBlockLufs);
    if (this.lufsShortTermAccumulator.length > this.lufsShortTermMaxSamples) {
      this.lufsShortTermAccumulator.shift();
    }
    const shortTermMeanEnergy = this.lufsShortTermAccumulator.reduce((sum, v) => sum + Math.pow(10, v / 10), 0) / this.lufsShortTermAccumulator.length;
    const lufsShortTerm = shortTermMeanEnergy > 0 ? 10 * Math.log10(shortTermMeanEnergy) : -120;

    // Estimated integrated LUFS over entire run
    // For local visualizer dashboard, we average Short-Term results elegantly
    const lufsIntegrated = lufsShortTerm;

    // Calculate DC offset from first and last points, or mean value (to display DC bias)
    const calculatedDCL = inL.reduce((sum, v) => sum + v, 0) / length;
    const calculatedDCR = inR.reduce((sum, v) => sum + v, 0) / length;

    // Dispatch telemetry data to GUI
    if (this.onStatsCallback) {
      this.onStatsCallback({
        peakL: dbL,
        peakR: dbR,
        rmsL: dbRmsL,
        rmsR: dbRmsR,
        correlation: isNaN(corr) ? 0 : Math.max(-1, Math.min(1, corr)),
        lufsMomentary: Math.max(-120, Math.min(0, lufsMomentary)),
        lufsShortTerm: Math.max(-120, Math.min(0, lufsShortTerm)),
        lufsIntegrated: Math.max(-120, Math.min(0, lufsIntegrated)),
        dcOffsetL_current: calculatedDCL,
        dcOffsetR_current: calculatedDCR,
      });
    }
  }

  // --- Sound Sources Control ---
  public playSource(type: 'silent' | 'sine' | 'noise' | 'synth' | 'mic' | 'file') {
    this.init();
    this.resume();
    this.stopAllSources();

    this.currentSourceType = type;

    if (!this.ctx || !this.inputNode) return;

    if (type === 'sine') {
      this.sineOsc = this.ctx.createOscillator();
      this.sineOsc.type = 'sine';
      this.sineOsc.frequency.value = 1000; // standard testing pitch
      this.sineOsc.connect(this.inputNode);
      this.sineOsc.start();
    } else if (type === 'noise') {
      // Create white noise dynamically using script processor
      this.noiseNode = this.ctx.createScriptProcessor(4096, 0, 2);
      this.noiseNode.onaudioprocess = (e) => {
        const outL = e.outputBuffer.getChannelData(0);
        const outR = e.outputBuffer.getChannelData(1);
        for (let i = 0; i < e.outputBuffer.length; i++) {
          outL[i] = Math.random() * 2 - 1;
          outR[i] = Math.random() * 2 - 1;
        }
      };
      this.noiseNode.connect(this.inputNode);
    } else if (type === 'synth') {
      // Stereo Phase Sweep Synthesizer: Play an automatic retro baseline arpeggio
      // that sweeps Left & Right, introducing phase shifts to demonstrate goniometer / phase correlation
      let stepCount = 0;
      const pitches = [110, 137.5, 165, 220, 275, 330, 440, 550]; // Beautiful harmonic chords (A minor triad)

      const playSynthStep = () => {
        if (!this.ctx || !this.inputNode || this.currentSourceType !== 'synth') return;

        const now = this.ctx.currentTime;
        const o1 = this.ctx.createOscillator();
        const o2 = this.ctx.createOscillator();
        const pitch = pitches[stepCount % pitches.length];

        o1.type = 'sawtooth';
        o2.type = 'triangle';

        // Set frequencies
        o1.frequency.value = pitch;
        // Slightly detune to create stereo chorus beating side energy
        o2.frequency.value = pitch * 1.01;

        const noteGain = this.ctx.createGain();
        const stereoPan = this.ctx.createStereoPanner();

        // Automatic sweep on panning (-0.8 to +0.8) and phase shifts
        const panningValue = Math.sin(stepCount * 0.4) * 0.7;
        stereoPan.pan.value = panningValue;

        noteGain.gain.setValueAtTime(0.001, now);
        noteGain.gain.linearRampToValueAtTime(0.2, now + 0.05);
        noteGain.gain.exponentialRampToValueAtTime(0.001, now + 0.35);

        o1.connect(noteGain);
        o2.connect(noteGain);
        noteGain.connect(stereoPan);
        stereoPan.connect(this.inputNode);

        o1.start(now);
        o2.start(now);
        o1.stop(now + 0.4);
        o2.stop(now + 0.4);

        stepCount++;
      };

      playSynthStep(); // Run immediate
      this.synthInterval = window.setInterval(playSynthStep, 400);
    } else if (type === 'mic') {
      // Microphones Capture
      navigator.mediaDevices.getUserMedia({ audio: { echoCancellation: false, noiseSuppression: false } })
        .then((stream) => {
          this.activeMediaStream = stream;
          if (this.ctx && this.inputNode) {
            this.micSource = this.ctx.createMediaStreamSource(stream);
            this.micSource.connect(this.inputNode);
          }
        })
        .catch((err) => {
          console.error('Failed to access microphone:', err);
          alert('无法获取麦克风输入，请在浏览器中允许麦克风权限。');
          this.currentSourceType = 'silent';
        });
    } else if (type === 'file') {
      if (this.uploadedAudioBuffer) {
        this.playBuffer(this.uploadedAudioBuffer);
      } else {
        alert('请先上传本地音频文件！');
        this.currentSourceType = 'silent';
      }
    }
  }

  public playBuffer(buffer: AudioBuffer) {
    if (!this.ctx || !this.inputNode) return;
    this.stopAllSources();
    this.currentSourceType = 'file';

    this.fileSource = this.ctx.createBufferSource();
    this.fileSource.buffer = buffer;
    this.fileSource.loop = true;
    this.fileSource.connect(this.inputNode);
    this.fileSource.start();
  }

  public uploadAudioFile(file: File): Promise<AudioBuffer> {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = () => {
        this.init();
        const arrayBuffer = reader.result as ArrayBuffer;
        if (this.ctx) {
          this.ctx.decodeAudioData(arrayBuffer, (decodedBuffer) => {
            this.uploadedAudioBuffer = decodedBuffer;
            resolve(decodedBuffer);
          }, (err) => {
            reject(err);
          });
        } else {
          reject(new Error('Audio engine not initialized'));
        }
      };
      reader.onerror = () => {
        reject(reader.error);
      };
      reader.readAsArrayBuffer(file);
    });
  }

  public stopAllSources() {
    if (this.sineOsc) {
      try { this.sineOsc.stop(); } catch (e) {}
      this.sineOsc.disconnect();
      this.sineOsc = null;
    }
    if (this.noiseNode) {
      this.noiseNode.disconnect();
      this.noiseNode = null;
    }
    if (this.synthInterval) {
      clearInterval(this.synthInterval);
      this.synthInterval = null;
    }
    if (this.fileSource) {
      try { this.fileSource.stop(); } catch (e) {}
      this.fileSource.disconnect();
      this.fileSource = null;
    }
    if (this.micSource) {
      this.micSource.disconnect();
      this.micSource = null;
    }
    if (this.activeMediaStream) {
      this.activeMediaStream.getTracks().forEach(track => track.stop());
      this.activeMediaStream = null;
    }
    this.currentSourceType = 'silent';
  }

  // --- Instant Recorder Functionality ---
  public startRecording() {
    this.init();
    this.resume();

    if (!this.processorNode) {
      console.error('Audio engine processor not initialized');
      return;
    }

    this.recordedChunks = [];
    this.recordingStartTime = Date.now();

    // To record the direct outputs of our processor, we can use MediaStreamAudioDestinationNode
    try {
      const dest = this.ctx!.createMediaStreamDestination();
      this.processorNode.connect(dest);

      // MediaRecorder setup
      this.mediaRecorder = new MediaRecorder(dest.stream);
      this.mediaRecorder.ondataavailable = (event) => {
        if (event.data.size > 0) {
          this.recordedChunks.push(event.data);
        }
      };

      this.mediaRecorder.start();
    } catch (e) {
      console.error('Recording initialization failed:', e);
    }
  }

  public stopRecording(): Promise<{ url: string; duration: number; buffer: AudioBuffer }> {
    return new Promise((resolve, reject) => {
      if (!this.mediaRecorder) {
        reject(new Error('Recorder not started'));
        return;
      }

      this.mediaRecorder.onstop = () => {
        const blob = new Blob(this.recordedChunks, { type: 'audio/webm; codecs=opus' });
        const url = URL.createObjectURL(blob);
        const duration = (Date.now() - this.recordingStartTime) / 1000;

        // Decode recorded bytes so user can play it back locally
        const reader = new FileReader();
        reader.onload = () => {
          const arrayBuffer = reader.result as ArrayBuffer;
          this.ctx!.decodeAudioData(arrayBuffer, (decodedBuffer) => {
            resolve({ url, duration, buffer: decodedBuffer });
          }, (err) => {
            console.error('Error decoding recorded audio data:', err);
            reject(err);
          });
        };
        reader.readAsArrayBuffer(blob);
      };

      this.mediaRecorder.stop();
    });
  }

  // Cleanup
  public destroy() {
    this.stopAllSources();
    if (this.ctx) {
      this.ctx.close();
      this.ctx = null;
    }
    this.isInitialized = false;
  }
}
