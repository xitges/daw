# Studio DAW

A professional digital audio workstation built in C++ using the JUCE framework.
Goal: feature parity with FL Studio, with a focus on being even more intuitive and user-friendly.

---

## Layout

```
┌──────────────────────────────────────────────────────────────┐
│  Toolbar Row 1 — Transport: Play / Stop / Rec / BPM / Mode / │
│                  Mixer / MIDI / Pad                          │
│  Toolbar Row 2 — Patterns / File / Export                    │
├──────────────────────────────────────────────────────────────┤
│  Playlist  (scrollable — bars × tracks timeline)             │
├──────────────────────────────────────────────────────────────┤
│  Channel Rack  (scrollable step sequencer)                   │
├──────────────────────────────────────────────────────────────┤
│  Mixer  (optional panel — 8 insert tracks + master)          │
└──────────────────────────────────────────────────────────────┘

Floating windows (opened on demand):
  Piano Roll · Synth Editor · FX Editor · Launchpad
```

---

## Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Space` | Play / Stop |
| `Cmd+Z` | Undo |
| `Cmd+Shift+Z` | Redo |
| `Cmd+S` | Save |
| `Cmd+N` | New project |
| `Cmd+O` | Open project |

---

## Toolbar

### Row 1 — Transport

| Control | How to use |
|---|---|
| **Play** | Starts playback. Pattern mode loops the active pattern; Song mode plays the full timeline from bar 0. |
| **Stop** | Stops playback, resets the playhead, and silences all voices immediately. |
| **Rec** | Planned — Audio Recording (M7). |
| **BPM slider** | Drag to change tempo (60–200 BPM). Takes effect immediately, even during playback. Default: 70. |
| **Pattern / Song** | Switches play mode. |
| **Mixer** | Toggles the Mixer panel at the bottom of the window. |
| **MIDI** | Opens a popup to select a MIDI input device and target channel. |
| **Pad** | Opens the Launchpad — an 8×8 pad grid for one-shot sample performance and sequence recording. |

### Row 2 — Pattern Management

| Control | How to use |
|---|---|
| **Pattern dropdown** | Selects the active pattern. The Channel Rack immediately loads its steps. |
| **+ button** | Creates a new empty 16-step pattern. |
| **= button** | Duplicates the active pattern (copies all steps). |
| **− button** | Deletes the active pattern. Disabled when only one pattern exists. |
| **Rename button** | Renames the active pattern via a dialog. |

### Row 2 — File & Export

| Control | How to use |
|---|---|
| **New** | Creates a new empty project. Warns if there are unsaved changes. |
| **Open** | Opens a `.studioproj` file from disk. |
| **Save** | Saves the current project. Prompts for a filename on first save. |
| **Save As** | Saves to a new file. |
| **Export WAV** | Renders the project to a WAV file (24-bit stereo, 44.1 kHz). In Pattern mode, exports 4 loops of the active pattern. In Song mode, exports the full timeline. |

The title bar shows the project name with `*` when there are unsaved changes.

---

## Channel Rack

The step sequencer. Each row is one instrument channel (Kick, Snare, HiHat, etc.).

### Step Grid

| Action | Result |
|---|---|
| **Click a step** | Toggles that step on (lit) or off. Active steps fire the channel's sample. |
| **Cmd+Z** | Undoes the last step toggle. |
| **Step count slider** (bottom-left) | Sets the pattern length from 1 to 64 steps in increments of 1. Click +/− or type directly. |
| **+ Add Channel** (bottom-left) | Adds a new empty channel row. |

### Per-Channel Controls

| Control | Location | Range | What it does |
|---|---|---|---|
| **Volume** (blue slider, top row) | Right of channel name | 0.0 – 1.0 | Playback volume. Constant-power law. |
| **Pan** (orange slider, bottom row) | Right of channel name | −1.0 (L) – +1.0 (R) | Stereo position. |
| **Pitch** (green vertical slider) | Centre column | −24 to +24 semitones | Pitch-shifts the sample via linear interpolation resampling. |
| **M** (Mute) | Far right, top | toggle | Silences this channel. Turns red when active. |
| **S** (Solo) | Far right, bottom | toggle | Solos this channel. Turns orange when active. |

### Loading Samples

Drag & drop any **WAV, AIFF, or MP3** file from Finder onto a channel row.
The row highlights green on hover. On drop the sample loads immediately and the filename appears in the label.

**Samples are stored per pattern.** Changing a sample in Pattern 1 does not affect Pattern 2. Switching patterns restores each pattern's own samples automatically.

### Renaming a Channel

Double-click the channel name text (left 90 px of the label area) to rename via a dialog.

### Scrolling

The Channel Rack scrolls vertically when there are more channels than fit on screen.

---

## Playlist

The song arrangement timeline. Horizontal = bars. Vertical = tracks (unlimited).
Both axes scroll independently.

### Snap Grid

The **Snap** dropdown (top-right of the playlist area) controls how clips snap when you create, move, or resize them:

| Setting | Snap unit |
|---|---|
| 1 Bar | Whole bars (default) |
| 1/2 Bar | Half bars (2 beats) |
| 1/4 Bar | Quarter bars (1 beat) |
| Free | No snap — pixel-level precision |

Sub-bar guide lines appear on the ruler when a finer snap is selected.

### Working with Clips

| Action | Result |
|---|---|
| **Double-click empty space** | Creates a new 4-bar clip assigned to the active pattern. Snaps to current grid. |
| **Drag clip left / right** | Moves clip horizontally. Snaps to grid. |
| **Drag clip up / down** | Moves clip to a different track. |
| **Drag right edge of clip** | Resizes clip length. Minimum size = 1 snap unit. The resize handle is the bright stripe on the right edge. |
| **Double-click existing clip** | Renames the clip. |
| **Right-click clip** | Opens context menu: Rename / Assign Pattern / Delete. |
| **Cmd+Z** | Undoes the last clip add, delete, move, or resize. |

Each clip shows a **mini step-grid preview** in its lower section — three rows of dots representing the first three channels of the assigned pattern.

### Managing Tracks

**Right-click the track label** (left 80 px of any track row) to open the track menu:

| Option | Action |
|---|---|
| **Rename Track** | Opens a rename dialog for that track. |
| **Add Track Below** | Inserts a new empty track below the current one. |
| **Delete Track** | Removes the track and all clips on it. Disabled when only one track exists. |

Tracks are saved and restored with the project file. The three dots (···) on each label hint at the right-click menu.

### Playhead

The red vertical line shows the current playback position in Song mode. Updates at 30 Hz.

---

## Built-in Synthesizer (M13)

Right-click any channel in the Channel Rack → **Open Synth Editor** to open the synth parameter window.

### Enabling the Synth

Toggle **Enable Synth** in the editor. When enabled, the channel renders audio from the built-in synthesizer instead of a loaded sample file.

- For **Drum channels**: the synth fires on each active step at a fixed pitch (C4).
- For **Melodic channels**: the synth fires on Piano Roll NoteEvents with the correct pitch and duration.

### Synth Parameters

| Parameter | Description |
|---|---|
| **Waveform** | Oscillator shape: Sine / Saw / Square / Triangle |
| **Attack** | Envelope attack time (ms) |
| **Decay** | Envelope decay time (ms) |
| **Sustain** | Envelope sustain level (0 – 1) |
| **Release** | Envelope release time (ms) |
| **Cutoff** | Low-pass filter cutoff frequency (Hz) |
| **Resonance** | Filter resonance / Q (0 – 1) |
| **LFO Rate** | LFO oscillation speed (Hz) |
| **LFO Depth** | LFO modulation amount (0 = off) |
| **LFO Target** | What the LFO modulates: Cutoff or Pitch |

The synth is 8-voice polyphonic with round-robin voice stealing.

---

## Built-in FX (M14)

Open the **Mixer** panel, then click the purple **FX** button on any mixer track strip. This opens the FX chain editor for that track.

### FX Chain Order

```
Compressor → Delay → Reverb
```

Each effect can be enabled/disabled independently.

### Compressor

| Parameter | Description |
|---|---|
| **Threshold (dB)** | Level above which gain reduction kicks in |
| **Ratio** | Compression ratio (1 = bypass, 20 = hard limit) |
| **Attack (ms)** | How fast the compressor responds to transients |
| **Release (ms)** | How fast the compressor recovers |

### Delay

| Parameter | Description |
|---|---|
| **Time (beats)** | Delay time as a beat fraction (0.25 = 16th, 0.5 = 8th, 1.0 = quarter) |
| **Feedback** | How much of the delayed signal feeds back (0 – 0.95) |
| **Mix** | Wet/dry blend (0 = dry, 1 = full wet) |

The delay is BPM-synced — it automatically adjusts to the current project BPM.

### Reverb

| Parameter | Description |
|---|---|
| **Room Size** | Size of the simulated space (0 – 1) |
| **Damping** | High-frequency absorption (0 = bright, 1 = dark) |
| **Wet Mix** | Reverb return level (0 = dry, 1 = fully wet) |
| **Width** | Stereo width of the reverb tail (0 = mono, 1 = full stereo) |

---

## Mixer

Open/close with the **Mixer** button in the toolbar. Shows 8 insert tracks (Track 1–8) and a Master strip.

| Control | What it does |
|---|---|
| **Volume fader** | Sets the track output level |
| **Pan knob** | Stereo position |
| **M (Mute)** | Silences the track |
| **S (Solo)** | Solos the track |
| **FX (purple button)** | Opens the FX chain editor for that track |

Each channel in the Channel Rack routes to a mixer track. The routing label is shown under the channel name and can be changed in the mixer.

---

## Launchpad

Open with the **Pad** button in the toolbar. An 8×8 grid of one-shot sample pads for live performance and beat recording.

### Assigning Samples to Pads

- **Drag & drop** an audio file from Finder onto any pad cell.
- **Right-click** a pad → **Load Sample...** to browse for a file, or **Clear** to remove it.

Loaded pads glow green; empty pads are dark.

### Triggering Pads

- **Click** a pad with the mouse.
- **Keyboard** (while the Launchpad window is focused):

| Keyboard row | Pads |
|---|---|
| `1 2 3 4 5 6 7 8` | Row 1 (pads 0–7) |
| `Q W E R T Y U I` | Row 2 (pads 8–15) |
| `A S D F G H J K` | Row 3 (pads 16–23) |
| `Z X C V B N M ,` | Row 4 (pads 24–31) |

### Recording a Sequence

1. Set the **Bars** dropdown to the desired recording length (1, 2, or 4 bars).
2. Click **REC** — the button turns red and a countdown label shows the beat position.
3. Press pads in time. Each hit is recorded with beat-accurate timing.
4. Recording stops automatically after the set number of bars (or click **REC** again to stop early).
5. Click **> Pattern** to convert the recording to a new pattern. A dialog asks for a name.

The converted pattern maps each unique pad to its own channel, copies the pad's sample, and quantizes hits to the nearest 16th note. The new pattern appears immediately in the pattern list.

---

## Save / Load

Projects are saved as `.studioproj` files (XML format). Saved data includes: patterns (steps + notes + per-channel samples), playlist clips, playlist tracks, mixer tracks, channel routing, synth parameters, FX parameters, and launchpad pad assignments.

---

## Milestone Progress

현재 프로젝트는 **패턴 기반 시퀀싱 + 플레이리스트 + 기본 믹싱 + 내장 신스/샘플러**
축은 이미 usable한 수준까지 올라왔고, 앞으로는 **FL Studio형 워크플로우를 완성하는
고급 편집 / 믹서 라우팅 / 오디오 클립 / 정밀 재생** 영역이 핵심 과제다.

| # | Feature | Status | Current State |
|---|---|---|---|
| M1 | Channel Rack — Vol, Pan, Pitch, Mute/Solo, Scroll, Rename | **Done** | 채널 기본 제어, 이름 변경, per-pattern sample isolation까지 반영 |
| M2 | Multiple Patterns, Clip Editing, Pattern-to-Playlist link | **Done** | 다중 패턴, 클립 생성/이동/리사이즈, 패턴 detach, unassigned clip 흐름 지원 |
| M3 | Piano Roll (melodic note editor) | **Done** | 선택/다중선택, marquee, nudging, MIDI import/export, scale/key, note preview 포함 |
| M4 | Save / Load (XML project file) | **Done** | 패턴, 노트, 클립, 믹서, FX, 런치패드, 주요 UI 상태 저장/복원 |
| M5 | Mixer (per-track faders, routing, FX slots) | **Partial** | insert-style mixing과 track FX는 있으나 send/return/sidechain은 아직 없음 |
| M6 | Undo / Redo | **Done** | 주요 편집 작업에 대한 undo/redo 흐름 존재 |
| M7 | Audio Recording (mic / line-in → clip) | Planned | 오디오 clip 기반 녹음 경로는 아직 미구현 |
| M8 | VST / AU Plugin Hosting | **Partial** | plugin scan/load/editor/state 저장은 있으나 latency/PDC·고급 routing은 미완 |
| M9 | Automation (parameter curves on timeline) | **Partial** | automation lane 기초는 있으나 범위/대상/편집 UX는 확장 필요 |
| M10 | Export / Render to WAV | **Done** | pattern/song render 지원 |
| M11 | UI Polish, Zoom, Track Management, Pattern Preview | **Partial** | dark theme, zoom, clip note preview, audio device UI, track management 반영 |
| M12 | MIDI Input / Output | **Partial** | MIDI input device 선택 + live trigger는 구현, MIDI output/정밀 timestamp recording은 남음 |
| M13 | Built-in Instruments (synth, sampler) | **Done** | built-in synth/sampler와 프리셋, sample voice pool, preview 경로 구현 |
| M14 | Built-in FX (EQ, Compressor, Reverb, Delay) | **Partial** | compressor, reverb, delay, dynamic EQ 경로는 있으나 FL급 FX ecosystem은 아직 부족 |
| M15 | Sample Browser / Library Panel | **Partial** | 브라우저/preview/bookmark는 있으나 라이브러리 관리와 asset workflow는 확장 필요 |
| M16 | Auto-save, Recent Files, Project Templates | Planned | 프로젝트 운영성 기능은 아직 없음 |
| — | Launchpad (8×8 sample pads + sequence recording) | **Done** | 8×8 pad trigger, sample assignment, 녹음 변환 지원 |
| — | Per-pattern sample isolation | **Done** | 패턴별 샘플/채널 설정 독립성 확보 |
| M17 | Piano Roll Recording — MIDI Audio-Thread Timestamps | Planned | 현재보다 더 정확한 MIDI capture용 정밀 타임스탬프 계층 필요 |
| M18 | MVC Architecture — Quantize / Grid settings in ProjectModel | Planned | quantize/grid 등 editor state를 모델로 승격하는 단계 |
| M19 | Audio Clips — Trim, Fade, Gain, Stretch-ready Playlist Clips | Planned | 오디오 중심 DAW 워크플로우의 핵심 공백 |
| M20 | Mixer Routing — Send/Return Bus, Sidechain Workflow | Planned | FL형 믹싱 워크플로우로 가기 위한 최우선 기능 중 하나 |
| M21 | Synth Articulation — Legato, Slide, Mono Priority | Planned | 808/lead 표현과 note transition 안정화의 핵심 |
| M22 | Playback Accuracy — Plugin Delay Compensation (PDC) | Planned | plugin latency가 커질수록 필수 |
| M23 | Advanced Piano Roll Tools — Ghost Notes, Strum, Flam, Randomize | Planned | FL Studio식 작곡 보조 툴 세트 |

---

## Milestones — Detail

### Completed Core

#### M1 · Channel Rack

채널별 볼륨, 팬, 피치, 뮤트/솔로, 샘플 로드, 이름 변경, step 편집이 가능한 기본
Channel Rack 워크플로우가 구현되어 있다. 현재는 패턴별 채널 상태가 독립적으로
유지되어, FL Studio식 pattern-local instrument 설정에 더 가까워졌다.

#### M2 · Patterns / Playlist Clips

패턴 생성/복제/삭제, 플레이리스트 클립 생성/이동/리사이즈, 클립-패턴 연결,
detach-to-new-pattern 흐름이 작동한다. 최근에는 **unassigned clip** 개념과
클립 라벨에 패턴명 표시, 늘어난 클립의 looped preview까지 반영됐다.

#### M3 · Piano Roll

피아노롤은 단순 note editor를 넘어, 선택/다중선택, marquee selection,
keyboard nudging, vertical/horizontal note move, MIDI import/export, key/scale
표시, session note move까지 포함하는 편집 도구로 확장됐다. 아직 FL 수준의
ghost notes / strum / legato / slide는 남아 있다.

#### M4 · Save / Load

`.studioproj` 기반 XML 저장/불러오기로 패턴, 노트, 클립, 믹서, FX, 런치패드,
신스 설정, 프로젝트 메타를 복원할 수 있다. 최근에는 활성 패턴, 일부 UI 재동기화,
미할당 클립 default 처리 등 실제 사용 흐름을 더 잘 반영하도록 보강됐다.

#### M6 · Undo / Redo

핵심 편집 작업에 대해 undo/redo 기반이 들어가 있어 기본적인 비파괴 편집 흐름이
가능하다. 앞으로 audio clip editing과 automation이 확장되면 적용 범위를 더
넓혀야 한다.

#### M10 · Export / Render

Pattern / Song 모드 모두 WAV 렌더가 가능하다. 이후 freeze / render in place /
stem export 같은 FL형 생산성 기능으로 확장할 여지가 크다.

#### M13 · Built-in Instruments

built-in synth와 sampler, sample voice pool, launchpad/browser preview, 패턴별
신스 파라미터가 동작한다. 최근에는 note-on/off click 완화를 위해 de-click
보호가 여러 단계 추가되었지만, legato/mono articulation은 아직 다음 단계다.

#### Launchpad / Per-pattern Sample Isolation

런치패드는 성능용 보조 패드이자 시퀀스 기록 입력원으로 동작하며, 패턴 변환도
지원한다. 또한 패턴별 sample isolation이 들어가 있어 “패턴이 바뀌면 채널 성격도
달라지는” FL Studio식 작업 흐름에 더 가까워졌다.

---

### Partial Milestones

#### M5 · Mixer

**현재 상태:** per-track volume/pan/mute/solo, insert FX, routing 기반은 있다.  
**부족한 점:** send/return bus, sidechain source, pre/post-fader routing, PDC가 없다.  
**다음 단계:** M20, M22와 강하게 연결해 mixer를 “playback sum”이 아니라
실제 production mixer로 확장한다.

#### M8 · VST / AU Plugin Hosting

**현재 상태:** plugin scan/load/editor/state 저장이 가능하다.  
**부족한 점:** plugin latency 보정, 고급 bus routing, instrument/effect workflow
정교화가 필요하다.  
**다음 단계:** M22(PDC)와 함께 실제 프로젝트에서 timing이 무너지지 않도록
재생 정렬을 완성해야 한다.

#### M9 · Automation

**현재 상태:** automation lane 구조와 timeline 상의 기본 표현은 있다.  
**부족한 점:** 대상 파라미터 확장, 곡선 편집, clip automation, smoothing, 더 나은
UX가 필요하다.  
**다음 단계:** mixer / synth / FX / plugin parameter를 일관되게 automation
대상으로 묶는 공통 계층이 필요하다.

#### M11 · UI Polish

**현재 상태:** dark theme, zoom, track management, playlist note preview, 오디오
디바이스 선택 UI, piano roll selection tool 등 실사용 기능이 많이 들어갔다.  
**부족한 점:** 레이아웃 정제, 작은 팝업 UI 품질, FL형 툴바 일관성, 더 높은 정보
밀도와 시각적 정리가 필요하다.

#### M12 · MIDI I/O

**현재 상태:** MIDI input device 선택과 live note trigger는 동작한다.  
**부족한 점:** MIDI output, sample-accurate timestamp capture, 외부 clock/sync,
더 좋은 recording pipeline이 아직 없다.  
**다음 단계:** M17과 연결해서 오디오 스레드 기반 timestamp capture로 간다.

#### M14 · Built-in FX

**현재 상태:** compressor, reverb, delay, dynamic EQ 경로는 존재한다.  
**부족한 점:** FL의 폭넓은 FX 선택지, sidechain-centric workflow, bus send 활용이
부족하다.  
**다음 단계:** M20과 결합해 공용 FX return / ducking workflow를 만든다.

#### M15 · Sample Browser

**현재 상태:** sample browser, preview, bookmark 기반은 들어가 있다.  
**부족한 점:** 대형 라이브러리 탐색, tag/search, 빠른 drag-drop placement,
프로젝트 asset 관리가 아직 약하다.

---

### Next Milestones

#### M7 · Audio Recording: mic / line-in → clip

**문제:** 현재는 패턴/MIDI 중심이라 보컬, 기타, 외부 입력 녹음의 DAW 핵심
워크플로우가 빠져 있다.

**목표:** 입력 소스를 timeline clip으로 녹음하고, 이후 trim/fade/edit 가능한
audio clip 기반 작업 흐름을 만든다.

**주요 작업:**
- input device / monitoring / armed track 개념 추가
- recording take를 playlist audio clip으로 생성
- clip split/trim/fade와 연결
- metronome / count-in / latency 보정 검토

---

#### M16 · Auto-save / Recent Files / Project Templates

**문제:** 프로젝트 운영 편의성이 아직 개발용 수준에 머물러 있다.

**목표:** 실제 사용 중 크래시 복구, 최근 파일 접근, 템플릿 시작 워크플로우를 지원한다.

**주요 작업:**
- 주기적 autosave와 recovery file 정책
- recent files 메뉴와 최근 프로젝트 로드
- default template / genre template / empty template 흐름
- dirty state와 저장 UX 정리

---

#### M17 · Piano Roll Recording: MIDI Audio-Thread Timestamps

**문제:** `keyStateChanged`는 UI(메시지) 스레드에서 호출되어 OS 메시지 큐 지연이
발생할 수 있다. CPU 부하가 높을 때 실제 연주 시점보다 늦게 `playheadBeat`가
캡처되어 타이밍이 어긋난다.

**목표:** `AudioEngine::handleIncomingMidiMessage`(오디오 스레드)에서
note-on/off 타임스탬프를 고정밀로 기록하고, lock-free 큐(예: `juce::AbstractFifo`)
를 통해 메시지 스레드의 `PianoRollComponent`에 전달한다.

**주요 작업:**
- AudioEngine에 lock-free MIDI event FIFO 추가
- 오디오 콜백에서 sample-accurate beat position 계산
- PianoRollComponent의 timerCallback에서 FIFO를 drain하여 NoteEvent 커밋
- 기존 UI thread 경로는 fallback으로 유지

---

#### M18 · MVC Architecture: Quantize / Grid를 ProjectModel로 이동

**문제:** `quantizeEnabled`, `quantizeGrid` 등 편집/녹음 설정이 컴포넌트 내부에
분산되어 있어 여러 입력 경로와 저장 포맷 간 정합성이 떨어질 수 있다.

**목표:** 녹음·퀀타이즈·그리드 관련 설정을 모델 계층으로 이동시키고, UI는 해당
모델을 참조/표시하는 방향으로 정리한다.

**주요 작업:**
- `ProjectModel`에 recording/editor settings 구조체 추가
- `ProjectSerializer`에 저장/불러오기 연동
- `PianoRollComponent`, `LaunchpadComponent`, 기타 입력 경로가 공통 모델 참조
- `onSettingsChanged` 기반 UI 동기화

---

#### M19 · Audio Clips: Trim / Fade / Gain / Stretch-ready Playlist Clips

**문제:** 현재 플레이리스트는 패턴 클립 중심이라 오디오 파일을 직접 배치하고
트리밍/페이드/게인 조절하는 DAW형 워크플로우가 부족하다.

**목표:** 오디오 클립을 플레이리스트에 직접 배치하고, clip 단위의 시작/끝 trim,
fade-in/out, gain, 이후 time-stretch 확장이 가능한 데이터 구조를 마련한다.

**주요 작업:**
- `ProjectModel`에 audio clip 전용 구조체 또는 clip type 구분 추가
- 플레이리스트에서 audio clip 파형 preview 렌더링
- clip trim / fade / gain 편집 핸들 추가
- AudioEngine Song 모드에서 audio clip 재생 경로 추가

---

#### M20 · Mixer Routing: Send / Return Bus, Sidechain Workflow

**문제:** 현재 믹서는 insert 중심이라 공용 reverb/delay bus, send 양 조절,
sidechain trigger 같은 실제 믹싱 흐름이 제한적이다.

**목표:** 각 채널/트랙이 send bus로 신호를 보내고, return track에서 공용 FX를
처리하며, compressor 등에서 sidechain source를 받을 수 있게 한다.

**주요 작업:**
- mixer track 간 send amount / routing 데이터 모델 추가
- return bus 렌더 경로와 pre/post-fader routing 정의
- compressor sidechain 입력 버스 추가
- Mixer UI에 send knob / sidechain source selector 추가

---

#### M21 · Synth Articulation: Legato / Slide / Mono Priority

**문제:** 현재 built-in synth는 note-on 시 위상/필터/ADSR를 재시작하는 구조라
lead, bass, 808 계열에서 FL Studio식 glide / legato 표현이 부족하고 note
transition 안정화도 더 필요하다.

**목표:** 채널 또는 패치 단위로 mono / legato / slide 모드를 제공하고, 겹치거나
붙은 노트 사이를 재트리거 대신 연속 pitch transition으로 처리한다.

**주요 작업:**
- `SynthParams`에 mono, legato, glide time 등 articulation 옵션 추가
- overlap note 해석 정책(재트리거 vs sustain vs slide) 정의
- PolySynth에 mono voice priority / pitch glide 경로 추가
- Piano Roll에서 slide note 또는 tie-like overlap UX 검토

---

#### M22 · Playback Accuracy: Plugin Delay Compensation (PDC)

**문제:** VST/AU 또는 lookahead FX가 생기면 트랙별 latency가 달라져 timing이
밀리고, 병렬 routing 시 위상/정렬 문제가 발생할 수 있다.

**목표:** plugin / FX chain latency를 수집해 mixer 및 song playback 전체를
자동 정렬하는 PDC를 도입한다.

**주요 작업:**
- plugin instance / FX chain latency query 수집
- mixer track / master 기준 latency 정렬 정책 설계
- delayed audio/midi buffering 경로 추가
- 렌더와 실시간 playback 모두 동일한 정렬 보장

---

#### M23 · Advanced Piano Roll Tools: Ghost Notes / Strum / Flam / Randomize

**문제:** 현재 Piano Roll은 직접 편집 기능은 갖췄지만, FL Studio의 강점인
작곡 보조 툴(ghost notes, strum, flam, randomize, humanize)이 부족하다.

**목표:** 빠른 아이디어 스케치와 human feel 부여를 위한 고급 편집 툴을 추가해
피아노롤을 단순 note editor에서 composition tool로 확장한다.

**주요 작업:**
- 다른 채널/패턴 노트를 희미하게 표시하는 ghost notes 추가
- 선택 노트 대상 strum / flam / randomize / humanize commands 추가
- velocity, timing, length를 범위 기반으로 변형하는 dialog/tooling 추가
- scale/key snapping과 함께 동작하는 일관된 note transform pipeline 정리

---
 
## Tech Stack

- **Language:** C++17
- **Framework:** JUCE
- **Audio:** 44,100 Hz sample rate, 512-sample buffer
- **Project format:** XML (`.studioproj`)
- **Export format:** WAV (24-bit stereo)
