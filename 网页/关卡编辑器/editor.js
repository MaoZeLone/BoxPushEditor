const EMPTY = 0;
const FLOOR = 1;
const WALL = 2;
const STORAGE_KEY = "boxpush-web-editor-v1";

const DEFS = {
  Box_Normal: { name: "普通箱", folder: "Box", blocking: true, pushable: true },
  Box_Slide: { name: "滑行箱", folder: "Box", blocking: true, pushable: true, slide: true },
  Box_Return: { name: "回程箱", folder: "Box", blocking: true, pushable: true, return: true },
  Target: { name: "目标", folder: "Target", goal: true },
  Pedal: { name: "踏板", folder: "Pedal", pedal: true },
};

const TOOLS = {
  floor: "地板",
  wall: "墙",
  empty: "空洞",
  eraser: "橡皮",
  player: "玩家",
  Box_Normal: "普通箱",
  Box_Slide: "滑行箱",
  Box_Return: "回程箱",
  Target: "目标",
  Pedal: "踏板",
};

const FOLDERS = [
  { id: "Game", path: "/Game", depth: 0 },
  { id: "Terrain", path: "/Game/Terrain", depth: 1 },
  { id: "Characters", path: "/Game/Characters", depth: 1 },
  { id: "Interactables", path: "/Game/Interactables", depth: 1 },
  { id: "Box", path: "/Game/Interactables/Box", depth: 2 },
  { id: "Target", path: "/Game/Interactables/Target", depth: 2 },
  { id: "Pedal", path: "/Game/Interactables/Pedal", depth: 2 },
];

const ASSETS = [
  { tool: "floor", name: "SM_Floor", type: "StaticMesh", folder: "Terrain" },
  { tool: "wall", name: "SM_Wall", type: "StaticMesh", folder: "Terrain" },
  { tool: "empty", name: "Empty", type: "Terrain", folder: "Terrain" },
  { tool: "eraser", name: "Eraser", type: "EditorUtility", folder: "Terrain" },
  { tool: "player", name: "DA_Player", type: "PlayerDef", folder: "Characters" },
  { tool: "Box_Normal", name: "DA_Box_Normal", type: "InteractableDef", folder: "Box" },
  { tool: "Box_Slide", name: "DA_Box_Slide", type: "InteractableDef", folder: "Box" },
  { tool: "Box_Return", name: "DA_Box_Return", type: "InteractableDef", folder: "Box" },
  { tool: "Target", name: "DA_Target", type: "InteractableDef", folder: "Target" },
  { tool: "Pedal", name: "DA_Pedal", type: "InteractableDef", folder: "Pedal" },
];

const FOLDER_CHILDREN = {
  Game: ["Terrain", "Characters", "Interactables", "Box", "Target", "Pedal"],
  Interactables: ["Box", "Target", "Pedal"],
};

const state = {
  levels: [],
  currentId: "",
  tool: "floor",
  folder: "Game",
  assetFilter: "",
  mode: "edit",
  play: null,
  hover: null,
  highlight: [],
  issues: [],
  filter: "",
  status: "左键铺设，右键橡皮。试玩不写档。",
  gesture: null,
};

function defOf(id) {
  return DEFS[id] || null;
}

function idx(level, x, y) {
  return y * level.width + x;
}

function inside(level, x, y) {
  return x >= 0 && y >= 0 && x < level.width && y < level.height;
}

function terrainAt(level, x, y) {
  if (!inside(level, x, y) || level.cells.length !== level.width * level.height) return EMPTY;
  return level.cells[idx(level, x, y)];
}

function standable(level, x, y) {
  return terrainAt(level, x, y) === FLOOR;
}

function cloneLevel(level) {
  return {
    levelId: level.levelId,
    displayName: level.displayName,
    designerNote: level.designerNote,
    width: level.width,
    height: level.height,
    cells: level.cells.slice(),
    player: { x: level.player.x, y: level.player.y },
    instances: level.instances.map((inst) => ({ id: inst.id, def: inst.def, x: inst.x, y: inst.y })),
    listed: !!level.listed,
    builtin: !!level.builtin,
  };
}

function blankLevel(levelId) {
  const width = 8;
  const height = 8;
  const cells = [];
  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const border = x === 0 || y === 0 || x === width - 1 || y === height - 1;
      cells.push(border ? WALL : FLOOR);
    }
  }
  return {
    levelId,
    displayName: "未命名关卡",
    designerNote: "",
    width,
    height,
    cells,
    player: { x: 2, y: 3 },
    instances: [
      { id: "Box_0", def: "Box_Normal", x: 3, y: 3 },
      { id: "Target_0", def: "Target", x: 5, y: 3 },
    ],
    listed: false,
    builtin: false,
  };
}

function paintMap(rows) {
  const height = rows.length;
  const width = rows[0].length;
  const cells = [];
  let player = { x: 1, y: 1 };
  const instances = [];
  const counts = {};
  const mark = {
    "#": WALL,
    ".": FLOOR,
    " ": EMPTY,
  };
  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const ch = rows[y][x];
      if (ch === "@" || ch === "$" || ch === "S" || ch === "R" || ch === "o" || ch === "p") {
        cells.push(FLOOR);
      } else {
        cells.push(mark[ch] ?? FLOOR);
      }
      if (ch === "@") player = { x, y };
      const def = { $: "Box_Normal", S: "Box_Slide", R: "Box_Return", o: "Target", p: "Pedal" }[ch];
      if (def) {
        counts[def] = (counts[def] || 0) + 1;
        instances.push({ id: `${def}_${counts[def]}`, def, x, y });
      }
    }
  }
  return { width, height, cells, player, instances };
}

function seedLevels() {
  const basic = paintMap([
    "########",
    "#......#",
    "#......#",
    "#.@$.o.#",
    "#......#",
    "#......#",
    "#......#",
    "########",
  ]);
  const holes = paintMap([
    "########",
    "#......#",
    "#..  ..#",
    "#.@$.o.#",
    "#..  ..#",
    "#......#",
    "#......#",
    "########",
  ]);
  const slide = paintMap([
    "########",
    "#......#",
    "#......#",
    "#.@S..o#",
    "#......#",
    "#......#",
    "#......#",
    "########",
  ]);
  return [
    {
      levelId: "LV_01",
      displayName: "走、推、目标",
      designerNote: "往右推两步，箱子进目标。",
      listed: true,
      builtin: true,
      ...basic,
    },
    {
      levelId: "LV_02",
      displayName: "墙与空洞",
      designerNote: "空洞不能站，也不能把箱子推进去。",
      listed: true,
      builtin: true,
      ...holes,
    },
    {
      levelId: "LV_03",
      displayName: "滑行箱",
      designerNote: "推一下，箱子滑到墙前。",
      listed: true,
      builtin: true,
      ...slide,
    },
  ];
}

function instancesAt(level, x, y, ignoreId) {
  return level.instances.filter((inst) => inst.x === x && inst.y === y && inst.id !== ignoreId);
}

function blockersAt(level, x, y, ignoreId) {
  return instancesAt(level, x, y, ignoreId).filter((inst) => defOf(inst.def)?.blocking);
}

function pushTravel(level, box, dx, dy) {
  const def = defOf(box.def);
  const max = def?.slide ? Math.max(level.width, level.height) : 1;
  let travel = 0;
  let x = box.x;
  let y = box.y;
  for (let step = 0; step < max; step += 1) {
    const nx = x + dx;
    const ny = y + dy;
    if (!standable(level, nx, ny)) break;
    if (blockersAt(level, nx, ny, box.id).length) break;
    x = nx;
    y = ny;
    travel += 1;
  }
  return travel;
}

function sameCell(a, b) {
  return a.x === b.x && a.y === b.y;
}

function refreshOccupancy(level) {
  for (const inst of level.instances) {
    const def = defOf(inst.def);
    if (!def) continue;
    if (def.goal) {
      const filled = level.instances.some((other) => other.id !== inst.id && other.x === inst.x && other.y === inst.y && defOf(other.def)?.folder === "Box");
      inst.state = filled ? "Occupied" : "Idle";
    } else if (def.pedal) {
      const pressed = level.instances.some((other) => other.id !== inst.id && other.x === inst.x && other.y === inst.y && defOf(other.def)?.folder === "Box");
      inst.state = pressed ? "Pressed" : "Idle";
    }
  }
}

function isWon(level) {
  const goals = level.instances.filter((inst) => defOf(inst.def)?.goal);
  if (!goals.length) return false;
  return goals.every((goal) => level.instances.some((other) => other.id !== goal.id && other.x === goal.x && other.y === goal.y && defOf(other.def)?.folder === "Box"));
}

function snapshot(play) {
  return {
    player: { ...play.level.player },
    instances: play.level.instances.map((inst) => ({ ...inst })),
    won: play.won,
  };
}

function restore(play, snap) {
  play.level.player = { ...snap.player };
  play.level.instances = snap.instances.map((inst) => ({ ...inst }));
  play.won = snap.won;
  refreshOccupancy(play.level);
}

function flushReturns(level) {
  const moves = [];
  for (const inst of level.instances) {
    if (inst.movesUntilReturn !== 0) continue;
    const home = inst.returnHome;
    const blocked = !home || !standable(level, home.x, home.y) || blockersAt(level, home.x, home.y, inst.id).length || (home.x === level.player.x && home.y === level.player.y);
    if (home && (home.x !== inst.x || home.y !== inst.y) && !blocked) {
      moves.push({ id: inst.id, x0: inst.x, y0: inst.y, x1: home.x, y1: home.y });
      inst.x = home.x;
      inst.y = home.y;
    }
    inst.movesUntilReturn = -1;
  }
  refreshOccupancy(level);
  return moves;
}

function tryMove(play, dx, dy) {
  if (play.won || play.anim) return null;
  if (Math.abs(dx) + Math.abs(dy) !== 1) return null;
  const level = play.level;
  const from = { ...level.player };
  const dest = { x: from.x + dx, y: from.y + dy };
  if (!standable(level, dest.x, dest.y)) return null;

  const blocking = blockersAt(level, dest.x, dest.y);
  const before = snapshot(play);
  const motions = [{ id: "player", x0: from.x, y0: from.y, x1: dest.x, y1: dest.y }];

  if (blocking.length) {
    const box = blocking.find((inst) => defOf(inst.def)?.pushable);
    if (!box) return null;
    const travel = pushTravel(level, box, dx, dy);
    if (travel <= 0) return null;
    motions.push({ id: box.id, x0: box.x, y0: box.y, x1: box.x + dx * travel, y1: box.y + dy * travel });
    box.returnHome = { x: box.x, y: box.y };
    box.x += dx * travel;
    box.y += dy * travel;
    if (defOf(box.def)?.return) box.movesUntilReturn = 0;
  }

  level.player = dest;
  play.undo.push(before);
  refreshOccupancy(level);
  play.won = isWon(level);
  return { motions };
}

function beginPlay(level) {
  const live = cloneLevel(level);
  for (const inst of live.instances) inst.movesUntilReturn = -1;
  refreshOccupancy(live);
  return {
    level: live,
    undo: [],
    won: isWon(live),
    anim: null,
    pendingReturns: false,
  };
}

function validate(level, levels) {
  const issues = [];
  const error = (message, cells = []) => issues.push({ error: true, message, cells });
  const warn = (message, cells = []) => issues.push({ error: false, message, cells });

  if (!level.levelId) error("LevelId 为空");
  if (levels.filter((item) => item.levelId === level.levelId).length > 1) error("LevelId 与已有关重复");
  if (level.width < 5 || level.width > 20 || level.height < 5 || level.height > 20) error("宽或高越界（5–20）");
  if (level.cells.length !== level.width * level.height) error("Cells 长度必须等于 Width * Height");
  if (!inside(level, level.player.x, level.player.y)) error("玩家出生点在地图外", [level.player]);
  else if (!standable(level, level.player.x, level.player.y)) error("玩家出生点必须在地板上", [level.player]);

  const seen = new Set();
  let pushable = 0;
  let goals = 0;
  const pushCells = [];
  const blocking = new Map();
  for (const inst of level.instances) {
    if (!inst.id) error("实例缺少 InstanceId", [{ x: inst.x, y: inst.y }]);
    else if (seen.has(inst.id)) error(`InstanceId 重复：${inst.id}`, [{ x: inst.x, y: inst.y }]);
    seen.add(inst.id);
    if (!defOf(inst.def)) error(`实例 ${inst.id} 的定义无法加载`, [{ x: inst.x, y: inst.y }]);
    if (!inside(level, inst.x, inst.y) || !standable(level, inst.x, inst.y)) {
      error("玩家或箱子落在空洞 / 墙上", [{ x: inst.x, y: inst.y }]);
    }
    const def = defOf(inst.def);
    if (def?.pushable) {
      pushable += 1;
      pushCells.push({ x: inst.x, y: inst.y });
    }
    if (def?.goal) goals += 1;
    if (def?.blocking) {
      const key = `${inst.x},${inst.y}`;
      const list = blocking.get(key) || [];
      list.push(inst);
      blocking.set(key, list);
    }
  }
  if (pushable !== goals || pushable < 1) {
    error(`箱子 ${pushable} 个，目标 ${goals} 个，胜利条件无法达成`, pushCells);
  }
  for (const list of blocking.values()) {
    if (list.length > 1) {
      error(`格子 (${list[0].x},${list[0].y}) 叠了 ${list.length} 个阻挡实例`, [{ x: list[0].x, y: list[0].y }]);
    }
  }
  if (!level.designerNote) warn("策划备注为空，这一关的教学意图没写");

  let openRim = false;
  if (level.cells.length === level.width * level.height) {
    for (let x = 0; x < level.width; x += 1) {
      if (terrainAt(level, x, 0) !== WALL || terrainAt(level, x, level.height - 1) !== WALL) openRim = true;
    }
    for (let y = 0; y < level.height; y += 1) {
      if (terrainAt(level, 0, y) !== WALL || terrainAt(level, level.width - 1, y) !== WALL) openRim = true;
    }
  }
  if (openRim) warn("最外圈没有封闭的墙，角色可能走出去");
  return issues;
}

function resizeLevel(level, width, height) {
  width = Math.max(5, Math.min(20, width));
  height = Math.max(5, Math.min(20, height));
  const next = new Array(width * height).fill(FLOOR);
  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      if (inside(level, x, y)) next[y * width + x] = level.cells[idx(level, x, y)];
    }
  }
  level.width = width;
  level.height = height;
  level.cells = next;
  if (!inside(level, level.player.x, level.player.y)) level.player = { x: 1, y: 1 };
  level.instances = level.instances.filter((inst) => inside(level, inst.x, inst.y));
}

function wouldCrop(level, width, height) {
  if (level.player.x >= width || level.player.y >= height) return true;
  return level.instances.some((inst) => inst.x >= width || inst.y >= height);
}

function nextLevelId(levels) {
  let n = 1;
  const used = new Set(levels.map((level) => level.levelId));
  while (used.has(`LV_${String(n).padStart(2, "0")}`)) n += 1;
  return `LV_${String(n).padStart(2, "0")}`;
}

function nextInstanceId(level, def) {
  let n = 0;
  const used = new Set(level.instances.map((inst) => inst.id));
  let id = `${def}_${n}`;
  while (used.has(id)) {
    n += 1;
    id = `${def}_${n}`;
  }
  return id;
}

function currentLevel() {
  return state.levels.find((level) => level.levelId === state.currentId) || null;
}

function activeLevel() {
  return state.mode === "play" && state.play ? state.play.level : currentLevel();
}

function saveStorage() {
  localStorage.setItem(STORAGE_KEY, JSON.stringify({ currentId: state.currentId, levels: state.levels }));
}

function loadStorage() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (!raw) return null;
    const data = JSON.parse(raw);
    if (!Array.isArray(data.levels) || !data.levels.length) return null;
    for (const level of data.levels) delete level.parMoves;
    return data;
  } catch {
    return null;
  }
}

function layoutOf(widthPx, heightPx, level, pad = 28) {
  const fit = Math.floor(Math.min((widthPx - pad * 2) / level.width, (heightPx - pad * 2) / level.height));
  const cell = Math.max(4, fit);
  const mapW = cell * level.width;
  const mapH = cell * level.height + Math.floor(cell * 0.22);
  return {
    cell,
    ox: Math.floor((widthPx - mapW) / 2),
    oy: Math.floor((heightPx - mapH) / 2) + Math.floor(cell * 0.22),
    lift: Math.floor(cell * 0.22),
  };
}

function cellAt(px, py, level, layout) {
  const x = Math.floor((px - layout.ox) / layout.cell);
  const y = Math.floor((py - layout.oy) / layout.cell);
  if (!inside(level, x, y)) return null;
  return { x, y };
}

function roundRect(ctx, x, y, w, h, r) {
  ctx.beginPath();
  ctx.moveTo(x + r, y);
  ctx.arcTo(x + w, y, x + w, y + h, r);
  ctx.arcTo(x + w, y + h, x, y + h, r);
  ctx.arcTo(x, y + h, x, y, r);
  ctx.arcTo(x, y, x + w, y, r);
  ctx.closePath();
}

function drawLevel(ctx, level, layout, hover, highlight, animPos) {
  const { cell, ox, oy, lift } = layout;
  for (let y = 0; y < level.height; y += 1) {
    for (let x = 0; x < level.width; x += 1) {
      const terrain = terrainAt(level, x, y);
      const left = ox + x * cell;
      const top = oy + y * cell;
      if (terrain === EMPTY) {
        ctx.fillStyle = "#100e0c";
        ctx.fillRect(left + 2, top + 2, cell - 4, cell - 4);
        continue;
      }
      if (terrain === WALL) {
        ctx.fillStyle = "#4e3828";
        ctx.fillRect(left + 1, top + 2, cell - 2, cell - 3);
        ctx.fillStyle = "#6b4e38";
        ctx.fillRect(left + 1, top - lift + 2, cell - 2, cell - 4);
        ctx.fillStyle = "#8a674c";
        ctx.fillRect(left + 1, top - lift + 2, cell - 2, 6);
        continue;
      }
      ctx.fillStyle = "#c9a36e";
      ctx.fillRect(left + 1, top + 1, cell - 2, cell - 2);
      ctx.strokeStyle = "rgba(90, 58, 32, 0.28)";
      ctx.strokeRect(left + 1.5, top + 1.5, cell - 3, cell - 3);
    }
  }

  const pos = (inst) => {
    const animated = animPos?.[inst.id];
    return animated || { x: inst.x, y: inst.y };
  };

  for (const inst of level.instances) {
    const def = defOf(inst.def);
    if (!def || def.blocking) continue;
    const p = pos(inst);
    const left = ox + p.x * cell;
    const top = oy + p.y * cell;
    if (def.goal) {
      ctx.strokeStyle = inst.state === "Occupied" ? "#f2d48a" : "#e0b33a";
      ctx.lineWidth = Math.max(2, cell * 0.08);
      ctx.beginPath();
      ctx.arc(left + cell / 2, top + cell / 2, cell * 0.28, 0, Math.PI * 2);
      ctx.stroke();
    } else if (def.pedal) {
      ctx.fillStyle = inst.state === "Pressed" ? "#8fd0c0" : "#3f7d72";
      roundRect(ctx, left + cell * 0.18, top + cell * 0.62, cell * 0.64, cell * 0.16, 3);
      ctx.fill();
    }
  }

  for (const inst of level.instances) {
    const def = defOf(inst.def);
    if (!def?.blocking) continue;
    const p = pos(inst);
    const left = ox + p.x * cell;
    const top = oy + p.y * cell;
    const color = def.slide ? "#6d8f45" : def.return ? "#8d5c86" : "#c56a32";
    ctx.fillStyle = "rgba(0,0,0,0.25)";
    ctx.fillRect(left + cell * 0.18, top + cell * 0.22, cell * 0.64, cell * 0.62);
    ctx.fillStyle = color;
    roundRect(ctx, left + cell * 0.16, top + cell * 0.16, cell * 0.64, cell * 0.64, 4);
    ctx.fill();
  }

  const player = animPos?.player || level.player;
  const px = ox + player.x * cell;
  const py = oy + player.y * cell;
  ctx.fillStyle = "#17384a";
  ctx.beginPath();
  ctx.arc(px + cell / 2, py + cell / 2, cell * 0.26, 0, Math.PI * 2);
  ctx.fill();
  ctx.fillStyle = "#8fd0ea";
  ctx.beginPath();
  ctx.arc(px + cell / 2, py + cell * 0.42, cell * 0.12, 0, Math.PI * 2);
  ctx.fill();

  if (hover && state.mode === "edit") {
    ctx.strokeStyle = "rgba(243, 234, 223, 0.85)";
    ctx.lineWidth = 2;
    ctx.strokeRect(ox + hover.x * cell + 2, oy + hover.y * cell + 2, cell - 4, cell - 4);
  }
  for (const cellPos of highlight) {
    ctx.fillStyle = "rgba(212, 101, 74, 0.35)";
    ctx.fillRect(ox + cellPos.x * cell, oy + cellPos.y * cell, cell, cell);
  }
}

function worldCanvas() {
  return document.getElementById("world");
}

function paintWorld() {
  const canvas = worldCanvas();
  const level = activeLevel();
  if (!canvas || !level) return;
  const rect = canvas.getBoundingClientRect();
  const dpr = window.devicePixelRatio || 1;
  canvas.width = Math.max(1, Math.floor(rect.width * dpr));
  canvas.height = Math.max(1, Math.floor(rect.height * dpr));
  const ctx = canvas.getContext("2d");
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, rect.width, rect.height);
  const layout = layoutOf(rect.width, rect.height, level);
  canvas._layout = layout;
  canvas._view = { width: rect.width, height: rect.height };
  drawLevel(ctx, level, layout, state.hover, state.mode === "edit" ? state.highlight : [], animPositions());
  if (state.mode === "play" && state.play?.won) {
    ctx.fillStyle = "rgba(22, 19, 16, 0.55)";
    ctx.fillRect(0, 0, rect.width, rect.height);
    ctx.fillStyle = "#f3eadf";
    ctx.font = "600 28px Segoe UI, Microsoft YaHei UI, sans-serif";
    ctx.textAlign = "center";
    ctx.fillText("过关", rect.width / 2, rect.height / 2);
    ctx.font = "14px Segoe UI, Microsoft YaHei UI, sans-serif";
    ctx.fillText("不写档", rect.width / 2, rect.height / 2 + 28);
  }
}

function animPositions() {
  const play = state.play;
  if (!play?.anim) return null;
  const t = Math.min(1, (performance.now() - play.anim.t0) / play.anim.dur);
  const ease = t * t * (3 - 2 * t);
  const pos = {};
  for (const motion of play.anim.motions) {
    pos[motion.id] = {
      x: motion.x0 + (motion.x1 - motion.x0) * ease,
      y: motion.y0 + (motion.y1 - motion.y0) * ease,
    };
  }
  for (const inst of play.level.instances) {
    if (!pos[inst.id]) pos[inst.id] = { x: inst.x, y: inst.y };
  }
  if (!pos.player) pos.player = { ...play.level.player };
  return pos;
}

function finishAnim() {
  const play = state.play;
  if (!play?.anim) return;
  play.anim = null;
  if (play.pendingReturns) {
    play.pendingReturns = false;
    const returns = flushReturns(play.level);
    play.won = isWon(play.level);
    if (returns.length) {
      play.anim = { t0: performance.now(), dur: 160, motions: returns };
      requestAnimationFrame(tick);
      return;
    }
  }
  renderChrome();
  paintWorld();
}

function tick() {
  if (!state.play?.anim) return;
  const elapsed = performance.now() - state.play.anim.t0;
  paintWorld();
  if (elapsed >= state.play.anim.dur) finishAnim();
  else requestAnimationFrame(tick);
}

function matchesFilter(level) {
  const q = state.filter.trim().toLowerCase();
  if (!q) return true;
  return `${level.levelId} ${level.displayName}`.toLowerCase().includes(q);
}

function renderList() {
  const root = document.getElementById("level-list");
  root.innerHTML = "";
  for (const level of state.levels.filter(matchesFilter)) {
    const row = document.createElement("button");
    row.type = "button";
    row.className = `outliner-row${level.levelId === state.currentId ? " is-selected" : ""}`;
    row.setAttribute("role", "treeitem");
    row.setAttribute("aria-selected", level.levelId === state.currentId ? "true" : "false");
    row.dataset.listed = level.listed ? "true" : "false";
    const icon = document.createElement("span");
    icon.className = "outliner-icon";
    const name = document.createElement("span");
    name.className = "outliner-name";
    name.textContent = level.displayName || level.levelId;
    const type = document.createElement("span");
    type.className = "outliner-type";
    type.textContent = level.levelId;
    row.append(icon, name, type);
    row.title = `${level.levelId} · ${level.listed ? "Listed" : "Unlisted"}`;
    row.addEventListener("click", () => selectLevel(level.levelId));
    root.append(row);
  }
}

function renderMeta() {
  const level = currentLevel();
  if (!level) return;
  document.getElementById("details-asset").textContent = `ULevelData  ${level.levelId}`;
  document.getElementById("field-id").value = level.levelId;
  document.getElementById("field-name").value = level.displayName;
  document.getElementById("field-w").value = String(level.width);
  document.getElementById("field-h").value = String(level.height);
  document.getElementById("field-note").value = level.designerNote;
  document.getElementById("field-listed").checked = !!level.listed;
  document.getElementById("field-spawn").textContent = `(${level.player.x}, ${level.player.y})`;
  document.getElementById("field-count").textContent = String(level.instances.length);
  const locked = state.mode === "play";
  for (const id of ["field-name", "field-w", "field-h", "field-note", "field-listed", "btn-delete", "btn-new"]) {
    document.getElementById(id).disabled = locked;
  }
  document.getElementById("btn-delete").hidden = !!level.builtin;
}

function folderOf(id) {
  return FOLDERS.find((folder) => folder.id === id) || FOLDERS[0];
}

function assetsInFolder(folderId) {
  const allowed = FOLDER_CHILDREN[folderId] || [folderId];
  const q = state.assetFilter.trim().toLowerCase();
  return ASSETS.filter((asset) => {
    if (!allowed.includes(asset.folder)) return false;
    if (!q) return true;
    return `${asset.name} ${asset.type} ${TOOLS[asset.tool]}`.toLowerCase().includes(q);
  });
}

function drawAssetThumb(canvas, tool) {
  const ctx = canvas.getContext("2d");
  const w = canvas.width;
  const h = canvas.height;
  ctx.fillStyle = "#151515";
  ctx.fillRect(0, 0, w, h);
  for (let y = 0; y < h; y += 8) {
    for (let x = 0; x < w; x += 8) {
      if (((x + y) / 8) % 2 === 0) {
        ctx.fillStyle = "#1c1c1c";
        ctx.fillRect(x, y, 8, 8);
      }
    }
  }
  const cx = w / 2;
  const cy = h / 2;
  if (tool === "floor") {
    ctx.fillStyle = "#c9a36e";
    ctx.fillRect(10, 14, w - 20, h - 24);
    ctx.strokeStyle = "rgba(90, 58, 32, 0.4)";
    ctx.strokeRect(10.5, 14.5, w - 21, h - 25);
  } else if (tool === "wall") {
    ctx.fillStyle = "#4e3828";
    ctx.fillRect(16, 22, w - 32, h - 30);
    ctx.fillStyle = "#8a674c";
    ctx.fillRect(16, 10, w - 32, h - 28);
  } else if (tool === "empty") {
    ctx.fillStyle = "#0b0b0b";
    ctx.fillRect(14, 16, w - 28, h - 28);
  } else if (tool === "eraser") {
    ctx.strokeStyle = "#c75050";
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.moveTo(18, 16);
    ctx.lineTo(w - 18, h - 16);
    ctx.moveTo(w - 18, 16);
    ctx.lineTo(18, h - 16);
    ctx.stroke();
  } else if (tool === "player") {
    ctx.fillStyle = "#17384a";
    ctx.beginPath();
    ctx.arc(cx, cy + 2, 14, 0, Math.PI * 2);
    ctx.fill();
    ctx.fillStyle = "#8fd0ea";
    ctx.beginPath();
    ctx.arc(cx, cy - 4, 6, 0, Math.PI * 2);
    ctx.fill();
  } else if (tool === "Target") {
    ctx.strokeStyle = "#e0b33a";
    ctx.lineWidth = 4;
    ctx.beginPath();
    ctx.arc(cx, cy, 14, 0, Math.PI * 2);
    ctx.stroke();
  } else if (tool === "Pedal") {
    ctx.fillStyle = "#3f7d72";
    ctx.fillRect(16, cy, w - 32, 10);
  } else {
    const color = tool === "Box_Slide" ? "#6d8f45" : tool === "Box_Return" ? "#8d5c86" : "#c56a32";
    ctx.fillStyle = "rgba(0,0,0,0.35)";
    ctx.fillRect(cx - 12, cy - 4, 28, 22);
    ctx.fillStyle = color;
    ctx.fillRect(cx - 14, cy - 14, 28, 28);
  }
}

function renderTools() {
  const folders = document.getElementById("cb-folders");
  folders.innerHTML = "";
  for (const folder of FOLDERS) {
    const row = document.createElement("button");
    row.type = "button";
    row.className = `cb-folder${folder.id === state.folder ? " is-selected" : ""}`;
    row.dataset.folder = folder.id;
    row.dataset.depth = String(folder.depth);
    row.textContent = folder.id;
    folders.append(row);
  }
  document.getElementById("cb-path").textContent = folderOf(state.folder).path;
  const grid = document.getElementById("cb-grid");
  grid.innerHTML = "";
  const assets = assetsInFolder(state.folder);
  for (const asset of assets) {
    const tile = document.createElement("button");
    tile.type = "button";
    tile.className = `cb-tile${asset.tool === state.tool ? " is-selected" : ""}`;
    tile.dataset.tool = asset.tool;
    tile.setAttribute("role", "option");
    tile.setAttribute("aria-selected", asset.tool === state.tool ? "true" : "false");
    const thumb = document.createElement("canvas");
    thumb.className = "cb-thumb";
    thumb.width = 78;
    thumb.height = 62;
    const name = document.createElement("strong");
    name.textContent = asset.name;
    const type = document.createElement("em");
    type.textContent = asset.type;
    tile.append(thumb, name, type);
    tile.title = TOOLS[asset.tool];
    grid.append(tile);
    drawAssetThumb(thumb, asset.tool);
  }
  document.getElementById("cb-count").textContent = `${assets.length} items`;
  document.querySelector(".dock").classList.toggle("is-locked", state.mode === "play");
  document.querySelector(".stage").classList.toggle("is-playing", state.mode === "play");
}

function renderIssues() {
  const root = document.getElementById("issues");
  root.innerHTML = "";
  if (state.status) {
    const ok = document.createElement("span");
    ok.className = "issue ok";
    ok.textContent = state.status;
    root.append(ok);
  }
  for (const issue of state.issues) {
    const button = document.createElement("button");
    button.type = "button";
    button.className = `issue${issue.error ? "" : " warn"}`;
    button.textContent = issue.message;
    button.addEventListener("click", () => {
      state.highlight = issue.cells || [];
      paintWorld();
    });
    root.append(button);
  }
}

function renderChrome() {
  const level = currentLevel();
  const play = state.play;
  document.getElementById("btn-play").disabled = state.mode === "play";
  document.getElementById("btn-stop").disabled = state.mode !== "play";
  document.getElementById("btn-restart").disabled = state.mode !== "play";
  document.getElementById("btn-undo").disabled = state.mode !== "play" || !play || play.undo.length === 0 || !!play.anim;
  document.getElementById("btn-validate").disabled = state.mode === "play";
  const hud = document.getElementById("hud");
  if (state.mode === "play" && play) {
    hud.textContent = "试玩";
  } else if (level) {
    hud.textContent = `${TOOLS[state.tool] || "编辑"} · ${level.levelId}`;
  }
  renderIssues();
}

function refreshIssues() {
  const level = currentLevel();
  state.issues = level ? validate(level, state.levels) : [];
}

function renderAll() {
  renderList();
  renderMeta();
  renderTools();
  renderChrome();
  paintWorld();
}

function selectLevel(id) {
  if (state.mode === "play") stopPlay();
  state.currentId = id;
  state.highlight = [];
  refreshIssues();
  state.status = "左键铺设，右键橡皮。试玩不写档。";
  saveStorage();
  renderAll();
}

function applyTool(x, y, tool) {
  const level = currentLevel();
  if (!level || !inside(level, x, y)) return;
  if (tool === "floor" || tool === "wall" || tool === "empty") {
    const value = tool === "floor" ? FLOOR : tool === "wall" ? WALL : EMPTY;
    level.cells[idx(level, x, y)] = value;
    if (value !== FLOOR) {
      const had = level.instances.some((inst) => inst.x === x && inst.y === y);
      level.instances = level.instances.filter((inst) => inst.x !== x || inst.y !== y);
      if (had || sameCell(level.player, { x, y })) state.status = "这格不能站了，上面的机关已清掉";
    }
    return;
  }
  if (tool === "eraser") {
    level.instances = level.instances.filter((inst) => inst.x !== x || inst.y !== y);
    level.cells[idx(level, x, y)] = FLOOR;
    return;
  }
  if (tool === "player") {
    if (!standable(level, x, y)) {
      state.status = "玩家只能放在地板上";
      return;
    }
    if (blockersAt(level, x, y).length) {
      state.status = "这格已有箱子";
      return;
    }
    level.player = { x, y };
    return;
  }
  const def = defOf(tool);
  if (!def) return;
  if (!standable(level, x, y)) {
    state.status = "机关只能放在地板上";
    return;
  }
  const here = instancesAt(level, x, y);
  const same = here.find((inst) => inst.def === tool);
  if (same) {
    level.instances = level.instances.filter((inst) => inst.id !== same.id);
    return;
  }
  if (def.blocking && here.some((inst) => defOf(inst.def)?.blocking)) {
    state.status = "一格只能放一个箱子";
    return;
  }
  if (def.blocking && sameCell(level.player, { x, y })) {
    state.status = "箱子不能叠在玩家出生点上";
    return;
  }
  level.instances.push({ id: nextInstanceId(level, tool), def: tool, x, y });
}

function onPaint(x, y, tool) {
  if (state.mode === "play") return;
  const key = `${x},${y}`;
  if (state.gesture?.seen.has(key) && tool !== "floor" && tool !== "wall" && tool !== "empty") return;
  state.gesture?.seen.add(key);
  applyTool(x, y, tool);
  refreshIssues();
  saveStorage();
  renderAll();
}

function startPlay() {
  const level = currentLevel();
  refreshIssues();
  const errors = state.issues.filter((issue) => issue.error);
  if (errors.length) {
    state.status = "有错误，不能试玩";
    renderChrome();
    return;
  }
  state.mode = "play";
  state.play = beginPlay(level);
  state.hover = null;
  state.status = "WASD 或方向键移动，Z 撤销，R 重开，Esc 停止";
  renderAll();
}

function stopPlay() {
  state.mode = "edit";
  state.play = null;
  state.status = "已停止试玩，关卡数据还在";
  refreshIssues();
  renderAll();
}

function undoPlay() {
  const play = state.play;
  if (!play || !play.undo.length || play.anim) return;
  restore(play, play.undo.pop());
  renderChrome();
  paintWorld();
}

function restartPlay() {
  const level = currentLevel();
  if (!level || state.mode !== "play") return;
  state.play = beginPlay(level);
  state.status = "已重开";
  renderChrome();
  paintWorld();
}

function toggleListed() {
  const level = currentLevel();
  if (!level || state.mode === "play") return;
  if (level.listed) {
    level.listed = false;
    state.status = "已下架，选关列表里不再算官方关";
  } else {
    refreshIssues();
    if (state.issues.some((issue) => issue.error)) {
      state.status = "有错误，不能上架";
      renderChrome();
      return;
    }
    level.listed = true;
    state.status = "已上架";
  }
  saveStorage();
  renderAll();
}

function queueMove(dx, dy) {
  const play = state.play;
  if (!play || play.anim || play.won) return;
  const result = tryMove(play, dx, dy);
  if (!result) return;
  const returns = play.level.instances.some((inst) => inst.movesUntilReturn === 0);
  play.pendingReturns = returns;
  play.anim = { t0: performance.now(), dur: 160, motions: result.motions };
  renderChrome();
  requestAnimationFrame(tick);
}

function bind() {
  document.getElementById("btn-new").addEventListener("click", () => {
    if (state.mode === "play") return;
    const level = blankLevel(nextLevelId(state.levels));
    state.levels.push(level);
    selectLevel(level.levelId);
  });
  document.getElementById("btn-play").addEventListener("click", startPlay);
  document.getElementById("btn-stop").addEventListener("click", stopPlay);
  document.getElementById("btn-restart").addEventListener("click", restartPlay);
  document.getElementById("btn-undo").addEventListener("click", undoPlay);
  document.getElementById("btn-validate").addEventListener("click", () => {
    refreshIssues();
    state.status = state.issues.some((issue) => issue.error) ? "校验未通过" : "校验通过";
    if (!state.issues.length) state.issues = [];
    renderChrome();
  });
  document.getElementById("level-filter").addEventListener("input", (event) => {
    state.filter = event.target.value;
    renderList();
  });
  document.getElementById("field-listed").addEventListener("change", () => {
    const level = currentLevel();
    if (!level) return;
    if (document.getElementById("field-listed").checked === !!level.listed) return;
    toggleListed();
    document.getElementById("field-listed").checked = !!currentLevel()?.listed;
  });
  document.getElementById("btn-delete").addEventListener("click", () => {
    const level = currentLevel();
    if (!level || level.builtin || state.mode === "play") return;
    state.levels = state.levels.filter((item) => item.levelId !== level.levelId);
    state.currentId = state.levels[0]?.levelId || "";
    if (!state.currentId) {
      const created = blankLevel("LV_01");
      state.levels.push(created);
      state.currentId = created.levelId;
    }
    refreshIssues();
    saveStorage();
    renderAll();
  });

  document.getElementById("cb-folders").addEventListener("click", (event) => {
    const row = event.target.closest(".cb-folder");
    if (!row || state.mode === "play") return;
    state.folder = row.dataset.folder;
    renderTools();
  });
  document.getElementById("cb-grid").addEventListener("click", (event) => {
    const tile = event.target.closest(".cb-tile");
    if (!tile || state.mode === "play") return;
    state.tool = tile.dataset.tool;
    state.status = `当前：${TOOLS[state.tool]}`;
    renderTools();
    renderChrome();
  });
  document.getElementById("asset-filter").addEventListener("input", (event) => {
    state.assetFilter = event.target.value;
    renderTools();
  });

  const name = document.getElementById("field-name");
  name.addEventListener("input", () => {
    const level = currentLevel();
    if (!level) return;
    level.displayName = name.value.trim() || "未命名关卡";
    saveStorage();
    renderList();
    renderChrome();
  });
  const note = document.getElementById("field-note");
  note.addEventListener("input", () => {
    const level = currentLevel();
    if (!level) return;
    level.designerNote = note.value;
    refreshIssues();
    saveStorage();
    renderChrome();
  });

  function onSize(id, key) {
    const input = document.getElementById(id);
    input.addEventListener("change", () => {
      const level = currentLevel();
      if (!level || state.mode === "play") return;
      const value = Math.max(5, Math.min(20, Number(input.value) || level[key]));
      const width = key === "width" ? value : level.width;
      const height = key === "height" ? value : level.height;
      if ((width !== level.width || height !== level.height) && wouldCrop(level, width, height)) {
        const ok = window.confirm("缩小会裁掉玩家或机关，继续吗？");
        if (!ok) {
          input.value = String(level[key]);
          return;
        }
      }
      resizeLevel(level, width, height);
      refreshIssues();
      saveStorage();
      renderAll();
    });
  }
  onSize("field-w", "width");
  onSize("field-h", "height");

  const canvas = worldCanvas();
  canvas.addEventListener("contextmenu", (event) => event.preventDefault());
  canvas.addEventListener("pointerdown", (event) => {
    const level = activeLevel();
    if (!level || state.mode === "play") return;
    const rect = canvas.getBoundingClientRect();
    const cell = cellAt(event.clientX - rect.left, event.clientY - rect.top, level, canvas._layout);
    if (!cell) return;
    const tool = event.button === 2 ? "eraser" : state.tool;
    state.gesture = { tool, seen: new Set() };
    onPaint(cell.x, cell.y, tool);
    canvas.setPointerCapture(event.pointerId);
  });
  canvas.addEventListener("pointermove", (event) => {
    const level = activeLevel();
    if (!level || !canvas._layout) return;
    const rect = canvas.getBoundingClientRect();
    const cell = cellAt(event.clientX - rect.left, event.clientY - rect.top, level, canvas._layout);
    state.hover = cell;
    if (state.gesture && cell && (event.buttons & 3)) onPaint(cell.x, cell.y, state.gesture.tool);
    else paintWorld();
  });
  canvas.addEventListener("pointerup", () => {
    state.gesture = null;
  });
  canvas.addEventListener("pointerleave", () => {
    state.hover = null;
    paintWorld();
  });

  window.addEventListener("keydown", (event) => {
    const tag = event.target?.tagName;
    if (tag === "INPUT" || tag === "TEXTAREA") return;
    if (state.mode !== "play") return;
    const key = event.key.toLowerCase();
    const dir = { w: [0, -1], arrowup: [0, -1], s: [0, 1], arrowdown: [0, 1], a: [-1, 0], arrowleft: [-1, 0], d: [1, 0], arrowright: [1, 0] }[key];
    if (dir) {
      event.preventDefault();
      queueMove(dir[0], dir[1]);
    } else if (key === "z") undoPlay();
    else if (key === "r") restartPlay();
    else if (key === "escape") stopPlay();
  });
  window.addEventListener("resize", paintWorld);
}

function boot() {
  const stored = loadStorage();
  if (stored) {
    state.levels = stored.levels;
    state.currentId = stored.currentId;
  } else {
    state.levels = seedLevels();
    state.currentId = "LV_01";
  }
  if (!currentLevel()) state.currentId = state.levels[0].levelId;
  refreshIssues();
  bind();
  renderAll();
}

if (typeof document !== "undefined") {
  boot();
}

function runSelfTest() {
  const fail = (message) => {
    throw new Error(message);
  };
  const basic = seedLevels()[0];
  let play = beginPlay(basic);
  if (!tryMove(play, 1, 0)) fail("first push");
  flushReturns(play.level);
  const box = play.level.instances.find((inst) => inst.def === "Box_Normal");
  if (box.x !== 4 || play.level.player.x !== 3) fail("push one cell");
  if (!tryMove(play, 1, 0)) fail("second push");
  flushReturns(play.level);
  if (box.x !== 5 || !play.won) fail("should win");

  const blocked = cloneLevel(basic);
  blocked.cells[idx(blocked, 3, 3)] = WALL;
  blocked.instances = blocked.instances.filter((inst) => inst.def !== "Box_Normal");
  play = beginPlay(blocked);
  if (tryMove(play, 1, 0)) fail("wall should block");

  const slide = seedLevels()[2];
  play = beginPlay(slide);
  if (!tryMove(play, 1, 0)) fail("slide push");
  const slider = play.level.instances.find((inst) => inst.def === "Box_Slide");
  if (slider.x !== 6 || play.level.player.x !== 3 || !play.won) fail("slide should stop on target");

  const hole = seedLevels()[1];
  if (standable(hole, 3, 2)) fail("hole must be empty");
  const bad = cloneLevel(basic);
  bad.instances = bad.instances.filter((inst) => inst.def !== "Target");
  const issues = validate(bad, [bad]);
  if (!issues.some((issue) => issue.error && issue.message.includes("胜利条件"))) fail("validate box/goal");
  console.log("editor self-test ok");
}

if (typeof process !== "undefined" && process.argv[1] && process.argv[1].endsWith("editor.js")) {
  runSelfTest();
}
