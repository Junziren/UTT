(() => {
  "use strict";

  const nativeBackend = window.__JUCE__ && window.__JUCE__.backend;
  const hasNativeBridge = Boolean(nativeBackend && window.__JUCE__.initialisationData);
  const state = Object.create(null);
  const controls = Object.create(null);
  const activeEnders = new Set();
  const wheelTimers = Object.create(null);
  let scopeFrame = null;
  let scopeAnimationFrame = null;
  let surfaceActive = true;
  let lastAboutInvoker = null;
  let heroFeedbackTimer = null;
  const feedbackNodes = new Set();

  const inputRows = [
    { className: "split", controls: [
      { id: "dcOffsetBypass", kind: "toggle", label: "DC BLOCK" },
      { id: "phaseInvertL", kind: "toggle", label: "PHASE L" },
      { id: "phaseInvertR", kind: "toggle", label: "PHASE R" },
    ] },
    { className: "center divided", controls: [
      { id: "phaseOffset", kind: "knob", label: "PHASE", hero: true },
    ] },
  ];

  const processRows = [
    { className: "split", controls: [
      { id: "gain", kind: "knob", label: "GAIN", hero: true },
      { id: "msMode", kind: "choice", label: "M/S" },
    ] },
    { className: "divided", controls: [
      { id: "monoMode", kind: "choice", label: "MONO" },
    ] },
    { className: "split", controls: [
      { id: "stereoSeparation", kind: "knob", label: "WIDTH" },
      { id: "monoToStereo", kind: "choice", label: "M TO S" },
      { id: "channelSwap", kind: "toggle", label: "SWAP" },
    ] },
  ];

  const knownSpecs = [
    ...inputRows.flatMap((row) => row.controls),
    ...processRows.flatMap((row) => row.controls),
    { id: "outputLGain", kind: "knob", label: "L GAIN" },
    { id: "outputRGain", kind: "knob", label: "R GAIN" },
    { id: "outputLMute", kind: "mini", label: "M" },
    { id: "outputRMute", kind: "mini", label: "M" },
    { id: "outputLSolo", kind: "mini", label: "S" },
    { id: "outputRSolo", kind: "mini", label: "S" },
    { id: "pan", kind: "knob", label: "PAN", hero: true },
    { id: "recordEnable", kind: "toggle", label: "REC" },
  ];

  // Explanatory copy only. Native state remains the sole source for every
  // parameter value, range, default and choice list.
  const parameterHelp = Object.freeze({
    dcOffsetBypass: {
      title: "DC BLOCK / 直流阻隔",
      route: "控制逻辑：ON = BYPASS / OFF = FILTER ACTIVE",
      copy: "要清除设备或录音带来的直流偏移，请将开关置于 OFF。滤波器约从 5 Hz 开始工作，正常可听频段保持不变。",
    },
    phaseInvertL: {
      title: "PHASE L / 左极性",
      route: "L -> -L",
      copy: "只将左声道波形上下翻转 180 度。用于修正一侧接线或录音极性相反，也可检查单声道叠加时的抵消。",
    },
    phaseInvertR: {
      title: "PHASE R / 右极性",
      route: "R -> -R",
      copy: "只将右声道波形上下翻转 180 度。用于修正一侧接线或录音极性相反，也可检查单声道叠加时的抵消。",
    },
    phaseOffset: {
      title: "PHASE / 全通相位",
      route: "L/R -> ALL-PASS / 0...360 deg @ 1 kHz",
      copy: "对左右声道同时施加全通相位旋转；0 度时完全旁路。校准点为 1 kHz，其他频率的相位偏移会随频率变化。",
    },
    gain: {
      title: "GAIN / 输入增益",
      route: "INPUT -> GAIN -> M/S",
      copy: "这是所有立体声处理之前的总电平。0 dB 保持直通；先用它建立余量，再进入 M/S、宽度和输出修整。",
    },
    msMode: {
      title: "M/S / MID-SIDE CODEC",
      route: "ENC: L/R -> M=(L+R)/2 | S=(L-R)/2",
      copy: "Encode 会把 M 写到左输出、S 写到右输出，供中侧处理链使用；Decode 则把 M/S 还原为 L=M+S、R=M-S。Bypass 保持普通 L/R。",
    },
    monoMode: {
      title: "MONO / 单声道检查",
      route: "M=(L+R)/2 | S=(L-R)/2",
      copy: "L+R SUM 与 MID ONLY 在当前引擎中都会把 M 复制到左右输出；SIDE ONLY 会把 S 复制到左右，用于检查兼容性和差分信息。",
    },
    stereoSeparation: {
      title: "WIDTH / 立体声宽度",
      route: "M UNCHANGED / S x WIDTH",
      copy: "只缩放 Side，Mid 保持不变。100% 保持原宽度，0% 让左右相同。MONO 后 Side 已为 0，因此此项不会再改变声音。",
    },
    monoToStereo: {
      title: "M TO S / 单声道复制",
      route: "L -> L/R  OR  R -> L/R",
      copy: "在宽度处理后，将当前左或右物理输出复制到两侧。适合快速建立双通道单声道，并保留所选声道的处理结果。",
    },
    channelSwap: {
      title: "SWAP / 声道互换",
      route: "L <-> R",
      copy: "在声像和最终输出之前互换左右通道。用于检查声像、修正接线或验证宿主的声道映射。",
    },
    pan: {
      title: "PAN / 输出平衡",
      route: "RIGHT: ATTENUATE L / LEFT: ATTENUATE R",
      copy: "这是 balance 式声像：向右只衰减左侧，向左只衰减右侧。中心位置保持双通道透明，不会把两个声道交叉混合。",
    },
    outputLGain: {
      title: "L GAIN / 左输出",
      route: "PAN -> L TRIM -> MUTE / SOLO",
      copy: "独立修整左侧最终输出电平。它位于 PAN 之后、静音与独听逻辑之前，适合校准左右不平衡或输出余量。",
    },
    outputRGain: {
      title: "R GAIN / 右输出",
      route: "PAN -> R TRIM -> MUTE / SOLO",
      copy: "独立修整右侧最终输出电平。它位于 PAN 之后、静音与独听逻辑之前，适合校准左右不平衡或输出余量。",
    },
    outputLMute: {
      title: "L MUTE / 左静音",
      route: "L OUT -> SILENCE",
      copy: "将左最终输出静音，不改变参数或录音设置。若只开启右侧 SOLO，独听优先级会覆盖此静音状态。",
    },
    outputRMute: {
      title: "R MUTE / 右静音",
      route: "R OUT -> SILENCE",
      copy: "将右最终输出静音，不改变参数或录音设置。若只开启左侧 SOLO，独听优先级会覆盖此静音状态。",
    },
    outputLSolo: {
      title: "L SOLO / 左独听",
      route: "L OUT LIVE / R OUT MUTED",
      copy: "只监听左最终输出，右侧会被静音。独听优先于 MUTE；左右 SOLO 同时开启时，插件恢复正常立体声输出。",
    },
    outputRSolo: {
      title: "R SOLO / 右独听",
      route: "R OUT LIVE / L OUT MUTED",
      copy: "只监听右最终输出，左侧会被静音。独听优先于 MUTE；左右 SOLO 同时开启时，插件恢复正常立体声输出。",
    },
    recordEnable: {
      title: "RECORD / 后台录音",
      route: "POST-CHAIN STEREO -> 24-BIT WAV",
      copy: "录制处理完成后的立体声输出。先用 BROWSE 选择文件；REC 只负责启停，写入在后台线程进行，DROP 显示未写入的数据块数。",
    },
  });

  let parameterHelpBubble = null;
  let parameterHelpTarget = null;

  function clamp(value, min = 0, max = 1) {
    return Math.min(max, Math.max(min, value));
  }

  function element(tag, className, text) {
    const node = document.createElement(tag);
    if (className) node.className = className;
    if (text !== undefined) node.textContent = text;
    return node;
  }

  function createParameterHelp() {
    parameterHelpBubble = element("div", "parameter-help");
    parameterHelpBubble.id = "parameterHelp";
    parameterHelpBubble.hidden = true;
    parameterHelpBubble.setAttribute("role", "tooltip");
    document.body.append(parameterHelpBubble);
  }

  function positionParameterHelp() {
    if (!parameterHelpBubble || parameterHelpBubble.hidden || !parameterHelpTarget) return;
    const bubble = parameterHelpBubble.getBoundingClientRect();
    const edge = 12;
    const viewportWidth = window.innerWidth;
    const viewportHeight = window.innerHeight;
    const hero = document.querySelector(".hero")?.getBoundingClientRect();
    const heroField = document.getElementById("heroField")?.getBoundingClientRect();
    const center = clamp(heroField ? heroField.left + heroField.width / 2
      : hero ? hero.left + hero.width * .55 : viewportWidth / 2,
      edge + bubble.width / 2, viewportWidth - edge - bubble.width / 2);
    const top = hero
      ? clamp(hero.top + (hero.height - bubble.height) / 2,
        edge, viewportHeight - edge - bubble.height)
      : edge;

    parameterHelpBubble.style.left = `${Math.round(center)}px`;
    parameterHelpBubble.style.top = `${Math.round(top)}px`;
  }

  function hideParameterHelp(target) {
    if (!parameterHelpBubble || (target && parameterHelpTarget !== target)) return;
    parameterHelpBubble.hidden = true;
    parameterHelpTarget = null;
  }

  function showParameterHelp(target, id) {
    const help = parameterHelp[id];
    if (!help || !parameterHelpBubble) return;

    parameterHelpTarget = target;
    parameterHelpBubble.replaceChildren(
      element("strong", "parameter-help-title", help.title),
      element("span", "parameter-help-route mono", help.route),
      element("span", "parameter-help-copy", help.copy));
    parameterHelpBubble.hidden = false;
    positionParameterHelp();
  }

  function attachParameterHelp(node, id) {
    const help = parameterHelp[id];
    if (!help) return;

    node.dataset.help = id;
    node.setAttribute("aria-description", `${help.route}. ${help.copy}`);
    node.addEventListener("pointerenter", () => showParameterHelp(node, id));
    node.addEventListener("pointerleave", () => hideParameterHelp(node));
    node.addEventListener("focusin", () => showParameterHelp(node, id));
    node.addEventListener("focusout", (event) => {
      if (!node.contains(event.relatedTarget)) hideParameterHelp(node);
    });
  }

  function reducedMotion() {
    return window.matchMedia && window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  }

  function clearVisualFeedback() {
    if (heroFeedbackTimer !== null) {
      window.clearTimeout(heroFeedbackTimer);
      heroFeedbackTimer = null;
    }
    document.querySelector(".hero")?.classList.remove("gesture-active");
    feedbackNodes.forEach((node) => node.classList.remove("feedback"));
    feedbackNodes.clear();
  }

  function triggerParameterFeedback(node) {
    if (!surfaceActive || reducedMotion() || !node) return;
    const hero = document.querySelector(".hero");
    const field = document.getElementById("heroField");

    if (hero && field) {
      const sourceBox = node.getBoundingClientRect();
      const fieldBox = field.getBoundingClientRect();
      const x = clamp(sourceBox.left + sourceBox.width / 2 - fieldBox.left,
        10, Math.max(10, fieldBox.width - 10));
      field.style.setProperty("--gesture-x", `${Math.round(x)}px`);
      hero.classList.remove("gesture-active");
      void hero.offsetWidth;
      hero.classList.add("gesture-active");
      if (heroFeedbackTimer !== null) window.clearTimeout(heroFeedbackTimer);
      heroFeedbackTimer = window.setTimeout(() => {
        hero.classList.remove("gesture-active");
        heroFeedbackTimer = null;
      }, 440);
    }

    node.classList.remove("feedback");
    void node.offsetWidth;
    node.classList.add("feedback");
    feedbackNodes.add(node);
    window.setTimeout(() => {
      node.classList.remove("feedback");
      feedbackNodes.delete(node);
    }, 360);
  }

  function emit(name, payload) {
    if (hasNativeBridge) nativeBackend.emitEvent(name, payload || {});
  }

  function ensureState(id, kind) {
    if (!state[id]) {
      state[id] = {
        id,
        kind: kind || "continuous",
        normalized: 0,
        defaultNormalized: 0,
        text: "--",
        choices: [],
      };
    }
    return state[id];
  }

  function attachParameterContext(node, id) {
    node.addEventListener("contextmenu", (event) => {
      event.preventDefault();
      event.stopPropagation();
      emit("parameterContext", { id });
    });
  }

  function createKnob(spec, mount) {
    ensureState(spec.id, "continuous");
    const root = element("div", `knob${spec.hero ? " hero-knob" : ""}`);
    root.dataset.param = spec.id;
    const dial = element("div", "knob-dial");
    dial.tabIndex = 0;
    dial.setAttribute("role", "slider");
    dial.setAttribute("aria-label", spec.label);
    dial.setAttribute("aria-valuemin", "0");
    dial.setAttribute("aria-valuemax", "100");
    dial.append(element("i", "knob-needle"));
    dial.append(element("i", "knob-hub"));
    const label = element("span", "knob-label", spec.label);
    const value = element("output", "knob-value", "--");
    root.append(label, dial, value);
    mount.append(root);
    controls[spec.id] = { type: "knob", root, dial, value };

    let active = false;
    let lastY = 0;
    let current = 0;

    function finish() {
      if (!active) return;
      active = false;
      root.classList.remove("dragging");
      activeEnders.delete(finish);
      emit("parameterGesture", { id: spec.id, phase: "end" });
    }

    dial.addEventListener("pointerdown", (event) => {
      if (event.button !== 0) return;
      active = true;
      lastY = event.clientY;
      current = ensureState(spec.id).normalized || 0;
      root.classList.add("dragging");
      activeEnders.add(finish);
      dial.setPointerCapture(event.pointerId);
      emit("parameterGesture", { id: spec.id, phase: "begin" });
      triggerParameterFeedback(root);
      event.preventDefault();
    });

    dial.addEventListener("pointermove", (event) => {
      if (!active) return;
      const pixelsForTravel = event.shiftKey ? 1600 : 320;
      current = clamp(current + (lastY - event.clientY) / pixelsForTravel);
      lastY = event.clientY;
      setLocalNormalized(spec.id, current);
      emit("parameterGesture", { id: spec.id, phase: "value", normalized: current });
    });

    dial.addEventListener("pointerup", finish);
    dial.addEventListener("pointercancel", finish);
    dial.addEventListener("lostpointercapture", finish);
    window.addEventListener("blur", finish);

    dial.addEventListener("wheel", (event) => {
      event.preventDefault();
      const next = clamp((ensureState(spec.id).normalized || 0)
        + (event.deltaY < 0 ? 1 : -1) * (event.shiftKey ? .001 : .01));
      if (!wheelTimers[spec.id]) {
        emit("parameterGesture", { id: spec.id, phase: "begin" });
        triggerParameterFeedback(root);
      }
      else window.clearTimeout(wheelTimers[spec.id]);
      setLocalNormalized(spec.id, next);
      emit("parameterGesture", { id: spec.id, phase: "value", normalized: next });
      wheelTimers[spec.id] = window.setTimeout(() => {
        wheelTimers[spec.id] = null;
        emit("parameterGesture", { id: spec.id, phase: "end" });
      }, 140);
    }, { passive: false });

    let keyboardGesture = false;
    const finishKeyboardGesture = () => {
      if (!keyboardGesture) return;
      keyboardGesture = false;
      emit("parameterGesture", { id: spec.id, phase: "end" });
    };
    dial.addEventListener("keydown", (event) => {
      const fine = event.shiftKey ? .001 : .01;
      const currentValue = ensureState(spec.id).normalized || 0;
      let next = null;
      if (event.key === "ArrowUp" || event.key === "ArrowRight") next = currentValue + fine;
      if (event.key === "ArrowDown" || event.key === "ArrowLeft") next = currentValue - fine;
      if (event.key === "PageUp") next = currentValue + .1;
      if (event.key === "PageDown") next = currentValue - .1;
      if (event.key === "Home") next = 0;
      if (event.key === "End") next = 1;
      if (next === null) return;
      event.preventDefault();
      if (!keyboardGesture) {
        keyboardGesture = true;
        emit("parameterGesture", { id: spec.id, phase: "begin" });
        triggerParameterFeedback(root);
      }
      next = clamp(next);
      setLocalNormalized(spec.id, next);
      emit("parameterGesture", { id: spec.id, phase: "value", normalized: next });
    });
    dial.addEventListener("keyup", finishKeyboardGesture);
    dial.addEventListener("blur", finishKeyboardGesture);
    dial.addEventListener("dblclick", () => {
      setLocalNormalized(spec.id, ensureState(spec.id).defaultNormalized || 0);
      emit("parameterReset", { id: spec.id });
      triggerParameterFeedback(root);
    });
    attachParameterHelp(root, spec.id);
    attachParameterContext(root, spec.id);
  }

  function completeGesture(id, value, source) {
    emit("parameterGesture", { id, phase: "begin" });
    triggerParameterFeedback(source);
    setLocalNormalized(id, value);
    emit("parameterGesture", { id, phase: "value", normalized: value });
    emit("parameterGesture", { id, phase: "end" });
  }

  function createToggle(spec, mount) {
    ensureState(spec.id, "toggle");
    const root = element("button", "toggle");
    root.type = "button";
    root.dataset.param = spec.id;
    root.setAttribute("role", "switch");
    root.setAttribute("aria-label", spec.label);
    root.append(element("span", "toggle-track"), element("span", "toggle-label", spec.label));
    root.addEventListener("click", () => completeGesture(spec.id,
      ensureState(spec.id).normalized >= .5 ? 0 : 1, root));
    attachParameterHelp(root, spec.id);
    attachParameterContext(root, spec.id);
    mount.append(root);
    controls[spec.id] = { type: "toggle", root };
  }

  function createChoice(spec, mount) {
    ensureState(spec.id, "choice");
    const root = element("div", "segment");
    root.dataset.param = spec.id;
    root.setAttribute("role", "radiogroup");
    root.setAttribute("aria-label", spec.label);
    attachParameterHelp(root, spec.id);
    attachParameterContext(root, spec.id);
    mount.append(root);
    controls[spec.id] = { type: "choice", root, buttons: [] };
    renderControl(spec.id);
  }

  function createMini(spec, mount) {
    ensureState(spec.id, "toggle");
    const root = element("button", "mini", spec.label);
    root.type = "button";
    root.dataset.param = spec.id;
    root.setAttribute("aria-label", spec.label);
    root.addEventListener("click", () => completeGesture(spec.id,
      ensureState(spec.id).normalized >= .5 ? 0 : 1, root));
    attachParameterHelp(root, spec.id);
    attachParameterContext(root, spec.id);
    mount.append(root);
    controls[spec.id] = { type: "mini", root };
  }

  function createBank(mountID, rows) {
    const mount = document.getElementById(mountID);
    const bank = element("div", "control-bank");
    rows.forEach((row) => {
      const rowNode = element("div", `control-row ${row.className || ""}`);
      row.controls.forEach((spec) => {
        if (spec.kind === "knob") createKnob(spec, rowNode);
        if (spec.kind === "toggle") createToggle(spec, rowNode);
        if (spec.kind === "choice") createChoice(spec, rowNode);
      });
      bank.append(rowNode);
    });
    mount.append(bank);
  }

  function createOutputLine(mount, gainID, muteID, soloID, label) {
    const line = element("div", "output-line");
    createKnob({ id: gainID, kind: "knob", label }, line);
    const miniBank = element("div", "mini-bank");
    createMini({ id: muteID, kind: "mini", label: "M" }, miniBank);
    createMini({ id: soloID, kind: "mini", label: "S" }, miniBank);
    line.append(miniBank);
    mount.append(line);
  }

  function createOutputBank() {
    const mount = document.getElementById("outputControls");
    const bank = element("div", "control-bank");
    createOutputLine(bank, "outputLGain", "outputLMute", "outputLSolo", "L GAIN");
    createOutputLine(bank, "outputRGain", "outputRMute", "outputRSolo", "R GAIN");
    const pan = element("div", "pan-row");
    createKnob({ id: "pan", kind: "knob", label: "PAN" }, pan);
    bank.append(pan);
    mount.append(bank);
  }

  function createRecordBank() {
    createToggle({ id: "recordEnable", kind: "toggle", label: "REC" },
      document.getElementById("recordControls"));
  }

  function buildControls() {
    createBank("inputControls", inputRows);
    createBank("processControls", processRows);
    createOutputBank();
    createRecordBank();
  }

  function ensureChoiceButtons(id, current) {
    const control = controls[id];
    if (!control || control.type !== "choice") return;
    const choices = Array.isArray(current.choices) ? current.choices : [];
    const currentSignature = choices.join("\u0001");
    if (control.signature === currentSignature) return;
    control.signature = currentSignature;
    control.buttons = [];
    control.root.replaceChildren();
    if (!choices.length) {
      const placeholder = element("button", "", "--");
      placeholder.type = "button";
      placeholder.disabled = true;
      control.root.append(placeholder);
      return;
    }
    choices.forEach((choice, index) => {
      const button = element("button", "", choice);
      button.type = "button";
      button.setAttribute("role", "radio");
      button.addEventListener("click", () => {
        const normalized = choices.length === 1 ? 0 : index / (choices.length - 1);
        completeGesture(id, normalized, control.root);
      });
      control.root.append(button);
      control.buttons.push(button);
    });
  }

  function renderControl(id) {
    const current = state[id];
    const control = controls[id];
    if (!current || !control) return;
    const normalized = clamp(Number(current.normalized) || 0);
    if (control.type === "knob") {
      control.root.classList.toggle("pending", Boolean(current.pending));
      control.dial.style.setProperty("--angle", `${-135 + normalized * 270}deg`);
      control.dial.style.setProperty("--sweep", `${normalized * 270}deg`);
      control.value.textContent = current.text || "--";
      control.dial.setAttribute("aria-valuenow", String(Math.round(normalized * 100)));
      control.dial.setAttribute("aria-valuetext", current.text || "--");
      return;
    }
    if (control.type === "choice") {
      ensureChoiceButtons(id, current);
      const count = control.buttons.length;
      const activeIndex = count > 1 ? Math.round(normalized * (count - 1)) : 0;
      control.buttons.forEach((button, index) => {
        const active = index === activeIndex;
        button.classList.toggle("on", active);
        button.setAttribute("aria-checked", active ? "true" : "false");
      });
      return;
    }
    const active = normalized >= .5;
    control.root.classList.toggle("on", active);
    if (control.type === "toggle") control.root.setAttribute("aria-checked", active ? "true" : "false");
    control.root.setAttribute("aria-pressed", active ? "true" : "false");
  }

  function setLocalNormalized(id, normalized) {
    const current = ensureState(id);
    current.normalized = clamp(normalized);
    current.pending = hasNativeBridge;
    renderControl(id);
  }

  function applyParameterRows(rows) {
    if (!Array.isArray(rows)) return;
    rows.forEach((row) => {
      if (!row || typeof row.id !== "string") return;
      const current = ensureState(row.id, row.kind);
      Object.assign(current, row, { pending: false });
      renderControl(row.id);
    });
  }

  function applyHero(hero) {
    if (!hero) return;
    document.getElementById("heroValue").textContent = hero.text || "--";
    document.getElementById("heroUnit").textContent = hero.label || "GAIN";
    document.getElementById("heroCue").textContent = hero.id ? `${hero.label || hero.id} / ACTIVE PARAMETER` : "STEREO PROCESSING";
  }

  function applyEngine(engine) {
    if (!engine) return;
    const sampleRate = Number(engine.sampleRate) || 0;
    const blockSize = Number(engine.blockSize) || 0;
    const label = sampleRate > 0 ? `${Math.round(sampleRate)} Hz / ${blockSize}` : "ENGINE OFFLINE";
    document.getElementById("engineStatus").textContent = label;
    document.getElementById("topEngine").textContent = label;
    document.getElementById("aboutEngine").textContent = label;
  }

  function applyRecord(record) {
    if (!record) return;
    const path = record.path || "NO FILE SELECTED";
    const pathNode = document.getElementById("recordPath");
    pathNode.textContent = path;
    pathNode.title = record.path || "";
    const stateNode = document.getElementById("recordState");
    stateNode.classList.remove("live", "error");
    let label = "STANDBY";
    if (record.error) {
      label = "WRITE ERROR";
      stateNode.classList.add("error");
    } else if (record.active) {
      label = "RECORDING";
      stateNode.classList.add("live");
    } else if (record.desired) {
      label = "ARMED";
    }
    stateNode.textContent = `${label} / DROP ${Math.max(0, Number(record.dropped) || 0)}`;
  }

  function dbToPercent(value) {
    const db = Number(value);
    if (!Number.isFinite(db)) return 0;
    return clamp((db + 60) / 60, 0, 1) * 100;
  }

  function setMeterHeight(id, db) {
    const node = document.getElementById(id);
    if (node) node.style.height = `${dbToPercent(db)}%`;
  }

  function setMeterMarker(id, db) {
    const node = document.getElementById(id);
    if (node) node.style.bottom = `calc(${dbToPercent(db)}% - 1px)`;
  }

  function formatLufs(value) {
    const number = Number(value);
    return Number.isFinite(number) && number > -70 ? number.toFixed(1) : "-70.0";
  }

  function applyMeter(frame) {
    if (!frame) return;
    setMeterHeight("heroMeterL", frame.peakL);
    setMeterHeight("heroMeterR", frame.peakR);
    setMeterHeight("meterLRms", frame.rmsL);
    setMeterHeight("meterRRms", frame.rmsR);
    setMeterMarker("meterLPeak", frame.peakL);
    setMeterMarker("meterRPeak", frame.peakR);
    const correlation = clamp(Number(frame.correlation) || 0, -1, 1);
    document.getElementById("correlation").textContent = correlation.toFixed(2);
    document.getElementById("lufsMomentary").textContent = formatLufs(frame.momentary);
    document.getElementById("lufsShort").textContent = formatLufs(frame.shortTerm);
    document.getElementById("lufsIntegrated").textContent = formatLufs(frame.integrated);
  }

  function scheduleScopeDraw() {
    if (!surfaceActive || scopeAnimationFrame !== null) return;
    scopeAnimationFrame = window.requestAnimationFrame(() => {
      scopeAnimationFrame = null;
      drawScope();
    });
  }

  function drawScope() {
    const canvas = document.getElementById("scope");
    const rect = canvas.getBoundingClientRect();
    const dpr = Math.min(window.devicePixelRatio || 1, 1.25);
    const width = Math.max(1, Math.round(rect.width * dpr));
    const height = Math.max(1, Math.round(rect.height * dpr));
    if (canvas.width !== width || canvas.height !== height) {
      canvas.width = width;
      canvas.height = height;
    }
    const context = canvas.getContext("2d");
    context.fillStyle = "#0a0a0a";
    context.fillRect(0, 0, width, height);
    context.strokeStyle = "rgba(250,250,248,.14)";
    context.lineWidth = Math.max(1, dpr);
    context.beginPath();
    for (let x = 1; x < 4; x += 1) {
      const coordinate = Math.round(width * x / 4) + .5;
      context.moveTo(coordinate, 0);
      context.lineTo(coordinate, height);
    }
    context.moveTo(0, Math.round(height / 2) + .5);
    context.lineTo(width, Math.round(height / 2) + .5);
    context.stroke();
    if (!scopeFrame || !Array.isArray(scopeFrame.left) || scopeFrame.left.length < 2) return;
    const count = Math.min(scopeFrame.left.length, scopeFrame.right.length);
    const mid = height / 2;
    const amplitude = height / 2 - 3 * dpr;
    const drawChannel = (samples, color) => {
      context.strokeStyle = color;
      context.lineWidth = 1.25 * dpr;
      context.beginPath();
      for (let index = 0; index < count; index += 1) {
        const x = index / (count - 1) * width;
        const y = mid - clamp(Number(samples[index]) || 0, -1, 1) * amplitude;
        if (index === 0) context.moveTo(x, y);
        else context.lineTo(x, y);
      }
      context.stroke();
    };
    drawChannel(scopeFrame.left, "#fafaf8");
    drawChannel(scopeFrame.right, "#002fa7");
  }

  function endAllGestures() {
    [...activeEnders].forEach((finish) => finish());
    Object.keys(wheelTimers).forEach((id) => {
      if (wheelTimers[id]) {
        window.clearTimeout(wheelTimers[id]);
        wheelTimers[id] = null;
        emit("parameterGesture", { id, phase: "end" });
      }
    });
  }

  function bindBridge() {
    nativeBackend.addEventListener("fullState", (payload) => {
      applyParameterRows(payload && payload.params);
      applyHero(payload && payload.hero);
      applyEngine(payload && payload.engine);
      applyRecord(payload && payload.record);
      if (payload && payload.version) {
        document.getElementById("versionStatus").textContent = payload.version;
        document.getElementById("topVersion").textContent = payload.version;
        document.getElementById("aboutVersion").textContent = payload.version;
      }
      document.getElementById("topState").textContent = "NATIVE SURFACE READY";
      document.getElementById("topMode").textContent = "NATIVE";
      document.getElementById("statusMessage").textContent = "REAL PARAMETER AND METER STATE";
    });
    nativeBackend.addEventListener("parameterState", (payload) => {
      applyParameterRows(payload && payload.params);
      applyHero(payload && payload.hero);
    });
    nativeBackend.addEventListener("meterFrame", applyMeter);
    nativeBackend.addEventListener("scopeFrame", (payload) => {
      scopeFrame = payload;
      scheduleScopeDraw();
    });
    nativeBackend.addEventListener("recordState", applyRecord);
    nativeBackend.addEventListener("surfaceVisibility", (payload) => {
      surfaceActive = !payload || payload.active !== false;
      if (!surfaceActive && scopeAnimationFrame !== null) {
        window.cancelAnimationFrame(scopeAnimationFrame);
        scopeAnimationFrame = null;
      }
      if (!surfaceActive) clearVisualFeedback();
      if (surfaceActive) scheduleScopeDraw();
    });
  }

  function bindAbout() {
    const layer = document.getElementById("aboutLayer");
    const open = document.getElementById("aboutOpen");
    const close = document.getElementById("aboutClose");
    const closeLayer = () => {
      layer.hidden = true;
      if (lastAboutInvoker) lastAboutInvoker.focus();
    };
    open.addEventListener("click", () => {
      hideParameterHelp();
      clearVisualFeedback();
      lastAboutInvoker = open;
      layer.hidden = false;
      close.focus();
    });
    close.addEventListener("click", closeLayer);
    layer.addEventListener("pointerdown", (event) => {
      if (event.target === layer) closeLayer();
    });
    document.addEventListener("keydown", (event) => {
      if (event.key === "Escape" && !layer.hidden) {
        event.preventDefault();
        closeLayer();
      }
    });
  }

  function boot() {
    createParameterHelp();
    buildControls();
    bindAbout();
    document.getElementById("recordBrowse").addEventListener("click", () => emit("recordBrowse", {}));
    window.addEventListener("pagehide", () => {
      endAllGestures();
      clearVisualFeedback();
    });
    window.addEventListener("resize", () => {
      scheduleScopeDraw();
      positionParameterHelp();
      clearVisualFeedback();
    });
    window.addEventListener("blur", () => hideParameterHelp());
    document.addEventListener("pointerdown", () => hideParameterHelp(), true);
    document.addEventListener("contextmenu", (event) => event.preventDefault());
    document.addEventListener("dragstart", (event) => event.preventDefault());
    document.addEventListener("dragover", (event) => event.preventDefault());
    document.addEventListener("drop", (event) => event.preventDefault());
    drawScope();

    if (!hasNativeBridge) {
      document.getElementById("topState").textContent = "STATIC PREVIEW / NO NATIVE BRIDGE";
      document.getElementById("topMode").textContent = "PREVIEW";
      document.getElementById("statusMessage").textContent = "PREVIEW USES NO SIMULATED TELEMETRY";
      return;
    }

    bindBridge();
    emit("uiReady", {});
  }

  boot();
})();
