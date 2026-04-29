/* MIXER — 8 insert channels + 1 master strip
 * Per channel: color tab · name · pan · FX chain row (CMP/DLY/RVB) · M/S · fader · meter · dB readout
 * Per channel: Dynamic EQ button (opens 6-band popup), FX panel button
 * Master: Adaptive trim · glue comp · soft saturator · stereo meter
 */

const { useState: useStateMX } = React;

function FxSlot({ k, on, label }) {
  return (
    <button title={label} style={{
      height:13, padding:"0 4px", borderRadius:2,
      fontSize:7, fontWeight:700, fontFamily:'"JetBrains Mono", monospace',
      letterSpacing:"0.1em",
      background: on ? "var(--led-green)" : "rgba(0,0,0,0.06)",
      color: on ? "rgba(0,0,0,0.85)" : "var(--ink-faint)",
      border:"1px solid rgba(0,0,0,0.2)",
      cursor:"pointer",
      boxShadow: on ? "0 0 3px var(--led-green)" : "none",
      display:"flex", alignItems:"center", justifyContent:"center", width:"100%",
    }}>{k}</button>
  );
}

function ChannelStrip({ idx, name, color, defaultVol=0.7, defaultPan=0.5, eq=[0.5,0.55,0.5,0.5,0.5,0.5], muted=false, soloed=false, sends=[0.3,0.1], sources=[], fxOn={cmp:false,dly:false,rvb:false}, eqOn=false }) {
  const [vol, setVol] = useStateMX(defaultVol);
  const [m, setM] = useStateMX(muted);
  const [s, setS] = useStateMX(soloed);

  const dbStr = m ? "-INF" : (vol < 0.01 ? "-60.0" : (20 * Math.log10(vol)).toFixed(1));

  return (
    <div style={{
      width:64, padding:"6px 5px",
      background: "linear-gradient(180deg, #ebe3cf 0%, #e0d6bd 100%)",
      borderRadius:5,
      border:"1px solid var(--panel-rim)",
      boxShadow:"inset 0 1px 0 rgba(255,255,255,0.5), 0 1px 0 rgba(255,255,255,0.4)",
      display:"flex", flexDirection:"column", alignItems:"center", gap:5,
    }}>
      {/* color tab — 5px */}
      <div style={{
        width:"100%", height:5, borderRadius:1,
        background: color,
        boxShadow:`0 0 4px ${color}66`,
      }} />

      {/* idx + name */}
      <div style={{ width:"100%", display:"flex", flexDirection:"column", alignItems:"center", gap:1 }}>
        <div style={{
          fontFamily:'"JetBrains Mono", monospace',
          fontSize:7, fontWeight:700, letterSpacing:"0.1em",
          color:"var(--ink-faint)",
        }}>MX{idx.toString().padStart(2,"0")}</div>
        <div style={{
          width:"100%", textAlign:"center", fontSize:9, fontWeight:700, letterSpacing:"0.04em",
          color:"var(--ink)", textTransform:"uppercase",
          overflow:"hidden", textOverflow:"ellipsis", whiteSpace:"nowrap",
        }}>{name}</div>
      </div>

      {/* sources tag (channel rack channels routed here) */}
      <div style={{
        width:"100%", padding:"1px 3px",
        textAlign:"center", borderRadius:2,
        background:"rgba(0,0,0,0.06)",
        fontFamily:'"JetBrains Mono", monospace',
        fontSize:7, fontWeight:600, letterSpacing:"0.06em", color:"var(--ink-soft)",
        overflow:"hidden", textOverflow:"ellipsis", whiteSpace:"nowrap",
      }}>← {sources.length === 0 ? "—" : sources.join(",")}</div>

      {/* PAN */}
      <Knob defaultValue={defaultPan} size={26} marks={false} min={0} max={1} bipolar />

      {/* FX chain row: CMP DLY RVB */}
      <div style={{ width:"100%", display:"grid", gridTemplateColumns:"1fr 1fr 1fr", gap:2 }}>
        <FxSlot k="CMP" on={fxOn.cmp} label="Compressor" />
        <FxSlot k="DLY" on={fxOn.dly} label="Delay" />
        <FxSlot k="RVB" on={fxOn.rvb} label="Reverb" />
      </div>

      {/* DYN EQ button */}
      <button title="Dynamic EQ (6-band)" style={{
        width:"100%", height:14, padding:0, borderRadius:2,
        fontSize:7, fontWeight:700, fontFamily:'"JetBrains Mono", monospace',
        letterSpacing:"0.12em",
        background: eqOn ? "var(--accent)" : "rgba(0,0,0,0.06)",
        color: eqOn ? "#fff" : "var(--ink-faint)",
        border:"1px solid " + (eqOn ? "var(--accent)" : "rgba(0,0,0,0.2)"),
        cursor:"pointer",
        boxShadow: eqOn ? "0 0 4px var(--accent)" : "none",
      }}>DYN EQ {eqOn ? "●" : "○"}</button>

      {/* SENDS */}
      <div style={{ width:"100%", display:"flex", justifyContent:"space-around", gap:2 }}>
        {sends.map((v,i) => (
          <div key={i} style={{ display:"flex", flexDirection:"column", alignItems:"center", gap:1 }}>
            <Knob defaultValue={v} size={16} marks={false} accent />
            <span style={{ fontSize:6, fontWeight:700, color:"var(--accent)", letterSpacing:"0.08em" }}>{["RV","DL"][i]}</span>
          </div>
        ))}
      </div>

      {/* M/S buttons */}
      <div style={{ display:"flex", gap:3 }}>
        <button onClick={() => setM(!m)} style={msButton(m, "var(--led-amber)")}>M</button>
        <button onClick={() => setS(!s)} style={msButton(s, "var(--led-green)")}>S</button>
      </div>

      {/* Fader + meter */}
      <div style={{ display:"flex", gap:3, alignItems:"flex-end" }}>
        <Fader value={vol} onChange={setVol} height={92} />
        <ChannelMeter level={m ? 0 : vol * (0.55 + (Math.abs(Math.sin(name.charCodeAt(0))) * 0.35))} height={92} />
      </div>

      {/* dB readout — VT323 + displayBg */}
      <div style={{
        width:"100%", padding:"2px 4px", textAlign:"center",
        background:"var(--display-bg)",
        borderRadius:2,
        boxShadow:"inset 0 1px 2px rgba(0,0,0,0.7)",
        border:"1px solid rgba(0,0,0,0.5)",
        fontFamily:'"VT323", monospace', fontSize:11,
        color:"var(--display-fg)", letterSpacing:"0.05em",
        textShadow:"0 0 3px rgba(185,255,102,0.6)",
      }}>{dbStr}</div>
    </div>
  );
}

function msButton(on, c) {
  return {
    width:18, height:14, padding:0, borderRadius:2,
    fontSize:8, fontWeight:700, fontFamily:'"JetBrains Mono", monospace',
    background: on ? c : "rgba(0,0,0,0.06)",
    color: on ? "rgba(0,0,0,0.85)" : "var(--ink-soft)",
    border:"1px solid rgba(0,0,0,0.2)",
    cursor:"pointer",
    boxShadow: on ? `0 0 4px ${c}` : "none",
  };
}

function ChannelMeter({ level=0.5, height=100 }) {
  const SEG = 22;
  return (
    <div style={{
      display:"flex", flexDirection:"column-reverse", gap:1,
      padding:"3px 2px", borderRadius:2, height,
      background:"var(--display-bg)",
      boxShadow:"inset 0 1px 3px rgba(0,0,0,0.7)",
      border:"1px solid rgba(0,0,0,0.5)",
    }}>
      {Array.from({length:SEG}).map((_,i) => {
        const t = i / (SEG-1);
        const active = t < level;
        const c = t < 0.7 ? "var(--led-green)" : t < 0.88 ? "var(--led-amber)" : "var(--led-red)";
        return (
          <div key={i} style={{
            width:4, height: (height-8) / SEG - 1,
            background: active ? c : "rgba(0,0,0,0.4)",
            boxShadow: active ? `0 0 2px ${c}` : "none",
            borderRadius:0.5,
          }} />
        );
      })}
    </div>
  );
}

function MasterStrip({ vol=0.85 }) {
  const [v, setV] = useStateMX(vol);
  const dbStr = v < 0.01 ? "-60.0" : (20 * Math.log10(v)).toFixed(1);
  return (
    <div style={{
      width:120, padding:"6px 7px",
      background: "linear-gradient(180deg, #f3ecda 0%, #ddd3ba 100%)",
      borderRadius:5,
      border:"1px solid var(--accent)",
      boxShadow:"0 0 8px var(--accent)44, inset 0 1px 0 rgba(255,255,255,0.5)",
      display:"flex", flexDirection:"column", alignItems:"center", gap:6,
    }}>
      <div style={{
        width:"100%", height:5, borderRadius:1,
        background:"var(--accent)",
        boxShadow:"0 0 6px var(--accent)",
      }} />
      <div style={{ display:"flex", flexDirection:"column", alignItems:"center", gap:0 }}>
        <div style={{ fontFamily:'"JetBrains Mono", monospace', fontSize:7, fontWeight:700, letterSpacing:"0.14em", color:"var(--accent)" }}>MASTER</div>
        <div style={{ fontSize:9, fontWeight:700, letterSpacing:"0.06em", color:"var(--ink)" }}>STEREO OUT</div>
      </div>

      {/* Adaptive trim · Glue comp · Saturator */}
      <div style={{ display:"flex", flexDirection:"column", gap:4, alignItems:"center", padding:"4px 0", borderTop:"1px dashed rgba(0,0,0,0.2)", borderBottom:"1px dashed rgba(0,0,0,0.2)", width:"100%" }}>
        <div style={{ display:"flex", gap:6, alignItems:"flex-end" }}>
          <Knob defaultValue={0.5} size={26} label="TRIM" marks={false} bipolar />
          <Knob defaultValue={0.4} size={26} label="GLUE" marks={false} accent />
          <Knob defaultValue={0.3} size={26} label="SAT" marks={false} />
        </div>
        <div style={{ display:"flex", gap:3, fontSize:7, fontWeight:700, letterSpacing:"0.1em", color:"var(--ink-faint)" }}>
          <span style={{ color:"var(--led-green)" }}>● ADAPT</span>
          <span>·</span>
          <span style={{ color:"var(--led-amber)" }}>● GLUE</span>
          <span>·</span>
          <span style={{ color:"var(--led-amber)" }}>● SOFT</span>
        </div>
      </div>

      {/* L/R fader + meter */}
      <div style={{ display:"flex", gap:5, alignItems:"flex-end" }}>
        <div style={{ display:"flex", flexDirection:"column", alignItems:"center", gap:2 }}>
          <Fader value={v} onChange={setV} height={92} />
          <span style={{ fontSize:7, fontWeight:700, letterSpacing:"0.1em", color:"var(--ink-faint)" }}>OUT</span>
        </div>
        <ChannelMeter level={v * 0.86} height={92} />
        <ChannelMeter level={v * 0.79} height={92} />
      </div>

      {/* dB */}
      <div style={{
        width:"100%", padding:"3px 4px", textAlign:"center",
        background:"var(--display-bg)",
        borderRadius:2,
        boxShadow:"inset 0 1px 2px rgba(0,0,0,0.7)",
        border:"1px solid rgba(0,0,0,0.5)",
        fontFamily:'"VT323", monospace', fontSize:14,
        color:"var(--display-fg)", letterSpacing:"0.06em",
        textShadow:"0 0 4px rgba(185,255,102,0.6)",
      }}>{dbStr} dB</div>
    </div>
  );
}

function Mixer() {
  // 8 insert channels — realistic routing from 16 channel rack channels
  const channels = [
    { idx:1, name:"DRUMS",   color:"#d8412a", sources:["CH 01","CH 02","CH 03","CH 04","CH 05"], fxOn:{cmp:true,dly:false,rvb:false}, eqOn:true,  defaultVol:0.78 },
    { idx:2, name:"PERC",    color:"#e89c2b", sources:["CH 06","CH 13","CH 14"],                  fxOn:{cmp:false,dly:false,rvb:true},  eqOn:false, defaultVol:0.55 },
    { idx:3, name:"BASS",    color:"#f0c14a", sources:["CH 07"],                                  fxOn:{cmp:true,dly:false,rvb:false}, eqOn:true,  defaultVol:0.72 },
    { idx:4, name:"POLY",    color:"#7ab87a", sources:["CH 08","CH 09"],                          fxOn:{cmp:false,dly:true,rvb:true},  eqOn:false, defaultVol:0.62 },
    { idx:5, name:"LEAD",    color:"#5fa8d8", sources:["CH 10"],                                  fxOn:{cmp:true,dly:true,rvb:true},   eqOn:true,  defaultVol:0.6 },
    { idx:6, name:"VOX",     color:"#b87ad6", sources:["CH 11"],                                  fxOn:{cmp:true,dly:true,rvb:true},   eqOn:true,  defaultVol:0.7 },
    { idx:7, name:"FX",      color:"#8d7a5a", sources:["CH 12"], soloed:true,                     fxOn:{cmp:false,dly:true,rvb:true},  eqOn:false, defaultVol:0.45 },
    { idx:8, name:"AUX",     color:"#2cd4d4", sources:["CH 15","CH 16"], muted:true,              fxOn:{cmp:false,dly:false,rvb:false},eqOn:false, defaultVol:0.4 },
  ];

  return (
    <div style={{ display:"flex", flexDirection:"column", gap:8, height:"100%" }}>
      <Panel title="MIXER" sub="8 INSERT · 1 MASTER" accent style={{ flex:1, display:"flex", flexDirection:"column" }} contentStyle={{ display:"flex", flexDirection:"column", gap:8, flex:1 }}>
        <div className="scroll" style={{ display:"flex", gap:6, alignItems:"flex-start", overflowX:"auto", paddingBottom:4 }}>
          {channels.map(c => <ChannelStrip key={c.idx} {...c} />)}
          <div style={{ width:1, alignSelf:"stretch", background:"var(--panel-rim)", margin:"0 4px" }} />
          <MasterStrip vol={0.85} />
        </div>
      </Panel>
    </div>
  );
}

Object.assign(window, { Mixer, ChannelStrip, MasterStrip });
