/* UTT v0.3 frontend.
   Plugin mode: every value/meter/scope sample comes from the native bridge.
   Preview mode (page opened in a plain browser with no __JUCE__ bridge) shows
   simulated data behind an explicit PREVIEW flag and never runs when the
   native bridge is present. */

(function () {
'use strict';

/* ------------------------------------------------------------------ */
/* parameter layout descriptor — presentation only; values, display     */
/* text and defaults are native-authoritative                          */
/* ------------------------------------------------------------------ */
var LAYOUT = [
  { section: 'input', rows: [
    [ { id: 'dcOffsetBypass', kind: 'toggle', label: 'DC 阻隔', warn: false } ],
    [ { id: 'phaseInvertL', kind: 'toggle', label: '相位反转 L' },
      { id: 'phaseInvertR', kind: 'toggle', label: '相位反转 R' } ],
    [ { id: 'phaseOffset', kind: 'knob', label: '相位偏移', size: 'hero' } ],
  ]},
  { section: 'process', rows: [
    [ { id: 'gain', kind: 'knob', label: '增益', size: 'hero' } ],
    [ { id: 'msMode', kind: 'segment', label: 'M/S', options: ['旁路', '编码', '解码'] } ],
    [ { id: 'monoMode', kind: 'segment', label: '单声道', options: ['关', 'L+R', '中', '侧'] },
      { id: 'monoToStereo', kind: 'segment', label: 'M→S', options: ['关', 'L→双', 'R→双'] } ],
    [ { id: 'stereoSeparation', kind: 'knob', label: '立体声分离', size: 'm' },
      { id: 'channelSwap', kind: 'toggle', label: '声道交换' } ],
  ]},
  { section: 'output', rows: [
    [ { id: 'outputLGain', kind: 'knob', label: 'L 增益', size: 'm' },
      { id: 'outputLMute', kind: 'mini', label: 'M' },
      { id: 'outputLSolo', kind: 'mini', label: 'S' } ],
    [ { id: 'outputRGain', kind: 'knob', label: 'R 增益', size: 'm' },
      { id: 'outputRMute', kind: 'mini', label: 'M' },
      { id: 'outputRSolo', kind: 'mini', label: 'S' } ],
    [ { id: 'pan', kind: 'knob', label: '声像', size: 'hero' } ],
  ]},
  { section: 'record', rows: [
    [ { id: 'recordEnable', kind: 'toggle', label: '录制' } ],
  ]},
];

/* ------------------------------------------------------------------ */
/* native bridge                                                       */
/* ------------------------------------------------------------------ */
var hasNativeBridge = (typeof window.__JUCE__ !== 'undefined')
  && window.__JUCE__.backend
  && window.__JUCE__.initialisationData
  && Array.isArray(window.__JUCE__.initialisationData.__juce__functions);

var pendingPromises = new Map();
var promiseSeq = 0;

function callNative(name, arg) {
  return new Promise(function (resolve) {
    var resultId = promiseSeq++;
    pendingPromises.set(resultId, resolve);
    window.__JUCE__.backend.emitEvent('__juce__invoke', {
      name: name, params: [arg], resultId: resultId,
    });
  });
}

if (hasNativeBridge) {
  window.__JUCE__.backend.addEventListener('__juce__complete', function (ev) {
    var resolve = pendingPromises.get(ev.promiseId);
    if (resolve) { pendingPromises.delete(ev.promiseId); resolve(ev.result); }
  });
}

/* state: id -> { v: normalized 0..1, text: display string, def: normalized } */
var state = {};
/* controls currently being dragged locally; suppress native echo for them */
var dragging = {};

/* ------------------------------------------------------------------ */
/* gesture helpers                                                     */
/* ------------------------------------------------------------------ */
function setValue(id, v) {
  v = Math.min(1, Math.max(0, v));
  if (!isFinite(v)) return;
  var st = state[id];
  if (!st) return;
  st.v = v;
  st.text = '\u2014';
  paintControl(id);
}

function gesture(id, phase, v) {
  if (!hasNativeBridge) return;
  callNative('uttGesture', { id: id, phase: phase, value: v === undefined ? undefined : v });
}

/* wheel bursts share one begin..end window */
var wheelTimers = {};

/* ------------------------------------------------------------------ */
/* control factory                                                     */
/* ------------------------------------------------------------------ */
var knobs = {};

function makeKnob(id, spec, mount) {
  var wrap = document.createElement('div');
  wrap.className = 'knob' + (spec.size === 'hero' ? ' knob-hero' : '');
  var label = document.createElement('div');
  label.className = 'knob-label';
  label.textContent = spec.label;
  var shell = document.createElement('div');
  shell.className = 'knob-shell';
  shell.tabIndex = 0;
  shell.setAttribute('role', 'slider');
  shell.setAttribute('aria-label', spec.label);
  shell.setAttribute('aria-valuemin', 0);
  shell.setAttribute('aria-valuemax', 100);
  var svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
  svg.setAttribute('viewBox', '0 0 100 100');
  svg.appendChild(arcPath(-135, 135, 'knob-track'));
  var progress = arcPath(-135, 135, 'knob-progress');
  progress.setAttribute('pathLength', '100');
  svg.appendChild(progress);
  var g = document.createElementNS('http://www.w3.org/2000/svg', 'g');
  for (var t = 0; t <= 10; t++) {
    var ang = -135 + t * 27;
    var major = (t % 5 === 0);
    g.appendChild(tick(ang, major));
  }
  svg.appendChild(g);
  var face = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
  face.setAttribute('cx', 50); face.setAttribute('cy', 50); face.setAttribute('r', 26);
  face.setAttribute('class', 'knob-face');
  svg.appendChild(face);
  var indicator = document.createElementNS('http://www.w3.org/2000/svg', 'line');
  indicator.setAttribute('class', 'knob-indicator');
  svg.appendChild(indicator);
  shell.appendChild(svg);
  var value = document.createElement('output');
  value.className = 'knob-value';
  wrap.appendChild(label); wrap.appendChild(shell); wrap.appendChild(value);
  mount.appendChild(wrap);

  knobs[id] = { shell: shell, progress: progress, indicator: indicator,
                value: value, aria: shell };

  /* --- pointer drag: vertical relative, 320px travel, shift = 1600px --- */
  var active = false, lastY = 0, current = 0;
  shell.addEventListener('pointerdown', function (e) {
    if (e.button !== 0) return;
    active = true; lastY = e.clientY; current = state[id] ? state[id].v : 0;
    dragging[id] = true;
    wrap.classList.add('dragging');
    shell.setPointerCapture(e.pointerId);
    gesture(id, 'begin');
    e.preventDefault();
  });
  shell.addEventListener('pointermove', function (e) {
    if (!active) return;
    var travel = e.shiftKey ? 1600 : 320;
    var delta = (lastY - e.clientY) / travel;
    lastY = e.clientY;
    current = Math.min(1, Math.max(0, current + delta));
    setValue(id, current);
    gesture(id, 'set', current);
  });
  function endDrag() {
    if (!active) return;
    active = false;
    wrap.classList.remove('dragging');
    delete dragging[id];
    gesture(id, 'end');
  }
  shell.addEventListener('pointerup', endDrag);
  shell.addEventListener('pointercancel', endDrag);
  shell.addEventListener('lostpointercapture', endDrag);
  window.addEventListener('blur', endDrag);

  /* --- wheel: 0.01 / shift 0.001, one gesture per 140ms burst --- */
  shell.addEventListener('wheel', function (e) {
    e.preventDefault();
    var st = state[id]; if (!st) return;
    var step = e.shiftKey ? 0.001 : 0.01;
    var dir = e.deltaY < 0 ? 1 : -1;
    if (!wheelTimers[id]) gesture(id, 'begin');
    else clearTimeout(wheelTimers[id]);
    st.v = Math.min(1, Math.max(0, st.v + dir * step));
    setValue(id, st.v);
    gesture(id, 'set', st.v);
    wheelTimers[id] = setTimeout(function () {
      wheelTimers[id] = null;
      gesture(id, 'end');
    }, 140);
  }, { passive: false });

  /* --- keyboard --- */
  shell.addEventListener('keydown', function (e) {
    var st = state[id]; if (!st) return;
    var fine = e.shiftKey ? 0.001 : 0.01;
    var handled = true, next = null;
    switch (e.key) {
      case 'ArrowUp': case 'ArrowRight': next = st.v + fine; break;
      case 'ArrowDown': case 'ArrowLeft': next = st.v - fine; break;
      case 'PageUp': next = st.v + 0.1; break;
      case 'PageDown': next = st.v - 0.1; break;
      case 'Home': next = 0; break;
      case 'End': next = 1; break;
      default: handled = false;
    }
    if (!handled) return;
    e.preventDefault();
    if (!wheelTimers[id]) gesture(id, 'begin');
    else clearTimeout(wheelTimers[id]);
    st.v = Math.min(1, Math.max(0, next));
    setValue(id, st.v);
    gesture(id, 'set', st.v);
    wheelTimers[id] = setTimeout(function () {
      wheelTimers[id] = null;
      gesture(id, 'end');
    }, 140);
  });

  /* --- double-click reset --- */
  shell.addEventListener('dblclick', function () {
    if (!hasNativeBridge) return;
    callNative('uttResetParam', { id: id });
  });

  attachContextMenu(shell, id);
}

function makeToggle(id, spec, mount) {
  var el = document.createElement('div');
  el.className = 'toggle' + (spec.warn ? ' toggle-warn' : '');
  el.setAttribute('data-id', id);
  var track = document.createElement('div');
  track.className = 'toggle-track';
  track.tabIndex = 0;
  track.setAttribute('role', 'switch');
  track.setAttribute('aria-label', spec.label);
  var label = document.createElement('span');
  label.className = 'toggle-label';
  label.textContent = spec.label;
  el.appendChild(track); el.appendChild(label);
  mount.appendChild(el);

  function toggle() {
    if (!state[id]) return;
    var v = state[id].v >= 0.5 ? 0 : 1;
    gesture(id, 'begin');
    setValue(id, v);
    gesture(id, 'set', v);
    gesture(id, 'end');
  }
  track.addEventListener('click', toggle);
  track.addEventListener('keydown', function (e) {
    if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); toggle(); }
  });
  attachContextMenu(el, id);
}

function makeSegment(id, spec, mount) {
  var el = document.createElement('div');
  el.className = 'segment';
  var top = document.createElement('div');
  top.className = 'segment-top';
  var label = document.createElement('span');
  label.className = 'segment-label';
  label.textContent = spec.label;
  var bank = document.createElement('div');
  bank.className = 'segment-bank';
  bank.setAttribute('role', 'radiogroup');
  bank.setAttribute('aria-label', spec.label);
  top.appendChild(label);
  el.appendChild(top); el.appendChild(bank);
  mount.appendChild(el);

  var buttons = [];
  spec.options.forEach(function (opt, i) {
    var b = document.createElement('button');
    b.type = 'button';
    b.className = 'segment-btn';
    b.textContent = opt;
    b.setAttribute('role', 'radio');
    b.addEventListener('click', function () {
      if (!state[id]) return;
      var n = spec.options.length;
      var v = n === 1 ? 0 : i / (n - 1);
      gesture(id, 'begin');
      setValue(id, v);
      gesture(id, 'set', v);
      gesture(id, 'end');
    });
    attachContextMenu(b, id);
    bank.appendChild(b);
    buttons.push(b);
  });
  el._buttons = buttons;
  el._count = spec.options.length;
}

function makeMini(id, spec, mount) {
  var b = document.createElement('button');
  b.type = 'button';
  b.className = 'mini';
  b.textContent = spec.label;
  b.setAttribute('aria-label', spec.label);
  b.setAttribute('data-id', id);
  b.addEventListener('click', function () {
    if (!state[id]) return;
    var v = state[id].v >= 0.5 ? 0 : 1;
    gesture(id, 'begin');
    setValue(id, v);
    gesture(id, 'set', v);
    gesture(id, 'end');
  });
  mount.appendChild(b);
  attachContextMenu(b, id);
}

/* right-click on a parameter control → host native parameter menu */
function attachContextMenu(el, id) {
  el.addEventListener('contextmenu', function (e) {
    e.preventDefault();
    e.stopPropagation();
    if (hasNativeBridge) callNative('uttParamMenu', { id: id });
  });
}

function arcPath(startDeg, endDeg, cls) {
  function pt(deg) {
    var r = 44, cx = 50, cy = 50;
    var a = (deg - 90) * Math.PI / 180;
    return [cx + r * Math.cos(a), cy + r * Math.sin(a)];
  }
  var s = pt(startDeg), e2 = pt(endDeg);
  var large = (endDeg - startDeg) > 180 ? 1 : 0;
  var p = document.createElementNS('http://www.w3.org/2000/svg', 'path');
  p.setAttribute('d', 'M ' + s[0] + ' ' + s[1] +
    ' A 44 44 0 ' + large + ' 1 ' + e2[0] + ' ' + e2[1]);
  p.setAttribute('class', cls);
  p.setAttribute('fill', 'none');
  return p;
}

function tick(deg, major) {
  function pt(deg, r) {
    var a = (deg - 90) * Math.PI / 180;
    return [50 + r * Math.cos(a), 50 + r * Math.sin(a)];
  }
  var outer = pt(deg, major ? 36 : 34.5), inner = pt(deg, 31);
  var l = document.createElementNS('http://www.w3.org/2000/svg', 'line');
  l.setAttribute('x1', outer[0]); l.setAttribute('y1', outer[1]);
  l.setAttribute('x2', inner[0]); l.setAttribute('y2', inner[1]);
  l.setAttribute('class', major ? 'knob-tick knob-tick-major' : 'knob-tick');
  return l;
}

/* paint a control from state (native text when present) */
function paintControl(id) {
  var st = state[id]; if (!st) return;
  var host = document.querySelector('[data-id="' + id + '"]');
  /* knobs */
  if (knobs[id]) {
    var k = knobs[id];
    var deg = -135 + st.v * 270;
    k.progress.setAttribute('stroke-dasharray', (st.v * 75) + ' 100');
    var a = (deg - 90) * Math.PI / 180;
    var x1 = 50 + 12 * Math.cos(a), y1 = 50 + 12 * Math.sin(a);
    var x2 = 50 + 24 * Math.cos(a), y2 = 50 + 24 * Math.sin(a);
    k.indicator.setAttribute('x1', x1); k.indicator.setAttribute('y1', y1);
    k.indicator.setAttribute('x2', x2); k.indicator.setAttribute('y2', y2);
    k.value.textContent = st.text;
    k.aria.setAttribute('aria-valuenow', Math.round(st.v * 100));
    return;
  }
  /* segments */
  var seg = segmentsById[id];
  if (seg && seg._buttons) {
    var idx = Math.round(st.v * (seg._count - 1));
    seg._buttons.forEach(function (b, i) {
      b.classList.toggle('on', i === idx);
      b.setAttribute('aria-checked', i === idx ? 'true' : 'false');
    });
    return;
  }
  /* toggles + minis */
  if (host) {
    var on = st.v >= 0.5;
    host.classList.toggle('on', on);
    if (host.classList.contains('mini')) {
      host.classList.toggle('on-pine', on && id.indexOf('Solo') >= 0);
      host.classList.toggle('on', on && id.indexOf('Solo') < 0);
    }
    if (host.classList.contains('toggle')) {
      var track = host.querySelector('.toggle-track');
      if (track) track.setAttribute('aria-checked', on ? 'true' : 'false');
    }
  }
}

/* ------------------------------------------------------------------ */
/* build the DOM                                                       */
/* ------------------------------------------------------------------ */
var segmentsById = {};

LAYOUT.forEach(function (mod) {
  var mount = document.getElementById('sec-' + mod.section);
  if (!mount) return;
  mod.rows.forEach(function (row) {
    var rowEl = document.createElement('div');
    rowEl.className = 'ctl-row';
    row.forEach(function (spec) {
      if (spec.kind === 'knob') makeKnob(spec.id, spec, rowEl);
      else if (spec.kind === 'toggle') makeToggle(spec.id, spec, rowEl);
      else if (spec.kind === 'segment') {
        makeSegment(spec.id, spec, rowEl);
        segmentsById[spec.id] = rowEl.querySelector('.segment');
      }
      else if (spec.kind === 'mini') makeMini(spec.id, spec, rowEl);
    });
    mount.appendChild(rowEl);
    if (mod.rows.length > 1 && row !== mod.rows[mod.rows.length - 1]) {
      var d = document.createElement('div');
      d.className = 'ctl-divider';
      mount.appendChild(d);
    }
  });
});

/* ------------------------------------------------------------------ */
/* meters / scope rendering                                            */
/* ------------------------------------------------------------------ */
var scopeCanvas = document.getElementById('scope');
var scopeCtx = scopeCanvas.getContext('2d');
var scopeFrame = null;

var meterEls = {
  l: document.querySelector('#meter-l .meterbar-rms'),
  lp: document.querySelector('#meter-l .meterbar-peak'),
  lh: document.querySelector('#meter-l .meterbar-hold'),
  r: document.querySelector('#meter-r .meterbar-rms'),
  rp: document.querySelector('#meter-r .meterbar-peak'),
  rh: document.querySelector('#meter-r .meterbar-hold'),
  lwrap: document.getElementById('meter-l'),
  rwrap: document.getElementById('meter-r'),
};
var holds = { l: -60, r: -60 };
var holdTimes = { l: 0, r: 0 };

function dbToPct(db) {
  var lo = -60, hi = 0;
  var p = (db - lo) / (hi - lo);
  return Math.min(100, Math.max(0, p * 100));
}

function setBar(el, db) { el.style.height = dbToPct(db) + '%'; }

var corrFill = document.getElementById('corr-fill');
var corrMarker = document.getElementById('corr-marker');
var corrValue = document.getElementById('corr-value');
var lufsEls = {
  m: document.getElementById('lufs-m'),
  s: document.getElementById('lufs-s'),
  i: document.getElementById('lufs-i'),
};

function drawScope() {
  var w = scopeCanvas.width, h = scopeCanvas.height;
  scopeCtx.fillStyle = '#1C1B14';
  scopeCtx.fillRect(0, 0, w, h);
  /* grid */
  scopeCtx.strokeStyle = '#2E2C22';
  scopeCtx.lineWidth = 1;
  scopeCtx.beginPath();
  for (var gx = 1; gx < 4; gx++) {
    scopeCtx.moveTo(gx * w / 4, 0); scopeCtx.lineTo(gx * w / 4, h);
  }
  scopeCtx.moveTo(0, h / 2); scopeCtx.lineTo(w, h / 2);
  scopeCtx.stroke();
  if (!scopeFrame) return;
  var n = Math.min(scopeFrame.l.length, scopeFrame.r.length);
  if (n < 2) return;
  var mid = h / 2, amp = h / 2 - 3;
  scopeCtx.lineWidth = 1.5;
  scopeCtx.strokeStyle = '#7A9CFF';
  scopeCtx.beginPath();
  for (var i = 0; i < n; i++) {
    var x = i / (n - 1) * w;
    var y = mid - Math.max(-1, Math.min(1, scopeFrame.l[i])) * amp;
    if (i === 0) scopeCtx.moveTo(x, y); else scopeCtx.lineTo(x, y);
  }
  scopeCtx.stroke();
  scopeCtx.strokeStyle = 'rgba(228,225,212,0.55)';
  scopeCtx.beginPath();
  for (var j = 0; j < n; j++) {
    var x2 = j / (n - 1) * w;
    var y2 = mid - Math.max(-1, Math.min(1, scopeFrame.r[j])) * amp;
    if (j === 0) scopeCtx.moveTo(x2, y2); else scopeCtx.lineTo(x2, y2);
  }
  scopeCtx.stroke();
}

function fmtLufs(v) {
  if (v <= -70 || !isFinite(v)) return '-70.0';
  return v.toFixed(1);
}

/* one rAF loop; stops when no telemetry arrived recently */
var rafId = null;
var lastTelemetry = 0;

function renderMeters(m) {
  var now = performance.now();
  setBar(meterEls.l, m.lr); setBar(meterEls.lp, m.lp);
  setBar(meterEls.r, m.rr); setBar(meterEls.rp, m.rp);
  meterEls.lwrap.classList.toggle('clip', m.lp >= -0.1);
  meterEls.rwrap.classList.toggle('clip', m.rp >= -0.1);
  ['l', 'r'].forEach(function (ch) {
    var pk = ch === 'l' ? m.lp : m.rp;
    if (pk >= holds[ch]) { holds[ch] = pk; holdTimes[ch] = now; }
    else if (now - holdTimes[ch] > 1200) {
      holds[ch] = Math.max(-60, holds[ch] - 0.5);
    }
    setBar(ch === 'l' ? meterEls.lh : meterEls.rh, holds[ch]);
  });
  /* correlation: -1..1 → 0..100% */
  var c = Math.max(-1, Math.min(1, m.corr));
  corrValue.textContent = c.toFixed(2);
  var pct = (c + 1) / 2 * 100;
  if (c >= 0) {
    corrFill.style.left = '50%'; corrFill.style.width = (pct - 50) + '%';
  } else {
    corrFill.style.left = pct + '%'; corrFill.style.width = (50 - pct) + '%';
  }
  corrMarker.style.left = 'calc(' + pct + '% - 1px)';
  lufsEls.m.textContent = fmtLufs(m.lm);
  lufsEls.s.textContent = fmtLufs(m.ls);
  lufsEls.i.textContent = fmtLufs(m.li);
}

function maybeStopRaf() {
  if (rafId !== null && performance.now() - lastTelemetry > 800) {
    cancelAnimationFrame(rafId);
    rafId = null;
  }
}
setInterval(maybeStopRaf, 500);

function ensureRaf() {
  if (rafId === null) {
    var loop = function () {
      drawScope();
      rafId = requestAnimationFrame(loop);
    };
    rafId = requestAnimationFrame(loop);
  }
}

/* ------------------------------------------------------------------ */
/* native events                                                       */
/* ------------------------------------------------------------------ */
if (hasNativeBridge) {
  window.__JUCE__.backend.addEventListener('uttParamState', function (ev) {
    var params = ev.params || {};
    Object.keys(params).forEach(function (id) {
      if (dragging[id]) return;
      if (!state[id]) state[id] = { v: 0, text: '' };
      state[id].v = params[id].v;
      state[id].text = params[id].t;
      paintControl(id);
    });
  });
  window.__JUCE__.backend.addEventListener('uttMeter', function (ev) {
    lastTelemetry = performance.now();
    renderMeters(ev);
    ensureRaf();
  });
  window.__JUCE__.backend.addEventListener('uttScope', function (ev) {
    lastTelemetry = performance.now();
    scopeFrame = { l: ev.l || [], r: ev.r || [] };
    ensureRaf();
  });
  window.__JUCE__.backend.addEventListener('uttRecord', function (ev) {
    applyRecordState(ev);
  });
}

/* ------------------------------------------------------------------ */
/* record bay                                                          */
/* ------------------------------------------------------------------ */
var recordPath = document.getElementById('record-path');
var recordStateEl = document.getElementById('record-state');
var recordDropped = document.getElementById('record-dropped');

function applyRecordState(ev) {
  recordPath.textContent = ev.path || '—';
  recordPath.title = ev.path || '';
  if (ev.error) {
    recordStateEl.textContent = '写入失败';
    recordStateEl.className = 'record-state err';
  } else if (ev.active) {
    recordStateEl.textContent = '录制中';
    recordStateEl.className = 'record-state on';
  } else if (ev.on) {
    recordStateEl.textContent = '待录';
    recordStateEl.className = 'record-state';
  } else {
    recordStateEl.textContent = '待机';
    recordStateEl.className = 'record-state';
  }
  recordDropped.textContent = ev.dropped > 0 ? ('丢弃 ' + ev.dropped + ' 块') : '';
}

document.getElementById('btn-browse').addEventListener('click', function () {
  if (hasNativeBridge) callNative('uttRecordBrowse', {});
});

document.getElementById('btn-about').addEventListener('click', function () {
  if (hasNativeBridge) callNative('uttAbout', {});
});

/* ------------------------------------------------------------------ */
/* host hardening (HTML layer; native layer does the rest)             */
/* ------------------------------------------------------------------ */
document.addEventListener('contextmenu', function (e) { e.preventDefault(); });
document.addEventListener('dragstart', function (e) { e.preventDefault(); });
document.addEventListener('dragover', function (e) { e.preventDefault(); });
document.addEventListener('drop', function (e) { e.preventDefault(); });

/* ------------------------------------------------------------------ */
/* preview mode — only when the native bridge is absent                */
/* ------------------------------------------------------------------ */
function startPreview() {
  var msg = document.getElementById('status-msg');
  msg.textContent = '浏览器预览模式 · 模拟数据（插件内为真实遥测）';
  msg.classList.add('preview');
  var t = 0;
  LAYOUT.forEach(function (mod) {
    mod.rows.forEach(function (row) {
      row.forEach(function (spec) {
        state[spec.id] = { v: 0.5, text: '\u2014' };
      });
    });
  });
  Object.keys(state).forEach(paintControl);
  setInterval(function () {
    if (document.hidden) return;
    t += 0.08;
    lastTelemetry = performance.now();
    renderMeters({
      lp: -6 + 5 * Math.sin(t * 1.7), rp: -7 + 5 * Math.sin(t * 1.3 + 1),
      lr: -14 + 4 * Math.sin(t), rr: -15 + 4 * Math.cos(t * 0.8),
      corr: 0.5 * Math.sin(t * 0.4),
      lm: -18 + 4 * Math.sin(t), ls: -19 + 3 * Math.sin(t * 0.3), li: -20,
    });
    var l = new Array(128), r = new Array(128);
    for (var i = 0; i < 128; i++) {
      l[i] = 0.6 * Math.sin(i / 128 * 6 * Math.PI + t * 2);
      r[i] = 0.5 * Math.sin(i / 128 * 6 * Math.PI + t * 2 + 0.5);
    }
    scopeFrame = { l: l, r: r };
    ensureRaf();
  }, 80);
  applyRecordState({ path: '~/Music/UTT Recordings/UTT_20260822_101500.wav', on: false, active: false });
  document.getElementById('status-engine').textContent = '48000 Hz · 128';
}

/* ------------------------------------------------------------------ */
/* boot                                                                */
/* ------------------------------------------------------------------ */
function boot() {
  drawScope();
  if (!hasNativeBridge) { startPreview(); return; }
  callNative('uttUiReady', {}).then(function (snap) {
    (snap.params || []).forEach(function (p) {
      state[p.id] = { v: p.v, text: p.t, def: p.d };
      paintControl(p.id);
    });
    if (snap.record) applyRecordState(snap.record);
    if (snap.engine) document.getElementById('status-engine').textContent = snap.engine;
    document.getElementById('status-msg').textContent = '就绪';
  });
}

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', boot);
} else {
  boot();
}

})();
