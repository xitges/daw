/* APP — root layout, state, tweaks panel */

const { useState: useStateA, useEffect: useEffectA } = React;

const COLOR_SCHEMES = {
  cream_red: {
    label: "Cream / Red",
    chassis:"#e6dec9", chassis2:"#ddd3ba", chassisShadow:"#b9ad8c",
    panel:"#f3ecda", panelRim:"#c7bb9a",
    ink:"#1a1612", inkSoft:"#4a4338", inkFaint:"#8c8170",
    accent:"#d8412a", accent2:"#f06b54",
    displayBg:"#0d1410", displayFg:"#b9ff66",
  },
  slate_cyan: {
    label: "Slate / Cyan",
    chassis:"#3a4148", chassis2:"#2e353c", chassisShadow:"#1c2126",
    panel:"#454d54", panelRim:"#252a30",
    ink:"#e8edf2", inkSoft:"#a9b1ba", inkFaint:"#6d7680",
    accent:"#2cd4d4", accent2:"#5cf0f0",
    displayBg:"#0a1418", displayFg:"#7af0ff",
  },
  charcoal_amber: {
    label: "Charcoal / Amber",
    chassis:"#22201d", chassis2:"#1a1816", chassisShadow:"#0d0c0a",
    panel:"#2a2622", panelRim:"#0a0907",
    ink:"#e8dfd1", inkSoft:"#b8a98e", inkFaint:"#7d6f5a",
    accent:"#ff9020", accent2:"#ffb555",
    displayBg:"#1a0d05", displayFg:"#ff9530",
  },
  ivory_green: {
    label: "Ivory / Green",
    chassis:"#ece7da", chassis2:"#e0d9c8", chassisShadow:"#b9b09a",
    panel:"#f5f0e2", panelRim:"#c8bfa6",
    ink:"#1f2218", inkSoft:"#4a4d3e", inkFaint:"#85886e",
    accent:"#3da356", accent2:"#5cc874",
    displayBg:"#0a0f0a", displayFg:"#b9ff66",
  },
};

function applyScheme(name) {
  const s = COLOR_SCHEMES[name] || COLOR_SCHEMES.cream_red;
  const root = document.documentElement;
  root.style.setProperty("--chassis", s.chassis);
  root.style.setProperty("--chassis-2", s.chassis2);
  root.style.setProperty("--chassis-shadow", s.chassisShadow);
  root.style.setProperty("--panel", s.panel);
  root.style.setProperty("--panel-rim", s.panelRim);
  root.style.setProperty("--ink", s.ink);
  root.style.setProperty("--ink-soft", s.inkSoft);
  root.style.setProperty("--ink-faint", s.inkFaint);
  root.style.setProperty("--accent", s.accent);
  root.style.setProperty("--accent-2", s.accent2);
  root.style.setProperty("--display-bg", s.displayBg);
  root.style.setProperty("--display-fg", s.displayFg);
}

function InspectorTabs({ value, onChange, currentTrack, currentPattern }) {
  const tabs = [
    { id:"synth",     label:"INSTRUMENT", sub:`CH ${currentTrack.idx} — ${currentTrack.name}` },
    { id:"sequencer", label:"SEQUENCER",  sub: currentPattern },
    { id:"mixer",     label:"MIXER",      sub:"8 INSERT · 1 MASTER" },
  ];
  return (
    <div style={{
      display:"flex", gap:0, alignItems:"flex-end",
      padding:"0 12px",
      background:"linear-gradient(180deg, var(--chassis-2), var(--chassis))",
      borderTop:"1px solid var(--panel-rim)",
      borderBottom:"1px solid var(--panel-rim)",
      position:"relative",
    }}>
      {tabs.map(t => {
        const on = value === t.id;
        return (
          <button key={t.id} onClick={() => onChange(t.id)} style={{
            padding:"7px 16px 8px",
            border:"none", cursor:"pointer",
            background: on ? "var(--panel)" : "transparent",
            borderTop: on ? "2px solid var(--accent)" : "2px solid transparent",
            borderLeft: on ? "1px solid var(--panel-rim)" : "1px solid transparent",
            borderRight: on ? "1px solid var(--panel-rim)" : "1px solid transparent",
            borderBottom: on ? "1px solid var(--panel)" : "none",
            marginBottom: on ? -1 : 0,
            display:"flex", flexDirection:"column", alignItems:"flex-start", gap:2,
            fontFamily:'"JetBrains Mono", monospace',
            position:"relative",
            boxShadow: on ? "inset 0 1px 0 rgba(255,255,255,0.5)" : "none",
          }}>
            <span style={{
              fontSize:10, fontWeight:700, letterSpacing:"0.18em",
              color: on ? "var(--ink)" : "var(--ink-soft)",
              textTransform:"uppercase",
            }}>{t.label}</span>
            <span style={{
              fontSize:8, fontWeight:600, letterSpacing:"0.1em",
              color: on ? "var(--accent)" : "var(--ink-faint)",
              textTransform:"uppercase",
              maxWidth:160, overflow:"hidden", textOverflow:"ellipsis", whiteSpace:"nowrap",
            }}>{t.sub}</span>
          </button>
        );
      })}
      <div style={{ flex:1 }} />
      <div style={{
        padding:"8px 12px",
        display:"flex", alignItems:"center", gap:8,
        fontSize:9, letterSpacing:"0.12em", color:"var(--ink-faint)",
      }}>
        <Tag>INSPECTOR</Tag>
        <Tag>CH {currentTrack.idx.toString().padStart(2,"0")} — {currentTrack.name}</Tag>
      </div>
    </div>
  );
}

function App() {
  const TWEAK_DEFAULTS = /*EDITMODE-BEGIN*/{
    "scheme": "cream_red",
    "showInspector": true
  }/*EDITMODE-END*/;

  const [tweaks, setTweak] = useTweaks(TWEAK_DEFAULTS);

  const [playing, setPlaying] = useStateA(false);
  const [recording, setRecording] = useStateA(false);
  const [looping, setLooping] = useStateA(true);
  const [bpm] = useStateA(124.0);
  const [playMode, setPlayMode] = useStateA("PAT");
  const [playhead, setPlayhead] = useStateA(8.4);
  const [step, setStep] = useStateA(0);
  const [inspector, setInspector] = useStateA("sequencer");
  const [mixerOpen, setMixerOpen] = useStateA(false);

  const [patterns] = useStateA(["PAT 01 — verse","PAT 02 — chorus","PAT 03 — bridge","PAT 04 — riser"]);
  const [currentPattern, setCurrentPattern] = useStateA("PAT 01 — verse");

  const [midiOn, setMidi] = useStateA(true);
  const [audioOn, setAudio] = useStateA(true);
  const [padOn, setPad] = useStateA(false);
  const [touchOn, setTouch] = useStateA(false);
  const [liveOn, setLive] = useStateA(false);

  const currentTrack = { idx: 8, name: "POLY-6" };

  useEffectA(() => { applyScheme(tweaks.scheme); }, [tweaks.scheme]);

  useEffectA(() => {
    if (!playing) return;
    let last = performance.now();
    let raf;
    const tick = (now) => {
      const dt = (now - last) / 1000; last = now;
      const barsPerSec = bpm / 60 / 4;
      setPlayhead(p => {
        const next = p + dt * barsPerSec;
        return next >= 32 ? (looping ? 0 : 32) : next;
      });
      raf = requestAnimationFrame(tick);
    };
    raf = requestAnimationFrame(tick);
    return () => cancelAnimationFrame(raf);
  }, [playing, bpm, looping]);

  useEffectA(() => {
    if (!playing) return;
    const stepsPerSec = bpm / 60 * 4;
    const id = setInterval(() => setStep(s => (s + 1) % 16), 1000 / stepsPerSec);
    return () => clearInterval(id);
  }, [playing, bpm]);

  const position = (() => {
    const bar = Math.floor(playhead) + 1;
    const beat = Math.floor((playhead % 1) * 4) + 1;
    const tick = Math.floor(((playhead % 1) * 4 % 1) * 240);
    return { bar, beat, tick };
  })();

  return (
    <div style={{
      display:"flex", flexDirection:"column", gap:12,
      padding:14,
      background: "linear-gradient(180deg, var(--chassis) 0%, var(--chassis-2) 100%)",
      borderRadius:18,
      border:"1px solid var(--chassis-shadow)",
      boxShadow:`
        0 1px 0 rgba(255,255,255,0.4) inset,
        0 -2px 0 rgba(0,0,0,0.15) inset,
        0 20px 60px rgba(0,0,0,0.5),
        0 4px 0 var(--chassis-shadow)
      `,
      position:"relative",
    }}>
      <div style={{ position:"absolute", top:8, left:8 }}><Screw /></div>
      <div style={{ position:"absolute", top:8, right:8 }}><Screw /></div>
      <div style={{ position:"absolute", bottom:8, left:8 }}><Screw /></div>
      <div style={{ position:"absolute", bottom:8, right:8 }}><Screw /></div>

      <Transport
        playing={playing} recording={recording} looping={looping}
        bpm={bpm} position={position} playMode={playMode}
        onPlay={() => setPlaying(p => !p)}
        onStop={() => { setPlaying(false); setPlayhead(0); setStep(0); }}
        onRec={() => setRecording(r => !r)}
        onLoop={() => setLooping(l => !l)}
        onScrub={(d) => setPlayhead(p => Math.max(0, Math.min(32, p + d)))}
        onPlayMode={setPlayMode}
        patterns={patterns}
        currentPattern={currentPattern}
        onPattern={setCurrentPattern}
        onPatternNew={() => {}} onPatternDup={() => {}} onPatternDel={() => {}} onPatternRen={() => {}}
        onNewProject={() => {}} onOpen={() => {}} onSave={() => {}} onSaveAs={() => {}} onExport={() => {}}
        mixerOpen={mixerOpen} onToggleMixer={() => setMixerOpen(o => !o)}
        midiOn={midiOn} audioOn={audioOn} padOn={padOn} touchOn={touchOn} liveOn={liveOn}
        onMidi={() => setMidi(v => !v)} onAudio={() => setAudio(v => !v)}
        onPad={() => setPad(v => !v)} onTouch={() => setTouch(v => !v)} onLive={() => setLive(v => !v)}
      />

      {/* main 2-column layout */}
      <div style={{
        display:"grid",
        gridTemplateColumns:"260px 1fr",
        gap:12,
        minHeight: 760,
      }}>
        {/* LEFT RAIL: browser */}
        <div style={{ display:"flex", flexDirection:"column", gap:10, minWidth:0, minHeight:0 }}>
          <SampleBrowser />
        </div>

        {/* MAIN: arrangement (top) + inspector (bottom) */}
        <div style={{ display:"flex", flexDirection:"column", gap:0, minWidth:0, minHeight:0,
          background:"var(--panel)",
          borderRadius:8,
          border:"1px solid var(--panel-rim)",
          boxShadow:"inset 0 1px 0 rgba(255,255,255,0.5), 0 1px 0 rgba(255,255,255,0.4)",
          overflow:"hidden",
        }}>
          <div style={{ flex: tweaks.showInspector ? "1 1 55%" : "1 1 100%", display:"flex", flexDirection:"column", minHeight:0 }}>
            <Timeline
              playing={playing}
              playhead={playhead}
              onScrub={(p) => setPlayhead(Math.max(0, Math.min(200, p)))}
            />
          </div>

          {tweaks.showInspector && (
            <>
              <InspectorTabs value={inspector} onChange={setInspector}
                currentTrack={currentTrack} currentPattern={currentPattern} />
              <div style={{
                flex:"0 0 auto",
                background: "linear-gradient(180deg, var(--chassis), var(--chassis-2))",
                padding:12,
                borderBottom:"1px solid var(--panel-rim)",
                maxHeight: 380, overflow:"auto",
              }} className="scroll">
                {inspector === "synth" && <SynthPanel />}
                {inspector === "sequencer" && <SequencerBar playing={playing} currentStep={step} />}
                {inspector === "mixer" && <Mixer />}
              </div>
            </>
          )}
        </div>
      </div>

      <div style={{
        display:"flex", justifyContent:"space-between", alignItems:"center",
        padding:"4px 8px",
        fontSize:9, letterSpacing:"0.15em", color:"var(--ink-faint)", textTransform:"uppercase",
      }}>
        <div>xitges studio · juce daw · 44.1k 24b</div>
        <div style={{ display:"flex", gap:14 }}>
          <span>USB-C · MIDI · CV 1/8" × 4</span>
          <span>v 0.4.1</span>
        </div>
      </div>

      <TweaksPanel title="Tweaks">
        <TweakSection label="Color scheme">
          <TweakSelect
            label="Preset"
            value={tweaks.scheme}
            onChange={(v) => setTweak("scheme", v)}
            options={Object.entries(COLOR_SCHEMES).map(([k,v]) => ({ value:k, label:v.label }))}
          />
          <div style={{ display:"grid", gridTemplateColumns:"repeat(4, 1fr)", gap:6, marginTop:8 }}>
            {Object.entries(COLOR_SCHEMES).map(([k,v]) => (
              <button key={k} onClick={() => setTweak("scheme", k)}
                style={{
                  height:36, padding:0, borderRadius:6, cursor:"pointer",
                  background: `linear-gradient(180deg, ${v.chassis}, ${v.chassis2})`,
                  border: tweaks.scheme === k ? `2px solid ${v.accent}` : "1px solid rgba(0,0,0,0.2)",
                  position:"relative",
                  boxShadow: tweaks.scheme === k ? `0 0 8px ${v.accent}66` : "none",
                }}>
                <div style={{ position:"absolute", left:4, bottom:4, width:8, height:8, borderRadius:2, background:v.accent, boxShadow:`0 0 4px ${v.accent}` }} />
              </button>
            ))}
          </div>
        </TweakSection>
        <TweakSection label="Layout">
          <TweakToggle
            label="Show inspector"
            value={tweaks.showInspector}
            onChange={(v) => setTweak("showInspector", v)}
          />
        </TweakSection>
      </TweaksPanel>
    </div>
  );
}

const root = ReactDOM.createRoot(document.getElementById("root"));
root.render(<App />);
