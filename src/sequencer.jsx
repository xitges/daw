/* CHANNEL RACK — 16-channel step sequencer
 * Per channel: name, type (drum/melodic), vol/pan/pitch sliders, ADSR mini, mute/solo, mixer route
 * Step grid: 1..64 steps (1/16), per-step expression panel (vel/gate/prob/pitch/cutoff/start)
 * Variations: A/B/C/D
 */

const { useState: useStateSQ } = React;

const RACK_COLORS = ["#d8412a","#e89c2b","#f0c14a","#7ab87a","#3da356","#5fa8d8","#2cd4d4","#7a8fe0","#b87ad6","#e07ac8","#a8a098","#d8c8a8","#e89c2b","#7ab87a","#5fa8d8","#b87ad6"];

function ChannelRow({ idx, channel, currentStep, playing, selected, onSelect, onToggleStep, onMute, onSolo }) {
  const STEPS = channel.steps;
  return (
    <div style={{
      display:"grid", gridTemplateColumns:"180px 1fr", gap:0, alignItems:"stretch",
      borderBottom:"1px solid var(--rule)",
      background: selected
        ? "linear-gradient(90deg, rgba(216,65,42,0.08), transparent 60%)"
        : (idx % 2 === 0 ? "rgba(0,0,0,0.02)" : "transparent"),
    }}>
      {/* CHANNEL HEAD */}
      <div onClick={() => onSelect(idx)} style={{
        display:"grid", gridTemplateColumns:"22px 1fr auto", gap:6, alignItems:"center",
        padding:"4px 8px", cursor:"pointer",
        borderRight:"1px solid var(--rule)",
        borderLeft: selected ? "2px solid var(--accent)" : "2px solid transparent",
      }}>
        <div style={{
          width:18, height:18, borderRadius:3,
          background: channel.color,
          boxShadow:`0 0 4px ${channel.color}66, inset 0 1px 0 rgba(255,255,255,0.4)`,
          border:"1px solid rgba(0,0,0,0.4)",
          display:"flex", alignItems:"center", justifyContent:"center",
          fontSize:8, fontWeight:700, color:"#fff", fontFamily:'"JetBrains Mono", monospace',
        }}>{idx+1}</div>

        <div style={{ minWidth:0 }}>
          <div style={{ display:"flex", alignItems:"center", gap:4 }}>
            <span style={{
              fontSize:10, fontWeight:700, color:"var(--ink)", letterSpacing:"0.04em",
              textTransform:"uppercase", overflow:"hidden", textOverflow:"ellipsis", whiteSpace:"nowrap",
            }}>{channel.name}</span>
            <span style={{
              fontSize:7, fontWeight:700, letterSpacing:"0.12em",
              padding:"1px 4px", borderRadius:2,
              background: channel.type === "MEL" ? "rgba(216,65,42,0.15)" : "rgba(0,0,0,0.08)",
              color: channel.type === "MEL" ? "var(--accent)" : "var(--ink-soft)",
              border: "1px solid " + (channel.type === "MEL" ? "var(--accent)55" : "rgba(0,0,0,0.15)"),
            }}>{channel.type}</span>
          </div>
          {/* mini sliders: vol / pan / pitch */}
          <div style={{ display:"grid", gridTemplateColumns:"auto 1fr auto 1fr auto 1fr", gap:3, alignItems:"center", marginTop:3, fontFamily:'"JetBrains Mono", monospace' }}>
            <span style={{ fontSize:7, fontWeight:600, color:"var(--ink-faint)", letterSpacing:"0.1em" }}>V</span>
            <MiniSlider value={channel.vol} color={channel.color} />
            <span style={{ fontSize:7, fontWeight:600, color:"var(--ink-faint)", letterSpacing:"0.1em" }}>P</span>
            <MiniSlider value={channel.pan} bipolar color={channel.color} />
            <span style={{ fontSize:7, fontWeight:600, color:"var(--ink-faint)", letterSpacing:"0.1em" }}>♯</span>
            <MiniSlider value={channel.pitch} bipolar color={channel.color} />
          </div>
          <div style={{ display:"flex", alignItems:"center", gap:4, marginTop:3 }}>
            <button onClick={(e) => { e.stopPropagation(); onMute(idx); }} style={msrBtn(channel.muted, "var(--led-amber)")}>M</button>
            <button onClick={(e) => { e.stopPropagation(); onSolo(idx); }} style={msrBtn(channel.soloed, "var(--led-green)")}>S</button>
            <span style={{ fontSize:7, fontWeight:600, color:"var(--ink-faint)", letterSpacing:"0.1em" }}>→ MX{channel.bus}</span>
            <span style={{ fontSize:7, fontWeight:600, color:"var(--ink-faint)", letterSpacing:"0.1em", marginLeft:"auto" }}>{STEPS}st</span>
          </div>
        </div>

        <div style={{ display:"flex", alignItems:"center", gap:2 }}>
          <button title="Piano roll" style={tinyBtn()}>♪</button>
          <button title="Channel options" style={tinyBtn()}>⋯</button>
        </div>
      </div>

      {/* STEP GRID */}
      <div style={{ padding:"6px 8px", overflow:"hidden" }}>
        <div className="scroll" style={{ overflowX:"auto" }}>
          <div style={{
            display:"grid",
            gridTemplateColumns:`repeat(${STEPS}, minmax(16px, 1fr))`,
            gap:3,
            minWidth: STEPS * 18,
          }}>
            {channel.pattern.slice(0, STEPS).map((cell, ci) => {
              const on = cell.on;
              const isCurrent = ci === currentStep && playing;
              const isQuarter = ci % 4 === 0;
              const isHalfBar = ci % 8 === 0;
              return (
                <button key={ci} onClick={() => onToggleStep(idx, ci)} style={{
                  height:24, padding:0, borderRadius:3,
                  border: "1px solid " + (isHalfBar ? "rgba(0,0,0,0.4)" : "rgba(0,0,0,0.2)"),
                  background: on
                    ? `linear-gradient(180deg, ${channel.color}, ${channel.color}cc)`
                    : isQuarter ? "rgba(0,0,0,0.16)" : "rgba(0,0,0,0.06)",
                  boxShadow: on
                    ? `0 0 5px ${channel.color}88, inset 0 1px 0 rgba(255,255,255,0.4)`
                    : "inset 0 1px 1px rgba(0,0,0,0.25)",
                  cursor:"pointer",
                  outline: isCurrent ? "2px solid var(--accent)" : "none",
                  outlineOffset: isCurrent ? 1 : 0,
                  position:"relative",
                }}>
                  {/* velocity bar inside cell */}
                  {on && (
                    <div style={{
                      position:"absolute", left:1, right:1, bottom:1,
                      height: Math.max(2, cell.vel * 4),
                      background:"rgba(255,255,255,0.55)",
                      borderRadius:1,
                    }} />
                  )}
                </button>
              );
            })}
          </div>
        </div>
      </div>
    </div>
  );
}

function MiniSlider({ value=0.5, color="var(--accent)", bipolar=false }) {
  const norm = Math.max(0, Math.min(1, value));
  const center = bipolar ? 0.5 : 0;
  return (
    <div style={{
      height:5, borderRadius:2,
      background:"var(--display-bg)",
      boxShadow:"inset 0 1px 2px rgba(0,0,0,0.6)",
      position:"relative",
      border:"1px solid rgba(0,0,0,0.4)",
    }}>
      {bipolar ? (
        <div style={{
          position:"absolute", top:0, bottom:0,
          left: norm > center ? "50%" : `${norm*100}%`,
          right: norm > center ? `${100 - norm*100}%` : "50%",
          background: color,
          boxShadow:`0 0 3px ${color}88`,
          borderRadius:1,
        }} />
      ) : (
        <div style={{
          position:"absolute", top:0, bottom:0, left:0, width:`${norm*100}%`,
          background: color,
          boxShadow:`0 0 3px ${color}88`,
          borderRadius:1,
        }} />
      )}
    </div>
  );
}

function msrBtn(on, c) {
  return {
    width:14, height:11, padding:0, borderRadius:2,
    fontSize:7, fontWeight:700, fontFamily:'"JetBrains Mono", monospace',
    background: on ? c : "rgba(0,0,0,0.06)",
    color: on ? "rgba(0,0,0,0.85)" : "var(--ink-soft)",
    border:"1px solid rgba(0,0,0,0.2)",
    cursor:"pointer",
    boxShadow: on ? `0 0 3px ${c}` : "none",
  };
}
function tinyBtn() {
  return {
    width:18, height:18, padding:0, borderRadius:3, cursor:"pointer",
    background:"linear-gradient(180deg, #ede5d1, #d8cdb1)",
    color:"var(--ink)",
    border:"1px solid rgba(0,0,0,0.2)",
    boxShadow:"inset 0 1px 0 rgba(255,255,255,0.5)",
    fontFamily:'"VT323", monospace', fontSize:13, fontWeight:700,
    display:"flex", alignItems:"center", justifyContent:"center",
  };
}

function StepInspector({ step }) {
  const params = [
    { k:"VEL",  v:step.vel,  unit:"" },
    { k:"GATE", v:step.gate, unit:"%" },
    { k:"PROB", v:step.prob, unit:"%" },
    { k:"PITCH", v:step.pitchOff, bipolar:true, unit:"st" },
    { k:"CUT",  v:step.cutMod, bipolar:true, unit:"" },
    { k:"START", v:step.startOff, unit:"%" },
  ];
  return (
    <div style={{
      display:"flex", flexDirection:"column", gap:6,
      padding:"8px 10px", minWidth:170,
      background:"linear-gradient(180deg, var(--chassis), var(--chassis-2))",
      borderRadius:6, border:"1px solid var(--panel-rim)",
      boxShadow:"inset 0 1px 0 rgba(255,255,255,0.5)",
    }}>
      <div style={{ display:"flex", alignItems:"center", justifyContent:"space-between" }}>
        <span style={{ fontSize:9, fontWeight:700, letterSpacing:"0.16em", color:"var(--ink)" }}>STEP INSPECTOR</span>
        <span style={{ fontSize:8, fontWeight:600, letterSpacing:"0.12em", color:"var(--accent)" }}>● S{step.idx+1}</span>
      </div>
      <div style={{ display:"grid", gridTemplateColumns:"1fr 1fr", gap:6 }}>
        {params.map(p => (
          <div key={p.k} style={{
            display:"flex", flexDirection:"column", gap:2,
            padding:"4px 6px", borderRadius:3,
            background:"rgba(0,0,0,0.04)",
          }}>
            <div style={{ display:"flex", justifyContent:"space-between" }}>
              <span style={{ fontSize:7, fontWeight:700, letterSpacing:"0.14em", color:"var(--ink-faint)" }}>{p.k}</span>
              <span style={{ fontFamily:'"VT323",monospace', fontSize:11, color:"var(--accent)", lineHeight:1 }}>
                {p.bipolar ? (p.v >= 0 ? "+" : "") : ""}{(p.v).toFixed(p.unit === "st" ? 1 : 2)}{p.unit}
              </span>
            </div>
            <MiniSlider value={p.bipolar ? (p.v + 1) / 2 : p.v} bipolar={p.bipolar} color="var(--accent)" />
          </div>
        ))}
      </div>
    </div>
  );
}

function VariationButtons({ active, onChange }) {
  return (
    <div style={{ display:"flex", gap:3 }}>
      {["A","B","C","D"].map(p => (
        <button key={p} onClick={() => onChange(p)} style={{
          width:24, height:20, padding:0, borderRadius:3,
          fontSize:9, fontWeight:700, fontFamily:'"JetBrains Mono", monospace',
          background: active === p ? "var(--accent)" : "rgba(0,0,0,0.06)",
          color: active === p ? "#fff" : "var(--ink-soft)",
          border:"1px solid " + (active === p ? "var(--accent)" : "rgba(0,0,0,0.2)"),
          cursor:"pointer",
          boxShadow: active === p ? "0 0 4px var(--accent)" : "none",
        }}>{p}</button>
      ))}
    </div>
  );
}

function SequencerBar({ playing, currentStep }) {
  // build 16 channels with realistic mix of drum + melodic
  const channelDefs = [
    { name:"Kick",       type:"DRUM", steps:16, vol:0.8, pan:0.5, pitch:0.5, bus:1, pattern:p([1,0,0,0,1,0,0,0,1,0,0,0,1,0,1,0]) },
    { name:"Snare",      type:"DRUM", steps:16, vol:0.7, pan:0.5, pitch:0.5, bus:1, pattern:p([0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,1]) },
    { name:"Hat Closed", type:"DRUM", steps:16, vol:0.6, pan:0.6, pitch:0.5, bus:1, pattern:p([1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0]) },
    { name:"Hat Open",   type:"DRUM", steps:16, vol:0.5, pan:0.4, pitch:0.5, bus:1, pattern:p([0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0]) },
    { name:"Clap",       type:"DRUM", steps:16, vol:0.65, pan:0.5, pitch:0.5, bus:1, pattern:p([0,0,0,0,1,0,0,0,0,0,0,1,1,0,0,0]) },
    { name:"Perc",       type:"DRUM", steps:16, vol:0.5, pan:0.7, pitch:0.55, bus:2, pattern:p([0,0,1,0,0,0,0,1,0,1,0,0,0,0,1,0]) },
    { name:"Sub Bass",   type:"MEL",  steps:16, vol:0.85, pan:0.5, pitch:0.4, bus:3, pattern:p([1,0,0,0,1,0,0,1,1,0,0,0,1,0,1,0]) },
    { name:"Poly-6",     type:"MEL",  steps:32, vol:0.65, pan:0.5, pitch:0.5, bus:4, pattern:p([1,0,0,1, 0,1,0,0, 1,0,0,1, 0,0,1,0, 1,0,0,1, 0,1,0,0, 1,0,0,1, 0,0,1,0]) },
    { name:"Pad",        type:"MEL",  steps:16, vol:0.55, pan:0.5, pitch:0.5, bus:4, pattern:p([1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0]) },
    { name:"Lead VST",   type:"MEL",  steps:16, vol:0.6, pan:0.55, pitch:0.5, bus:5, pattern:p([0,0,0,0,0,0,0,0,1,0,1,0,0,1,0,0]) },
    { name:"Vox Chop",   type:"MEL",  steps:16, vol:0.55, pan:0.5, pitch:0.5, bus:6, pattern:p([0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0]) },
    { name:"FX Riser",   type:"DRUM", steps:16, vol:0.4, pan:0.5, pitch:0.5, bus:7, pattern:p([0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1]) },
    { name:"Tom Lo",     type:"DRUM", steps:16, vol:0.5, pan:0.4, pitch:0.45, bus:2, pattern:p([0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1]), muted:true },
    { name:"Cymbal",     type:"DRUM", steps:16, vol:0.4, pan:0.6, pitch:0.5, bus:2, pattern:p([1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]) },
    { name:"Field Rec",  type:"DRUM", steps:16, vol:0.35, pan:0.5, pitch:0.5, bus:8, pattern:p([1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]) },
    { name:"Empty",      type:"DRUM", steps:16, vol:0.5, pan:0.5, pitch:0.5, bus:8, pattern:p([0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]) },
  ];
  const channels = channelDefs.map((c, i) => ({ ...c, color: RACK_COLORS[i] }));

  const [grid, setGrid] = useStateSQ(channels);
  const [selected, setSelected] = useStateSQ(0);
  const [variation, setVariation] = useStateSQ("A");
  const [selectedStep, setSelectedStep] = useStateSQ({ ch:0, idx:0 });

  const toggle = (chIdx, stepIdx) => {
    setGrid(g => g.map((c, ci) => ci !== chIdx ? c : {
      ...c, pattern: c.pattern.map((s, si) => si !== stepIdx ? s : { ...s, on: !s.on })
    }));
    setSelectedStep({ ch: chIdx, idx: stepIdx });
  };
  const mute = i => setGrid(g => g.map((c, ci) => ci === i ? { ...c, muted:!c.muted } : c));
  const solo = i => setGrid(g => g.map((c, ci) => ci === i ? { ...c, soloed:!c.soloed } : c));

  const stepData = (() => {
    const c = grid[selectedStep.ch];
    const s = c.pattern[selectedStep.idx] || {};
    return { ...s, idx: selectedStep.idx };
  })();

  return (
    <div style={{ display:"grid", gridTemplateColumns:"1fr auto", gap:10, alignItems:"stretch" }}>
      <div style={{
        background:"var(--panel)",
        border:"1px solid var(--panel-rim)",
        borderRadius:6,
        boxShadow:"inset 0 1px 0 rgba(255,255,255,0.5)",
        overflow:"hidden",
        display:"flex", flexDirection:"column",
      }}>
        {/* header */}
        <div style={{
          display:"flex", alignItems:"center", justifyContent:"space-between", gap:10,
          padding:"6px 10px",
          background:"linear-gradient(180deg, var(--chassis), var(--chassis-2))",
          borderBottom:"1px solid var(--panel-rim)",
        }}>
          <div style={{ display:"flex", alignItems:"center", gap:8 }}>
            <div style={{ width:7, height:7, borderRadius:2, background:"var(--accent)" }} />
            <span style={{ fontSize:10, fontWeight:700, letterSpacing:"0.16em", color:"var(--ink)" }}>CHANNEL RACK</span>
            <Tag>16 / 16 CH</Tag>
            <Tag>1·16 STEP · 1/16</Tag>
          </div>
          <div style={{ display:"flex", alignItems:"center", gap:8 }}>
            <span style={{ fontSize:8, fontWeight:700, letterSpacing:"0.14em", color:"var(--ink-faint)" }}>VARIATION</span>
            <VariationButtons active={variation} onChange={setVariation} />
            <button title="Add channel" style={tinyBtn()}>＋</button>
          </div>
        </div>

        {/* step number ruler */}
        <div style={{ display:"grid", gridTemplateColumns:"180px 1fr", borderBottom:"1px solid var(--rule)", background:"rgba(0,0,0,0.03)" }}>
          <div style={{
            padding:"3px 8px",
            fontSize:8, fontWeight:700, letterSpacing:"0.14em", color:"var(--ink-faint)",
            borderRight:"1px solid var(--rule)",
          }}>CH · NAME · MIX BUS</div>
          <div style={{ padding:"3px 8px", overflow:"hidden" }}>
            <div className="scroll" style={{ overflowX:"hidden" }}>
              <div style={{ display:"grid", gridTemplateColumns:"repeat(16, minmax(16px, 1fr))", gap:3, minWidth: 16*18 }}>
                {Array.from({length:16}).map((_,i) => (
                  <div key={i} style={{
                    textAlign:"center",
                    fontSize:7, fontWeight:700, letterSpacing:"0.05em",
                    color: i === currentStep && playing ? "var(--accent)" : (i % 4 === 0 ? "var(--ink)" : "var(--ink-faint)"),
                  }}>{(i+1).toString().padStart(2,"0")}</div>
                ))}
              </div>
            </div>
          </div>
        </div>

        {/* channels */}
        <div className="scroll" style={{ flex:1, overflowY:"auto", maxHeight:340 }}>
          {grid.map((c, i) => (
            <ChannelRow key={i} idx={i} channel={c}
              currentStep={currentStep} playing={playing}
              selected={selected === i}
              onSelect={setSelected}
              onToggleStep={toggle}
              onMute={mute} onSolo={solo}
            />
          ))}
        </div>
      </div>

      {/* right side: step inspector */}
      <StepInspector step={stepData} />
    </div>
  );
}

function p(arr) {
  // build pattern of step objects with default expression values, sized to 64
  return Array.from({length:64}, (_, i) => ({
    on: !!arr[i],
    vel: 0.78 + (i % 4 === 0 ? 0.1 : 0),
    gate: 0.5,
    prob: 1.0,
    pitchOff: 0,
    cutMod: 0,
    startOff: 0,
  }));
}

Object.assign(window, { SequencerBar });
