/* TIMELINE — Playlist (song mode)
 * Dynamic tracks, 200 bars, snap grid (1bar/½/¼/Free), pattern clips show mini step-grid dots
 */

const { useMemo: useMemoTL } = React;

function StepDots({ pattern, color, w, h=32 }) {
  // Mini step-grid representation (8x4 dots) replacing waveform
  const cols = 16, rows = 4;
  return (
    <svg width="100%" height={h} viewBox={`0 0 ${cols*4} ${rows*4}`} preserveAspectRatio="none" style={{ display:"block" }}>
      {Array.from({length: rows}).map((_,r) => (
        Array.from({length: cols}).map((_,c) => {
          const on = pattern[(r * cols + c) % pattern.length];
          return (
            <rect key={`${r}-${c}`}
              x={c*4 + 0.6} y={r*4 + 0.6}
              width={2.8} height={2.8}
              rx={0.6}
              fill={on ? color : "rgba(0,0,0,0.2)"}
              opacity={on ? 0.95 : 0.5}
            />
          );
        })
      ))}
    </svg>
  );
}

function patternFor(seed, density=0.35) {
  let s = seed;
  const r = () => { s = (s * 9301 + 49297) % 233280; return s / 233280; };
  return Array.from({length:64}, () => r() < density ? 1 : 0);
}

function PatternClip({ x, w, color, label, patternId, seed, muted }) {
  const dots = useMemoTL(() => patternFor(seed), [seed]);
  return (
    <div style={{
      position:"absolute", left:x, top:4, width:w, bottom:4,
      borderRadius:3, overflow:"hidden",
      background: `linear-gradient(180deg, ${color}38, ${color}18)`,
      border:`1px solid ${color}`,
      boxShadow:"0 1px 0 rgba(255,255,255,0.4), inset 0 1px 0 rgba(255,255,255,0.15)",
      opacity: muted ? 0.4 : 1, cursor:"grab",
    }}>
      <div style={{
        position:"absolute", top:0, left:0, right:0, padding:"2px 6px",
        fontSize:9, fontWeight:600, letterSpacing:"0.06em", color:"var(--ink)",
        background: `linear-gradient(180deg, ${color}66, ${color}33)`,
        borderBottom: `1px solid ${color}`,
        display:"flex", justifyContent:"space-between", alignItems:"center",
      }}>
        <span style={{ overflow:"hidden", textOverflow:"ellipsis", whiteSpace:"nowrap" }}>
          <span style={{ fontFamily:'"JetBrains Mono",monospace', fontWeight:700, marginRight:4 }}>▦</span>
          {patternId} · {label}
        </span>
        {muted && <span style={{ color:"var(--ink-faint)" }}>M</span>}
      </div>
      <div style={{ position:"absolute", top:18, left:4, right:4, bottom:4, display:"flex", alignItems:"center" }}>
        <StepDots pattern={dots} color={color} w={w} h={32} />
      </div>
    </div>
  );
}

function AudioClip({ x, w, color, label, seed, muted }) {
  // simple horizontal stripes for audio clips — distinct from pattern dot grids
  return (
    <div style={{
      position:"absolute", left:x, top:4, width:w, bottom:4,
      borderRadius:3, overflow:"hidden",
      background: `repeating-linear-gradient(135deg, ${color}33 0 4px, ${color}55 4px 8px)`,
      border:`1px solid ${color}`,
      boxShadow:"0 1px 0 rgba(255,255,255,0.4)",
      opacity: muted ? 0.4 : 1,
    }}>
      <div style={{
        position:"absolute", top:0, left:0, right:0, padding:"2px 6px",
        fontSize:9, fontWeight:600, letterSpacing:"0.06em", color:"var(--ink)",
        background: `linear-gradient(180deg, ${color}77, ${color}44)`,
        borderBottom: `1px solid ${color}`,
      }}>
        <span style={{ fontFamily:'"JetBrains Mono",monospace', fontWeight:700, marginRight:4 }}>♪</span>
        {label}
      </div>
    </div>
  );
}

function TrackHead({ idx, name, color, muted, soloed, armed }) {
  return (
    <div style={{
      height:54, padding:"5px 8px",
      background: "linear-gradient(180deg, var(--panel) 0%, #ebe3cf 100%)",
      borderBottom:"1px solid var(--rule)",
      display:"grid", gridTemplateColumns:"auto 1fr auto", gap:6, alignItems:"center",
    }}>
      <div style={{
        width:18, height:18, borderRadius:3, background: color,
        boxShadow:`0 0 4px ${color}66, inset 0 1px 0 rgba(255,255,255,0.4)`,
        border:"1px solid rgba(0,0,0,0.4)",
        display:"flex", alignItems:"center", justifyContent:"center",
        fontSize:9, fontWeight:700, color:"#fff",
      }}>{idx}</div>
      <div style={{ minWidth:0 }}>
        <div style={{ fontSize:10, fontWeight:600, color:"var(--ink)", letterSpacing:"0.02em", textTransform:"uppercase", overflow:"hidden", textOverflow:"ellipsis", whiteSpace:"nowrap" }}>
          {name}
        </div>
        <div style={{ display:"flex", gap:3, marginTop:3 }}>
          {[
            { k:"M", on:muted, c:"var(--led-amber)" },
            { k:"S", on:soloed, c:"var(--led-green)" },
            { k:"R", on:armed, c:"var(--led-red)" },
          ].map(b => (
            <button key={b.k} style={{
              width:16, height:13, padding:0, borderRadius:2,
              fontSize:7, fontWeight:700, fontFamily:'"JetBrains Mono", monospace',
              background: b.on ? b.c : "rgba(0,0,0,0.05)",
              color: b.on ? "rgba(0,0,0,0.85)" : "var(--ink-soft)",
              border:"1px solid rgba(0,0,0,0.2)", cursor:"pointer",
              boxShadow: b.on ? `0 0 4px ${b.c}` : "none",
            }}>{b.k}</button>
          ))}
        </div>
      </div>
      <Knob defaultValue={0.7} size={22} marks={false} />
    </div>
  );
}

function Ruler({ bars, pxPerBar, playhead }) {
  return (
    <div style={{
      height:22, background:"var(--display-bg)", position:"relative",
      borderBottom:"1px solid rgba(0,0,0,0.6)",
      boxShadow:"inset 0 -2px 4px rgba(0,0,0,0.5)", overflow:"hidden",
    }}>
      {Array.from({length: bars+1}).map((_,i) => {
        const isMajor = i % 4 === 0;
        return (
          <div key={i} style={{
            position:"absolute", left: i * pxPerBar, top:0, bottom:0,
            width:1, background: isMajor ? "rgba(185,255,102,0.5)" : "rgba(185,255,102,0.15)",
          }}>
            {isMajor && (
              <span style={{
                position:"absolute", left:4, top:3,
                fontFamily:'"VT323", monospace', fontSize:13, color:"var(--display-fg)",
                letterSpacing:"0.05em", textShadow:"0 0 4px rgba(185,255,102,0.6)",
              }}>{(i+1).toString().padStart(3,"0")}</span>
            )}
          </div>
        );
      })}
      <div style={{
        position:"absolute", left: playhead, top:0, bottom:0, width:2,
        background:"var(--accent)", boxShadow:"0 0 6px var(--accent)",
      }}>
        <div style={{
          position:"absolute", left:-5, top:0, width:12, height:8,
          background:"var(--accent)",
          clipPath:"polygon(0 0, 100% 0, 50% 100%)",
        }} />
      </div>
    </div>
  );
}

function Timeline({ playing, playhead, onScrub }) {
  const PX_PER_BAR = 38;
  const BARS = 200;

  // dynamic track list — could be empty/added by user. realistic mid-project state.
  const tracks = [
    { idx:1, name:"Drums",       color:"#d8412a", clips:[
      { type:"pattern", bar:0,  len:8, patternId:"PAT 01", label:"verse",   seed:11 },
      { type:"pattern", bar:8,  len:8, patternId:"PAT 01", label:"verse",   seed:11 },
      { type:"pattern", bar:16, len:8, patternId:"PAT 02", label:"chorus",  seed:13 },
      { type:"pattern", bar:24, len:8, patternId:"PAT 02", label:"chorus",  seed:13 },
    ]},
    { idx:2, name:"Bass",        color:"#e89c2b", clips:[
      { type:"pattern", bar:4,  len:12, patternId:"PAT 01", label:"verse",  seed:23 },
      { type:"pattern", bar:18, len:10, patternId:"PAT 02", label:"chorus", seed:29 },
    ]},
    { idx:3, name:"Poly-6",      color:"#7ab87a", clips:[
      { type:"pattern", bar:8,  len:16, patternId:"PAT 03", label:"chord stack", seed:31 },
    ]},
    { idx:4, name:"Field Rec",   color:"#5fa8d8", clips:[
      { type:"audio",   bar:0,  len:32, label:"birds_dawn.wav", muted:true },
    ]},
    { idx:5, name:"Vox",         color:"#b87ad6", armed:true, clips:[
      { type:"audio",   bar:12, len:6,  label:"vox_take_03.wav" },
      { type:"audio",   bar:20, len:8,  label:"vox_take_04.wav" },
    ]},
    { idx:6, name:"FX Bus",      color:"#8d7a5a", soloed:true, clips:[
      { type:"pattern", bar:0,  len:4, patternId:"PAT 04", label:"riser", seed:47 },
      { type:"pattern", bar:16, len:4, patternId:"PAT 04", label:"riser", seed:47 },
    ]},
  ];

  const [snap, setSnap] = React.useState("1bar");

  return (
    <div style={{
      display:"flex", flexDirection:"column", flex:1, minHeight:0,
      overflow:"hidden",
    }}>
      {/* header strip */}
      <div style={{
        padding:"7px 12px",
        background:"linear-gradient(180deg, var(--chassis), var(--chassis-2))",
        borderBottom:"1px solid var(--panel-rim)",
        display:"flex", alignItems:"center", justifyContent:"space-between",
      }}>
        <div style={{ display:"flex", alignItems:"center", gap:10 }}>
          <div style={{ width:8, height:8, borderRadius:2, background:"var(--accent)" }} />
          <div style={{ fontSize:10, fontWeight:700, letterSpacing:"0.18em", color:"var(--ink)", textTransform:"uppercase" }}>
            PLAYLIST
          </div>
          <Tag>200 BARS · 4/4</Tag>
          <Tag>{tracks.length} TRACKS</Tag>
        </div>
        <div style={{ display:"flex", alignItems:"center", gap:6 }}>
          <span style={{ fontSize:8, fontWeight:700, letterSpacing:"0.14em", color:"var(--ink-faint)" }}>SNAP</span>
          <div style={{ display:"flex", padding:2, borderRadius:3, background:"rgba(0,0,0,0.06)" }}>
            {["1bar","½","¼","FREE"].map(s => (
              <button key={s} onClick={() => setSnap(s)} style={{
                padding:"2px 6px", borderRadius:2, cursor:"pointer",
                fontSize:8, fontWeight:700, letterSpacing:"0.1em",
                fontFamily:'"JetBrains Mono", monospace',
                background: snap === s ? "var(--accent)" : "transparent",
                color: snap === s ? "#fff" : "var(--ink-soft)",
                border:"none",
                boxShadow: snap === s ? "0 0 3px var(--accent)" : "none",
              }}>{s}</button>
            ))}
          </div>
          <Tag>UNDO ⌘Z</Tag>
        </div>
      </div>

      <div style={{ display:"grid", gridTemplateColumns:"160px 1fr", flex:1, minHeight:0, overflow:"hidden" }}>
        <div style={{ borderRight:"1px solid var(--panel-rim)", overflow:"hidden", display:"flex", flexDirection:"column" }}>
          <div style={{ height:22, background:"var(--display-bg)", borderBottom:"1px solid rgba(0,0,0,0.6)", display:"flex", alignItems:"center", justifyContent:"space-between", padding:"0 8px" }}>
            <span style={{ fontFamily:'"VT323", monospace', fontSize:12, color:"var(--display-fg)", letterSpacing:"0.08em" }}>
              TRK / {tracks.length}
            </span>
            <span title="Add track" style={{ fontFamily:'"VT323",monospace', fontSize:14, color:"var(--display-fg)", cursor:"pointer", textShadow:"0 0 4px rgba(185,255,102,0.5)" }}>＋</span>
          </div>
          {tracks.map(t => <TrackHead key={t.idx} {...t} />)}
        </div>

        <div className="scroll" style={{ overflow:"auto", position:"relative" }}
          onClick={(e) => {
            const rect = e.currentTarget.getBoundingClientRect();
            const x = e.clientX - rect.left + e.currentTarget.scrollLeft;
            onScrub && onScrub(x / PX_PER_BAR);
          }}
        >
          <div style={{ width: BARS * PX_PER_BAR, position:"relative" }}>
            <Ruler bars={BARS} pxPerBar={PX_PER_BAR} playhead={playhead * PX_PER_BAR} />
            {tracks.map(t => (
              <div key={t.idx} style={{
                height:54, position:"relative",
                borderBottom:"1px solid var(--rule)",
                background: t.idx % 2 === 0
                  ? "linear-gradient(180deg, #ede5d1, #e6dec9)"
                  : "linear-gradient(180deg, #f3ecda, #ede5d1)",
              }}>
                {Array.from({length: BARS+1}).map((_,i) => (
                  <div key={i} style={{
                    position:"absolute", left:i*PX_PER_BAR, top:0, bottom:0, width:1,
                    background: i % 4 === 0 ? "rgba(0,0,0,0.12)" : "rgba(0,0,0,0.05)",
                  }} />
                ))}
                {t.clips.map((c, i) => (
                  c.type === "audio" ? (
                    <AudioClip key={i} x={c.bar*PX_PER_BAR + 2} w={c.len*PX_PER_BAR - 4}
                      color={t.color} label={c.label} muted={c.muted} seed={c.bar*7} />
                  ) : (
                    <PatternClip key={i} x={c.bar*PX_PER_BAR + 2} w={c.len*PX_PER_BAR - 4}
                      color={t.color} label={c.label} patternId={c.patternId} seed={c.seed} muted={c.muted} />
                  )
                ))}
              </div>
            ))}
            <div style={{
              position:"absolute", left: playhead * PX_PER_BAR, top:22, bottom:0, width:2,
              background:"var(--accent)", boxShadow:"0 0 6px var(--accent)",
              pointerEvents:"none", opacity: 0.85,
            }} />
          </div>
        </div>
      </div>
    </div>
  );
}

Object.assign(window, { Timeline });
