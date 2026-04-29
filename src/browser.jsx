/* SAMPLE BROWSER — bookmark folders (user-registered), expandable tree,
 * file ext only, hash-based color, drag-to-rack/launchpad, real-time preview, search */

const { useState: useStateB, useMemo: useMemoB } = React;

const FILE_COLORS = ["#d8412a","#e89c2b","#7ab87a","#5fa8d8","#b87ad6","#8d7a5a","#3da356","#2cd4d4"];
function hashColor(name) {
  let h = 0;
  for (let i = 0; i < name.length; i++) h = (h * 31 + name.charCodeAt(i)) >>> 0;
  return FILE_COLORS[h % FILE_COLORS.length];
}
function fileExt(name) {
  const m = name.match(/\.([a-z0-9]+)$/i);
  return m ? m[1].toLowerCase() : "";
}

function miniWave(seed, n=60) {
  let s = seed;
  const r = () => { s = (s * 9301 + 49297) % 233280; return s / 233280; };
  return Array.from({length:n}, (_, i) => {
    const env = Math.sin((i/n) * Math.PI) * 0.7 + 0.3;
    return ((r() - 0.5) * 1.6 + Math.sin(i*0.3) * 0.4) * env;
  });
}
function MiniWave({ seed, color="var(--accent)", w=80, h=18 }) {
  const data = useMemoB(() => miniWave(seed), [seed]);
  return (
    <svg width={w} height={h} viewBox={`0 0 ${data.length} ${h}`} preserveAspectRatio="none" style={{ display:"block" }}>
      {data.map((v,i) => {
        const bh = Math.abs(v) * (h/2 - 1);
        return <rect key={i} x={i} y={h/2 - bh} width={0.7} height={bh*2} fill={color} opacity={0.85} />;
      })}
    </svg>
  );
}

/* Tree node — bookmark folder or its children. Expandable. */
function TreeNode({ node, depth=0, expanded, onToggle, selected, onSelect }) {
  const isExpanded = expanded.has(node.path);
  const isSelected = selected === node.path;
  const isFolder = node.type === "folder" || node.type === "bookmark";
  const hasChildren = isFolder && node.children && node.children.length > 0;

  return (
    <>
      <div onClick={() => isFolder ? onToggle(node.path) : onSelect(node.path)}
        style={{
          display:"flex", alignItems:"center", gap:5,
          paddingLeft: 6 + depth * 12, paddingRight:8, paddingTop:3, paddingBottom:3,
          cursor:"pointer",
          background: isSelected ? "rgba(216,65,42,0.14)" : "transparent",
          borderLeft: isSelected ? "2px solid var(--accent)" : "2px solid transparent",
          fontSize:10, fontWeight: isSelected ? 700 : 500,
          color: isFolder ? "var(--ink)" : "var(--ink-soft)",
          letterSpacing:"0.02em",
        }}>
        <span style={{
          width:10, fontFamily:'"VT323", monospace', fontSize:13,
          color: isFolder ? "var(--ink-faint)" : "transparent",
          textAlign:"center",
        }}>
          {isFolder ? (hasChildren ? (isExpanded ? "▾" : "▸") : "·") : ""}
        </span>
        <span style={{
          fontFamily:'"VT323", monospace', fontSize:13,
          color: node.type === "bookmark" ? "var(--accent)" : "var(--ink-faint)",
        }}>
          {node.type === "bookmark" ? "★" : isFolder ? "▣" : "♪"}
        </span>
        <span style={{
          flex:1, overflow:"hidden", textOverflow:"ellipsis", whiteSpace:"nowrap",
          textTransform: isFolder ? "uppercase" : "none",
          fontWeight: node.type === "bookmark" ? 700 : (isFolder ? 600 : 500),
        }}>{node.name}</span>
        {!isFolder && (
          <span style={{
            fontSize:8, fontWeight:700, letterSpacing:"0.1em",
            color: hashColor(node.name), textTransform:"uppercase",
            padding:"1px 4px", borderRadius:2,
            background:`${hashColor(node.name)}22`,
            border:`1px solid ${hashColor(node.name)}55`,
          }}>{fileExt(node.name)}</span>
        )}
      </div>
      {isFolder && isExpanded && hasChildren && node.children.map(c => (
        <TreeNode key={c.path} node={c} depth={depth+1}
          expanded={expanded} onToggle={onToggle}
          selected={selected} onSelect={onSelect} />
      ))}
    </>
  );
}

function SampleBrowser() {
  // realistic empty-state-capable bookmark tree
  const [tree, setTree] = useStateB([
    {
      name: "Audio Library", path:"/lib", type:"bookmark",
      children: [
        { name:"909 kit", path:"/lib/909", type:"folder", children:[
          { name:"kick_punch.wav",      path:"/lib/909/kick_punch.wav",      type:"file" },
          { name:"kick_subby.wav",      path:"/lib/909/kick_subby.wav",      type:"file" },
          { name:"snare_tight.wav",     path:"/lib/909/snare_tight.wav",     type:"file" },
          { name:"snare_open.wav",      path:"/lib/909/snare_open.wav",      type:"file" },
          { name:"hat_closed.wav",      path:"/lib/909/hat_closed.wav",      type:"file" },
          { name:"hat_open.wav",        path:"/lib/909/hat_open.wav",        type:"file" },
          { name:"clap_layered.aif",    path:"/lib/909/clap_layered.aif",    type:"file" },
        ]},
        { name:"loops", path:"/lib/loops", type:"folder", children:[
          { name:"break_124.wav",       path:"/lib/loops/break_124.wav",     type:"file" },
          { name:"vinyl_loop.flac",     path:"/lib/loops/vinyl_loop.flac",   type:"file" },
        ]},
        { name:"keys", path:"/lib/keys", type:"folder", children:[] },
      ],
    },
    {
      name: "Field Recordings", path:"/field", type:"bookmark",
      children: [
        { name:"birds_dawn.wav",        path:"/field/birds_dawn.wav",        type:"file" },
        { name:"rain_window.wav",       path:"/field/rain_window.wav",       type:"file" },
        { name:"voice_memo_03.mp3",     path:"/field/voice_memo_03.mp3",     type:"file" },
      ],
    },
    {
      name: "Project Samples", path:"/proj", type:"bookmark",
      children: [],
    },
  ]);

  const [expanded, setExpanded] = useStateB(new Set(["/lib","/lib/909"]));
  const [selected, setSelected] = useStateB("/lib/909/kick_punch.wav");
  const [query, setQuery] = useStateB("");

  const toggle = (path) => {
    setExpanded(prev => {
      const next = new Set(prev);
      if (next.has(path)) next.delete(path); else next.add(path);
      return next;
    });
  };

  // when searching, filter to flat file list
  const filterTree = (nodes) => {
    if (!query) return nodes;
    const q = query.toLowerCase();
    const flat = [];
    const walk = (n) => {
      if (n.type === "file" && n.name.toLowerCase().includes(q)) flat.push(n);
      if (n.children) n.children.forEach(walk);
    };
    nodes.forEach(walk);
    return flat;
  };

  const visible = filterTree(tree);
  const selectedNode = (() => {
    let found = null;
    const walk = (n) => {
      if (n.path === selected) found = n;
      if (n.children) n.children.forEach(walk);
    };
    tree.forEach(walk);
    return found;
  })();

  return (
    <div style={{
      background:"var(--panel)",
      border:"1px solid var(--panel-rim)",
      borderRadius:8,
      boxShadow:"inset 0 1px 0 rgba(255,255,255,0.5), 0 1px 0 rgba(255,255,255,0.4)",
      display:"flex", flexDirection:"column",
      flex:1, minHeight:0, overflow:"hidden",
    }}>
      {/* header */}
      <div style={{
        padding:"10px 12px",
        background:"linear-gradient(180deg, var(--chassis), var(--chassis-2))",
        borderBottom:"1px solid var(--panel-rim)",
      }}>
        <div style={{ display:"flex", alignItems:"center", justifyContent:"space-between", marginBottom:8 }}>
          <div style={{ display:"flex", alignItems:"center", gap:8 }}>
            <div style={{ width:8, height:8, borderRadius:2, background:"var(--accent)" }} />
            <div style={{ fontSize:10, fontWeight:700, letterSpacing:"0.18em", color:"var(--ink)", textTransform:"uppercase" }}>
              BROWSER
            </div>
          </div>
          <div style={{ display:"flex", gap:4 }}>
            <button title="Add bookmark folder" style={miniBtn()}>＋</button>
            <button title="Refresh" style={miniBtn()}>↻</button>
          </div>
        </div>

        <div style={{
          display:"flex", alignItems:"center", gap:6,
          background:"var(--display-bg)",
          borderRadius:3, padding:"4px 8px",
          boxShadow:"inset 0 1px 3px rgba(0,0,0,0.7)",
          border:"1px solid rgba(0,0,0,0.5)",
        }}>
          <span style={{ fontFamily:'"VT323",monospace', color:"var(--display-fg)", fontSize:14, textShadow:"0 0 3px rgba(185,255,102,0.6)" }}>›</span>
          <input value={query} onChange={e => setQuery(e.target.value)}
            placeholder="filter files..."
            style={{
              flex:1, background:"transparent", border:"none", outline:"none",
              fontFamily:'"VT323",monospace', fontSize:14, color:"var(--display-fg)",
              letterSpacing:"0.05em",
            }} />
          {query && (
            <button onClick={() => setQuery("")} style={{
              background:"transparent", border:"none", cursor:"pointer",
              fontFamily:'"VT323",monospace', fontSize:13, color:"var(--display-fg)", opacity:0.6, padding:0,
            }}>×</button>
          )}
        </div>
      </div>

      {/* body: tree */}
      <div className="scroll" style={{ flex:1, overflowY:"auto", padding:"4px 0" }}>
        {tree.length === 0 ? (
          <EmptyState />
        ) : query ? (
          <div style={{ padding:"4px 0" }}>
            {visible.length === 0 ? (
              <div style={{ padding:"20px 12px", textAlign:"center", fontSize:10, color:"var(--ink-faint)", letterSpacing:"0.1em" }}>
                no matches
              </div>
            ) : visible.map(n => (
              <TreeNode key={n.path} node={n} depth={0}
                expanded={expanded} onToggle={toggle}
                selected={selected} onSelect={setSelected} />
            ))}
          </div>
        ) : tree.map(n => (
          <TreeNode key={n.path} node={n} depth={0}
            expanded={expanded} onToggle={toggle}
            selected={selected} onSelect={setSelected} />
        ))}
      </div>

      {/* preview footer */}
      {selectedNode && selectedNode.type === "file" && (
        <div style={{
          padding:"8px 10px",
          borderTop:"1px solid var(--panel-rim)",
          background:"linear-gradient(180deg, var(--chassis-2), var(--chassis))",
          display:"flex", flexDirection:"column", gap:6,
        }}>
          <div style={{ display:"flex", alignItems:"center", justifyContent:"space-between" }}>
            <div style={{ minWidth:0 }}>
              <div style={{ fontSize:9, fontWeight:600, letterSpacing:"0.12em", color:"var(--ink-faint)", textTransform:"uppercase" }}>
                PREVIEW
              </div>
              <div style={{ fontSize:11, fontWeight:700, color:"var(--ink)", overflow:"hidden", textOverflow:"ellipsis", whiteSpace:"nowrap" }}>
                {selectedNode.name}
              </div>
            </div>
            <LED on color="green" size={7} />
          </div>
          <div style={{
            background:"var(--display-bg)", borderRadius:3,
            padding:"6px 8px",
            boxShadow:"inset 0 2px 4px rgba(0,0,0,0.7)",
            border:"1px solid rgba(0,0,0,0.5)",
          }}>
            <MiniWave seed={selectedNode.name.charCodeAt(0)*7 + selectedNode.name.length*3} w={250} h={28} color="var(--display-fg)" />
          </div>
          <div style={{ display:"flex", gap:4 }}>
            <button style={{
              flex:1, padding:"5px 8px", borderRadius:3, cursor:"grab",
              fontSize:9, fontWeight:700, letterSpacing:"0.12em",
              fontFamily:'"JetBrains Mono", monospace',
              background:"var(--accent)", color:"#fff",
              border:"1px solid var(--accent)",
              boxShadow:"0 0 6px var(--accent)66",
            }}>⤓ DRAG TO RACK</button>
            <button title="Send to launchpad" style={miniBtn()}>▦</button>
            <button title="Loop preview" style={miniBtn()}>↻</button>
          </div>
        </div>
      )}
    </div>
  );
}

function EmptyState() {
  return (
    <div style={{
      padding:"32px 16px", textAlign:"center",
      display:"flex", flexDirection:"column", alignItems:"center", gap:10,
    }}>
      <div style={{
        width:48, height:48, borderRadius:8,
        background:"var(--display-bg)",
        border:"1px solid rgba(0,0,0,0.5)",
        boxShadow:"inset 0 2px 4px rgba(0,0,0,0.6)",
        display:"flex", alignItems:"center", justifyContent:"center",
        fontFamily:'"VT323", monospace', fontSize:24,
        color:"var(--display-fg)", opacity:0.5,
        textShadow:"0 0 4px rgba(185,255,102,0.4)",
      }}>★</div>
      <div style={{ fontSize:10, fontWeight:700, letterSpacing:"0.18em", color:"var(--ink)", textTransform:"uppercase" }}>
        No bookmarks
      </div>
      <div style={{ fontSize:9, color:"var(--ink-faint)", lineHeight:1.5, letterSpacing:"0.04em", maxWidth:200 }}>
        Add a folder to start browsing audio files. wav · aif · mp3 · flac · ogg
      </div>
      <button style={{
        marginTop:4, padding:"6px 12px", borderRadius:3, cursor:"pointer",
        fontSize:9, fontWeight:700, letterSpacing:"0.12em",
        fontFamily:'"JetBrains Mono", monospace',
        background:"var(--accent)", color:"#fff",
        border:"1px solid var(--accent)",
        boxShadow:"0 0 6px var(--accent)66",
      }}>＋ ADD FOLDER</button>
    </div>
  );
}

function miniBtn() {
  return {
    width:22, height:22, padding:0, borderRadius:3, cursor:"pointer",
    background:"linear-gradient(180deg, #ede5d1, #d8cdb1)",
    color:"var(--ink)",
    border:"1px solid rgba(0,0,0,0.25)",
    boxShadow:"inset 0 1px 0 rgba(255,255,255,0.5), 0 1px 0 rgba(0,0,0,0.15)",
    fontFamily:'"JetBrains Mono", monospace',
    fontSize:11, fontWeight:700,
    display:"flex", alignItems:"center", justifyContent:"center",
  };
}

Object.assign(window, { SampleBrowser });
