/* TRANSPORT — Row 1: tape reels + transport + BPM + meters
                Row 2: pattern mgmt + file ops + util buttons (MIDI/AUDIO/PAD/TOUCH/LIVE) */

const { useEffect: useEffectT, useState: useStateT, useRef: useRefT } = React;

function TapeReel({ spinning=false, size=58, side="L" }) {
  const [angle, setAngle] = useStateT(0);
  const rafRef = useRefT();
  useEffectT(() => {
    if (!spinning) return;
    let last = performance.now();
    const tick = (now) => {
      const dt = now - last; last = now;
      setAngle(a => (a + dt * 0.18) % 360);
      rafRef.current = requestAnimationFrame(tick);
    };
    rafRef.current = requestAnimationFrame(tick);
    return () => cancelAnimationFrame(rafRef.current);
  }, [spinning]);

  return (
    <div style={{ width:size, height:size, position:"relative" }}>
      <div style={{ position:"absolute", inset:0, borderRadius:"50%",
        background:"radial-gradient(circle at 30% 25%, #4a4338, #1a1612 70%, #0a0805)",
        boxShadow:"0 2px 4px rgba(0,0,0,0.6), inset 0 1px 0 rgba(255,255,255,0.1)",
        border:"1px solid rgba(0,0,0,0.6)" }} />
      <div style={{ position:"absolute", inset:size*0.12, borderRadius:"50%",
        background:"radial-gradient(circle, #2a1f15 0%, #1a1208 60%, #0d0805 100%)",
        boxShadow:"inset 0 0 8px rgba(0,0,0,0.8)" }} />
      <div style={{ position:"absolute", inset:0,
        transform:`rotate(${angle}deg)`,
        transition: spinning ? "none" : "transform 600ms ease-out" }}>
        {[0,60,120].map(a => (
          <div key={a} style={{ position:"absolute", left:"50%", top:"50%",
            width: size*0.7, height:3,
            background:"linear-gradient(90deg, #c7bb9a, #888070)",
            transform:`translate(-50%,-50%) rotate(${a}deg)`, borderRadius:1,
            boxShadow:"0 1px 1px rgba(0,0,0,0.6)" }} />
        ))}
        <div style={{ position:"absolute", left:"50%", top:"50%",
          width:size*0.28, height:size*0.28,
          transform:"translate(-50%,-50%)", borderRadius:"50%",
          background:"radial-gradient(circle at 30% 25%, #f3ecda, #c7bb9a 60%, #8d8268)",
          boxShadow:"0 1px 2px rgba(0,0,0,0.5), inset 0 1px 0 rgba(255,255,255,0.6)",
          border:"1px solid rgba(0,0,0,0.5)" }}>
          <div style={{ position:"absolute", left:"50%", top:"50%",
            width:3, height:3, borderRadius:"50%", background:"var(--accent)",
            transform:"translate(-50%,-50%)" }} />
        </div>
      </div>
    </div>
  );
}

function TransportBtn({ icon, active=false, onClick, color, size=34, title }) {
  return (
    <button onClick={onClick} title={title}
      style={{
        width:size, height:size, borderRadius:6, padding:0,
        background: active && color
          ? `radial-gradient(circle at 30% 25%, ${color}cc, ${color} 60%, ${color}88)`
          : "linear-gradient(180deg, #f3ecda 0%, #c7bb9a 60%, #8d8268 100%)",
        border:"1px solid rgba(0,0,0,0.5)",
        boxShadow: active
          ? `0 0 12px ${color}, inset 0 1px 0 rgba(255,255,255,0.4), inset 0 -1px 2px rgba(0,0,0,0.3)`
          : "0 2px 0 rgba(0,0,0,0.5), inset 0 1px 0 rgba(255,255,255,0.6), inset 0 -2px 4px rgba(0,0,0,0.2)",
        cursor:"pointer", display:"flex", alignItems:"center", justifyContent:"center",
        color: active ? "#fff" : "var(--ink)",
      }}>
      {icon}
    </button>
  );
}

const ICON = {
  play:    <svg width="11" height="11" viewBox="0 0 14 14"><polygon points="3,2 12,7 3,12" fill="currentColor"/></svg>,
  pause:   <svg width="11" height="11" viewBox="0 0 14 14"><rect x="3" y="2" width="3" height="10" fill="currentColor"/><rect x="8" y="2" width="3" height="10" fill="currentColor"/></svg>,
  stop:    <svg width="11" height="11" viewBox="0 0 14 14"><rect x="3" y="3" width="8" height="8" fill="currentColor"/></svg>,
  rec:     <svg width="11" height="11" viewBox="0 0 14 14"><circle cx="7" cy="7" r="4.5" fill="currentColor"/></svg>,
  rew:     <svg width="11" height="11" viewBox="0 0 14 14"><polygon points="11,2 5,7 11,12" fill="currentColor"/><rect x="3" y="2" width="2" height="10" fill="currentColor"/></svg>,
  ff:      <svg width="11" height="11" viewBox="0 0 14 14"><polygon points="3,2 9,7 3,12" fill="currentColor"/><rect x="9" y="2" width="2" height="10" fill="currentColor"/></svg>,
  loop:    <svg width="11" height="11" viewBox="0 0 14 14" fill="none" stroke="currentColor" strokeWidth="1.4"><path d="M2 7 a5 5 0 1 1 5 5"/><polyline points="5,11 7,12 8,10" fill="currentColor" stroke="none"/></svg>,
};

function MasterMeter({ playing=false, level=0.7 }) {
  const [bars, setBars] = useStateT([0.6,0.7]);
  useEffectT(() => {
    if (!playing) { setBars([0.05,0.05]); return; }
    const id = setInterval(() => {
      setBars([
        Math.min(1, level * (0.5 + Math.random() * 0.7)),
        Math.min(1, level * (0.5 + Math.random() * 0.7)),
      ]);
    }, 90);
    return () => clearInterval(id);
  }, [playing, level]);

  const SEGMENTS = 16;
  const renderBar = (v) => (
    <div style={{ display:"flex", flexDirection:"column-reverse", gap:1, height:38 }}>
      {Array.from({length: SEGMENTS}).map((_,i) => {
        const t = i / (SEGMENTS-1);
        const active = t < v;
        const c = t < 0.7 ? "var(--led-green)" : t < 0.88 ? "var(--led-amber)" : "var(--led-red)";
        return (
          <div key={i} style={{
            width:7, height:1.4,
            background: active ? c : "rgba(0,0,0,0.35)",
            boxShadow: active ? `0 0 3px ${c}` : "none",
            borderRadius:0.5,
          }} />
        );
      })}
    </div>
  );

  return (
    <div style={{
      display:"flex", gap:5, padding:"5px 7px",
      background:"var(--display-bg)", borderRadius:4,
      boxShadow:"inset 0 2px 4px rgba(0,0,0,0.8)",
      border:"1px solid rgba(0,0,0,0.6)",
    }}>
      {renderBar(bars[0])}
      {renderBar(bars[1])}
    </div>
  );
}

/* Row 2 button — flat hardware tile */
function UtilBtn({ label, sub, active=false, onClick, color="var(--accent)", flex=null }) {
  return (
    <button onClick={onClick} style={{
      flex,
      padding:"5px 10px",
      borderRadius:4, cursor:"pointer",
      background: active
        ? `linear-gradient(180deg, ${color}, ${color}cc)`
        : "linear-gradient(180deg, #ede5d1, #d8cdb1)",
      color: active ? "#fff" : "var(--ink)",
      border:"1px solid " + (active ? color : "rgba(0,0,0,0.25)"),
      boxShadow: active
        ? `0 0 6px ${color}88, inset 0 1px 0 rgba(255,255,255,0.3)`
        : "inset 0 1px 0 rgba(255,255,255,0.5), 0 1px 0 rgba(0,0,0,0.15)",
      fontFamily:'"JetBrains Mono", monospace',
      fontSize:9, fontWeight:700, letterSpacing:"0.12em",
      textTransform:"uppercase",
      display:"flex", flexDirection:"column", alignItems:"center", gap:1,
      minWidth: 0,
    }}>
      <span>{label}</span>
      {sub && <span style={{ fontSize:7, fontWeight:600, opacity:0.7, letterSpacing:"0.08em" }}>{sub}</span>}
    </button>
  );
}

function PatternCombo({ patterns, active, onSelect }) {
  return (
    <div style={{
      display:"flex", alignItems:"center", gap:0,
      background:"var(--display-bg)",
      borderRadius:4,
      boxShadow:"inset 0 2px 4px rgba(0,0,0,0.7)",
      border:"1px solid rgba(0,0,0,0.5)",
      padding:"3px 4px",
      minWidth:200,
    }}>
      <span style={{
        fontFamily:'"VT323", monospace', fontSize:13,
        color:"var(--display-fg)",
        textShadow:"0 0 3px rgba(185,255,102,0.5)",
        letterSpacing:"0.08em",
        padding:"0 4px",
        opacity:0.7,
      }}>▦</span>
      <select value={active} onChange={e => onSelect && onSelect(e.target.value)}
        style={{
          flex:1, background:"transparent", border:"none", outline:"none",
          fontFamily:'"VT323", monospace', fontSize:14,
          color:"var(--display-fg)", letterSpacing:"0.05em",
          textShadow:"0 0 3px rgba(185,255,102,0.5)",
          cursor:"pointer", appearance:"none",
          padding:"0 4px",
        }}>
        {patterns.map(p => <option key={p} value={p} style={{ background:"#0d1410" }}>{p}</option>)}
      </select>
      <span style={{
        fontFamily:'"VT323", monospace', fontSize:14,
        color:"var(--display-fg)",
        textShadow:"0 0 3px rgba(185,255,102,0.5)",
        opacity:0.6, padding:"0 4px",
      }}>▾</span>
    </div>
  );
}

function Transport({
  playing, recording, looping, bpm, position, playMode,
  onPlay, onStop, onRec, onLoop, onScrub, onPlayMode,
  patterns, currentPattern, onPattern,
  onNewProject, onOpen, onSave, onSaveAs, onExport,
  mixerOpen, onToggleMixer,
  midiOn, audioOn, padOn, touchOn, liveOn,
  onMidi, onAudio, onPad, onTouch, onLive,
  onPatternNew, onPatternDup, onPatternDel, onPatternRen,
}) {
  return (
    <div style={{
      display:"flex", flexDirection:"column", gap:8,
      padding:"12px 14px",
      background: "linear-gradient(180deg, var(--chassis) 0%, var(--chassis-2) 100%)",
      borderRadius:12,
      border:"1px solid var(--panel-rim)",
      boxShadow:"inset 0 1px 0 rgba(255,255,255,0.5), 0 1px 0 rgba(255,255,255,0.4)",
      position:"relative",
    }}>
      {/* ROW 1: brand · reels · time/transport/bpm · play-mode · meters · master */}
      <div style={{ display:"grid", gridTemplateColumns:"auto auto 1fr auto auto", gap:14, alignItems:"center" }}>
        {/* brand */}
        <div style={{ display:"flex", flexDirection:"column", gap:2, paddingRight:12, borderRight:"1px dashed rgba(0,0,0,0.15)" }}>
          <div style={{ fontFamily:'"Space Grotesk", sans-serif', fontSize:20, fontWeight:700, letterSpacing:"-0.02em", color:"var(--ink)", lineHeight:1 }}>
            xitges<span style={{ color:"var(--accent)" }}>.</span>
          </div>
          <div style={{ fontSize:9, letterSpacing:"0.2em", color:"var(--ink-faint)", fontWeight:600, textTransform:"uppercase" }}>
            STUDIO DAW
          </div>
        </div>

        {/* tape reels */}
        <div style={{ display:"flex", alignItems:"center", gap:10, paddingRight:12, borderRight:"1px dashed rgba(0,0,0,0.15)" }}>
          <TapeReel spinning={playing} side="L" />
          <div style={{ width:30, height:3, borderRadius:1,
            background:"linear-gradient(90deg, #2a1f15, #4a3a25, #2a1f15)",
            boxShadow:"0 1px 1px rgba(0,0,0,0.4)" }} />
          <TapeReel spinning={playing} side="R" />
        </div>

        {/* center: time + transport + bpm */}
        <div style={{ display:"flex", alignItems:"center", gap:12, justifyContent:"center" }}>
          <div style={{ display:"flex", flexDirection:"column", gap:2 }}>
            <SegmentDisplay height={36}>
              {position.bar.toString().padStart(3,"0")}.{position.beat.toString().padStart(2,"0")}.{position.tick.toString().padStart(3,"0")}
            </SegmentDisplay>
            <div style={{ display:"flex", gap:6, justifyContent:"space-between", padding:"0 4px" }}>
              <span style={{ fontSize:7, fontWeight:600, letterSpacing:"0.15em", color:"var(--ink-faint)" }}>BAR</span>
              <span style={{ fontSize:7, fontWeight:600, letterSpacing:"0.15em", color:"var(--ink-faint)" }}>BEAT</span>
              <span style={{ fontSize:7, fontWeight:600, letterSpacing:"0.15em", color:"var(--ink-faint)" }}>TICK</span>
            </div>
          </div>

          <div style={{ display:"flex", alignItems:"center", gap:5 }}>
            <TransportBtn icon={ICON.rew} onClick={() => onScrub && onScrub(-1)} title="Rewind" size={28} />
            <TransportBtn icon={playing ? ICON.pause : ICON.play} onClick={onPlay} active={playing} color="var(--led-green)" size={38} title="Play / Pause" />
            <TransportBtn icon={ICON.stop} onClick={onStop} title="Stop" size={28} />
            <TransportBtn icon={ICON.rec} onClick={onRec} active={recording} color="var(--led-red)" title="Record" size={28} />
            <TransportBtn icon={ICON.ff} onClick={() => onScrub && onScrub(1)} title="Fast forward" size={28} />
            <TransportBtn icon={ICON.loop} onClick={onLoop} active={looping} color="var(--led-amber)" title="Loop" size={28} />
          </div>

          <div style={{ display:"flex", flexDirection:"column", alignItems:"center", gap:2, minWidth:72 }}>
            <SegmentDisplay height={26} padding="2px 10px">
              {bpm.toFixed(1)}
            </SegmentDisplay>
            <div style={{ fontSize:7, fontWeight:600, letterSpacing:"0.15em", color:"var(--ink-faint)" }}>BPM 60·200</div>
          </div>

          {/* pattern / song mode toggle */}
          <div style={{
            display:"flex", padding:2, borderRadius:4,
            background:"rgba(0,0,0,0.08)",
            boxShadow:"inset 0 1px 2px rgba(0,0,0,0.2)",
          }}>
            {["PAT","SONG"].map(m => (
              <button key={m} onClick={() => onPlayMode && onPlayMode(m)} style={{
                padding:"4px 10px", borderRadius:3, cursor:"pointer",
                fontSize:9, fontWeight:700, letterSpacing:"0.12em",
                fontFamily:'"JetBrains Mono", monospace',
                background: playMode === m ? "var(--accent)" : "transparent",
                color: playMode === m ? "#fff" : "var(--ink-soft)",
                border:"none",
                boxShadow: playMode === m ? "0 0 4px var(--accent)" : "none",
              }}>{m}</button>
            ))}
          </div>
        </div>

        {/* meters */}
        <div style={{ display:"flex", flexDirection:"column", alignItems:"center", gap:3 }}>
          <MasterMeter playing={playing} level={0.78} />
          <div style={{ fontSize:7, fontWeight:600, letterSpacing:"0.18em", color:"var(--ink-faint)" }}>L · R</div>
        </div>

        {/* master + status */}
        <div style={{ display:"flex", alignItems:"center", gap:10, paddingLeft:8, borderLeft:"1px dashed rgba(0,0,0,0.15)" }}>
          <div style={{ display:"flex", flexDirection:"column", gap:3, alignItems:"flex-end" }}>
            <div style={{ display:"flex", alignItems:"center", gap:5 }}>
              <LED on={playing} color="green" size={6} />
              <span style={{ fontSize:8, letterSpacing:"0.12em", color:"var(--ink-soft)", fontWeight:600 }}>PLAY</span>
              <LED on={recording} color="red" size={6} blink={recording} />
              <span style={{ fontSize:8, letterSpacing:"0.12em", color:"var(--ink-soft)", fontWeight:600 }}>REC</span>
            </div>
            <div style={{ fontSize:8, letterSpacing:"0.1em", color:"var(--ink-faint)" }}>
              44.1k · 24b · BUF 128 · CPU 21%
            </div>
          </div>
          <Knob defaultValue={0.85} label="MASTER" accent size={42} />
        </div>
      </div>

      {/* ROW 2: pattern mgmt · file ops · utility toggles */}
      <div style={{
        display:"grid",
        gridTemplateColumns:"auto auto 1fr auto auto",
        gap:10, alignItems:"center",
        padding:"6px 0 0",
        borderTop:"1px dashed rgba(0,0,0,0.15)",
      }}>
        {/* pattern combo + ops */}
        <div style={{ display:"flex", alignItems:"center", gap:6 }}>
          <PatternCombo patterns={patterns} active={currentPattern} onSelect={onPattern} />
          <button onClick={onPatternNew} title="New pattern" style={iconBtnStyle()}>＋</button>
          <button onClick={onPatternDup} title="Duplicate pattern" style={iconBtnStyle()}>⎘</button>
          <button onClick={onPatternRen} title="Rename pattern" style={iconBtnStyle()}>✎</button>
          <button onClick={onPatternDel} title="Delete pattern" style={iconBtnStyle()}>×</button>
        </div>

        {/* file ops */}
        <div style={{ display:"flex", alignItems:"center", gap:4, paddingLeft:10, borderLeft:"1px dashed rgba(0,0,0,0.15)" }}>
          <UtilBtn label="NEW" onClick={onNewProject} />
          <UtilBtn label="OPEN" onClick={onOpen} />
          <UtilBtn label="SAVE" onClick={onSave} />
          <UtilBtn label="SAVE AS" onClick={onSaveAs} />
          <UtilBtn label="EXPORT" sub="WAV" onClick={onExport} color="var(--led-amber)" />
        </div>

        {/* spacer */}
        <div />

        {/* mixer toggle */}
        <UtilBtn label="MIXER" active={mixerOpen} onClick={onToggleMixer} color="var(--accent)" />

        {/* device toggles */}
        <div style={{ display:"flex", alignItems:"center", gap:4, paddingLeft:10, borderLeft:"1px dashed rgba(0,0,0,0.15)" }}>
          <UtilBtn label="MIDI" active={midiOn} onClick={onMidi} color="var(--led-green)" />
          <UtilBtn label="AUDIO" active={audioOn} onClick={onAudio} color="var(--led-green)" />
          <UtilBtn label="PAD" active={padOn} onClick={onPad} color="var(--led-amber)" />
          <UtilBtn label="TOUCH" active={touchOn} onClick={onTouch} color="var(--led-amber)" />
          <UtilBtn label="LIVE" active={liveOn} onClick={onLive} color="var(--led-red)" />
        </div>
      </div>
    </div>
  );
}

function iconBtnStyle() {
  return {
    width:24, height:24, padding:0, borderRadius:3, cursor:"pointer",
    background:"linear-gradient(180deg, #ede5d1, #d8cdb1)",
    color:"var(--ink)",
    border:"1px solid rgba(0,0,0,0.25)",
    boxShadow:"inset 0 1px 0 rgba(255,255,255,0.5), 0 1px 0 rgba(0,0,0,0.15)",
    fontFamily:'"JetBrains Mono", monospace',
    fontSize:13, fontWeight:700,
    display:"flex", alignItems:"center", justifyContent:"center",
  };
}

Object.assign(window, { Transport, TapeReel });
