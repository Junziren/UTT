import React, { useState, useEffect } from 'react';
import { Circle, Square, Play, Download, Trash2, AudioWaveform as Waveform } from 'lucide-react';

interface TapeRecorderProps {
  isRecording: boolean;
  onStartRecord: () => void;
  onStopRecord: () => void;
  recordedUrl: string | null;
  recordedDuration: number;
  onPlayRecording: () => void;
  onDeleteRecording: () => void;
}

export const TapeRecorder: React.FC<TapeRecorderProps> = ({
  isRecording,
  onStartRecord,
  onStopRecord,
  recordedUrl,
  recordedDuration,
  onPlayRecording,
  onDeleteRecording
}) => {
  const [timer, setTimer] = useState(0);

  useEffect(() => {
    let interval: number | null = null;
    if (isRecording) {
      setTimer(0);
      interval = window.setInterval(() => {
        setTimer(t => t + 1);
      }, 1000);
    } else {
      if (interval) clearInterval(interval);
    }

    return () => {
      if (interval) clearInterval(interval);
    };
  }, [isRecording]);

  const formatTimer = (seconds: number) => {
    const mins = Math.floor(seconds / 60);
    const secs = seconds % 60;
    return `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
  };

  return (
    <div className="bg-neutral-900 border-2 border-neutral-800 rounded-md p-4 mt-4 shadow-md text-slate-300" id="plugin-tape-recorder">
      <div className="flex items-center justify-between border-b border-neutral-800 pb-2.5 mb-3" id="tape-header">
        <div className="font-mono text-xs font-bold uppercase tracking-wider text-[#00FF41] flex items-center gap-2">
          <Waveform className="w-4 h-4 text-[#00FF41]" />
          即时串流声音录制 (Master Out Tape Recorder)
        </div>
        <div className="text-[10px] font-mono text-neutral-500">
          以 WebM / Opus 实时格式抓取 UTT 通道条处理后的立体声信号
        </div>
      </div>

      <div className="flex flex-col md:flex-row items-center justify-between gap-4" id="tape-body">
        {/* Vintage Cassette Tape Visualizer */}
        <div className="relative w-full md:w-56 h-28 bg-neutral-950 rounded-lg border border-neutral-800 overflow-hidden flex flex-col justify-between p-2 shadow-inner" id="visual-cassette">
          {/* Cassette faceplate text detail */}
          <div className="flex justify-between font-mono text-[8px] text-neutral-600">
            <span>UTT-C90 CHROMIUM EXTRA</span>
            <span>HIGH OUTPUT</span>
          </div>

          {/* Spinner Wheels */}
          <div className="flex justify-center items-center space-x-12 my-1" id="cassette-wheels">
            {/* Left wheel reel */}
            <div 
              className={`w-11 h-11 rounded-full border-4 border-dashed border-[#00FF41]/25 bg-neutral-900 flex items-center justify-center ${isRecording ? 'animate-spin' : ''}`}
              style={{ animationDuration: '4s' }}
            >
              <div className="w-4 h-4 rounded-full bg-neutral-950 border border-[#00FF41]/30" />
            </div>
            {/* Right wheel reel */}
            <div 
              className={`w-11 h-11 rounded-full border-4 border-dashed border-[#00FF41]/25 bg-neutral-900 flex items-center justify-center ${isRecording ? 'animate-spin' : ''}`}
              style={{ animationDuration: '4s' }}
            >
              <div className="w-4 h-4 rounded-full bg-neutral-950 border border-[#00FF41]/30" />
            </div>
          </div>

          {/* Retro green segment digital meter timer */}
          <div className="flex justify-between items-end font-mono" id="cassette-footer">
            <span className="text-[8px] text-neutral-600">LN RECORDING DEPTH</span>
            <span className="text-xs bg-[#00FF41]/10 border border-[#00FF41]/20 px-2 py-0.5 rounded text-[#00FF41] font-semibold shadow-inner">
              {isRecording ? formatTimer(timer) : recordedUrl ? `已录制: ${recordedDuration.toFixed(1)}s` : '00:00'}
            </span>
          </div>
        </div>

        {/* Cassette Controls Section */}
        <div className="flex-1 w-full space-y-3" id="recorder-buttons-deck">
          <div className="flex flex-wrap gap-2.5 items-center justify-center md:justify-start">
            {!isRecording ? (
              <button
                onClick={onStartRecord}
                id="btn-tape-record"
                className="px-4 py-2 bg-gradient-to-r from-red-650 to-red-700 hover:from-red-500 hover:to-red-600 text-white font-mono text-xs font-bold rounded shadow-md border border-red-800/80 hover:scale-102 active:scale-98 transition flex items-center gap-1.5 animate-pulse"
              >
                <Circle className="w-3.5 h-3.5 fill-current" />
                启动母带录音
              </button>
            ) : (
              <button
                onClick={onStopRecord}
                id="btn-tape-stop"
                className="px-4 py-2 bg-gradient-to-r from-neutral-800 to-neutral-900 hover:from-neutral-700 hover:to-neutral-800 text-white font-mono text-xs font-bold rounded shadow-md border border-neutral-700/80 hover:scale-102 active:scale-98 transition flex items-center gap-1.5"
              >
                <Square className="w-3.5 h-3.5 fill-current" />
                停止并导出
              </button>
            )}

            {recordedUrl && (
              <>
                <button
                  onClick={onPlayRecording}
                  id="btn-tape-play"
                  className="px-4 py-2 bg-[#00FF41] hover:bg-[#00FF41]/85 text-black font-mono text-xs font-bold rounded shadow-md border border-[#00FF41]/80 hover:scale-102 active:scale-98 transition flex items-center gap-1.5"
                >
                  <Play className="w-3.5 h-3.5 fill-current" />
                  回放录制音频
                </button>

                <a
                  href={recordedUrl}
                  download="utt_utility_record.webm"
                  id="btn-tape-download"
                  className="px-4 py-2 bg-gradient-to-r from-neutral-800 to-neutral-900 hover:bg-neutral-800 text-slate-300 font-mono text-xs font-bold rounded border border-neutral-750 flex items-center gap-1.5 hover:text-white transition"
                >
                  <Download className="w-3.5 h-3.5" />
                  下载音频 (.webm)
                </a>

                <button
                  onClick={onDeleteRecording}
                  id="btn-tape-trash"
                  className="p-2 text-neutral-500 hover:text-red-400 hover:bg-neutral-950 border border-transparent hover:border-neutral-800 rounded transition"
                  title="丢弃"
                >
                  <Trash2 className="w-4 h-4" />
                </button>
              </>
            )}
          </div>

          <div className="bg-neutral-950 p-2.5 rounded border border-neutral-850/60 font-mono text-[10px] text-neutral-500 leading-relaxed" id="cassette-notices">
            📌 说明: 即时串流录音将直接捕捉 UTT DSP 处理器计算后的主干立体声信号（如果启用了 Swap / Width / Phase 偏移，这些效果会完全嵌入录音中）。适合对调试后的通道表现进行即时录制并下载二次处理。
          </div>
        </div>
      </div>
    </div>
  );
};
