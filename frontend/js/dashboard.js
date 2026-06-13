// ═══════════════════════════════════════════
// KONFIGURASI
// ═══════════════════════════════════════════
const API   = "http://3.0.3.212";
const WS    = API.replace("http","ws") + "/ws";
const ROOMS = ["room-101","room-102"];
const ROOM_LABELS = { "room-101":"Ruang 101", "room-102":"Ruang 102" };
const ROOM_EMOJI  = { "room-101":"🏫", "room-102":"🏫" };
const ROOM_COLORS = { "room-101":"#dbeafe", "room-102":"#ede9fe" };
const ROOM_TEXT   = { "room-101":"#3b82f6", "room-102":"#8b5cf6" };

// Mapping room → ESP topic
const ROOM_TO_ESP = { "room-101": "esp1", "room-102": "esp2" };

const RELAY_COUNT = 4;
const RELAY_LABELS = {
  "relay-1": "Lampu Utama",
  "relay-2": "AC",
  "relay-3": "Proyektor",
  "relay-4": "Kipas"
};

// ═══════════════════════════════════════════
// STATE
// ═══════════════════════════════════════════
const CHART_COLORS = { "room-101": "#3b82f6", "room-102": "#8b5cf6" };
const MAX_PTS = 40;

const state = {};
ROOMS.forEach(id => {
  state[id] = { volt:0, ampere:0, watt:0, person_count:0, is_occupied:false, mode:"manual", relays:{} };
  for (let i = 1; i <= RELAY_COUNT; i++) state[id].relays[`relay-${i}`] = false;
});

const energyBuf = {}, personBuf = {};
ROOMS.forEach(id => {
  energyBuf[id] = Array(MAX_PTS).fill(0);
  personBuf[id] = Array(MAX_PTS).fill(0);
});

let energyMetric = "watt";
let energyChart, personChart;
const relayLogs = {};
ROOMS.forEach(id => { relayLogs[id] = []; });

// ═══════════════════════════════════════════
// STARS HELPER
// ═══════════════════════════════════════════
function calcOccupancyRating(personCount, watt) {
  if (personCount === 0) return 0;
  const efficiency = personCount / Math.max(1, watt / 100);
  return Math.min(5, Math.max(1, (efficiency * 2).toFixed(1)));
}

function renderStars(rating) {
  const full  = Math.floor(rating);
  const half  = rating % 1 >= 0.5 ? 1 : 0;
  const empty = 5 - full - half;
  let html = '<div class="stars">';
  for (let i = 0; i < full;  i++) html += '<span class="star full">★</span>';
  if (half)                        html += '<span class="star half">★</span>';
  for (let i = 0; i < empty; i++) html += '<span class="star empty">★</span>';
  html += '</div>';
  return html;
}

// ═══════════════════════════════════════════
// BUILD ROOM CARDS
// ═══════════════════════════════════════════
function buildRoomCards() {
  const grid = document.getElementById("roomsGrid");
  grid.innerHTML = "";

  ROOMS.forEach(roomId => {
    const label     = ROOM_LABELS[roomId];
    const bgColor   = ROOM_COLORS[roomId];
    const textColor = ROOM_TEXT[roomId];

    let relayHTML = `
      <div class="relay-header">
        <div class="relay-section-lbl">Kontrol Perangkat</div>
        <div class="mode-switch">
          <button class="mode-btn active" id="btn-manual-${roomId}" onclick="setMode('${roomId}', 'manual')">Manual</button>
          <button class="mode-btn" id="btn-auto-${roomId}" onclick="setMode('${roomId}', 'auto')">Auto</button>
        </div>
      </div>
      <div class="relay-grid">`;

    for (let i = 1; i <= RELAY_COUNT; i++) {
      const rid = `relay-${i}`;
      relayHTML += `
        <button class="relay-btn off" id="rbtn-${roomId}-${rid}" onclick="sendRelay('${roomId}','${rid}')">
          <div class="relay-btn-left">
            <span class="relay-btn-name">${rid.toUpperCase()}</span>
            <span class="relay-btn-lbl">${RELAY_LABELS[rid]}</span>
          </div>
          <div class="relay-toggle" id="rtoggle-${roomId}-${rid}"></div>
        </button>`;
    }
    relayHTML += "</div>";

    const card = document.createElement("div");
    card.className = "room-card";
    card.id = `card-${roomId}`;
    card.dataset.occ = "false";
    card.innerHTML = `
      <div class="room-hdr">
        <div class="room-hdr-l">
          <div class="room-avatar" style="background:${bgColor};color:${textColor}">
            ${ROOM_EMOJI[roomId]}
          </div>
          <div class="room-info">
            <div class="room-name">${label}</div>
            <div class="room-loc">
              <svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 10c0 7-9 13-9 13s-9-6-9-13a9 9 0 0 1 18 0z"/><circle cx="12" cy="10" r="3"/></svg>
              Gedung Kuliah
            </div>
          </div>
        </div>
        <span class="occ-badge emp" id="occ-${roomId}">Kosong</span>
      </div>

      <div class="stars-row" id="starsrow-${roomId}">
        ${renderStars(0)}
        <span class="rating-num" id="ratingnum-${roomId}">—</span>
        <span class="rating-count" id="ratingcount-${roomId}"></span>
      </div>

      <div class="prow">
        <div class="pnum-wrap">
          <span class="pnum" id="pnum-${roomId}">0</span>
          <span class="plbl">orang terdeteksi</span>
        </div>
        <div class="pbar">
          <div class="pbar-fill" id="pbar-${roomId}" style="width:0%;background:${textColor}"></div>
        </div>
      </div>

      <div class="sg">
        <div class="sb"><div class="sv" id="vvolt-${roomId}">—<span class="su">V</span></div><div class="sl">Tegangan</div></div>
        <div class="sb"><div class="sv" id="vamp-${roomId}">—<span class="su">A</span></div><div class="sl">Arus</div></div>
        <div class="sb"><div class="sv" id="vwatt-${roomId}">—<span class="su">W</span></div><div class="sl">Daya</div></div>
      </div>

      ${relayHTML}`;
    grid.appendChild(card);
  });
}

// ═══════════════════════════════════════════
// MODE SWITCHER (MANUAL / AUTO)
// ═══════════════════════════════════════════
async function setMode(roomId, mode) {
  state[roomId].mode = mode;

  // Kirim mode ke FastAPI → ESP
  try {
    await fetch(`${API}/api/mode/${roomId}`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ mode: mode })
    });
    console.log(`[MODE] ${roomId} → ${mode}`);
  } catch(e) { console.warn("[MODE] Gagal kirim:", e); }

  document.getElementById(`btn-manual-${roomId}`).classList.toggle('active', mode === 'manual');
  document.getElementById(`btn-auto-${roomId}`).classList.toggle('active', mode === 'auto');

  const isAuto = (mode === 'auto');
  for (let i = 1; i <= RELAY_COUNT; i++) {
    const btn = document.getElementById(`rbtn-${roomId}-relay-${i}`);
    if (btn) {
      btn.disabled = isAuto;
      btn.style.opacity = isAuto ? "0.6" : "1";
      btn.style.cursor  = isAuto ? "not-allowed" : "pointer";
    }
  }
}

// ═══════════════════════════════════════════
// BUILD LOG PANELS
// ═══════════════════════════════════════════
function buildLogPanels() {
  const grid = document.getElementById("logGrid");
  grid.innerHTML = "";
  ROOMS.forEach(roomId => {
    const div = document.createElement("div");
    div.className = "log-card";
    div.innerHTML = `
      <div class="log-hdr">
        <div class="log-ttl">Relay Log — ${ROOM_LABELS[roomId]}</div>
        <span class="log-count" id="logcount-${roomId}">0 entri</span>
      </div>
      <div class="log-list" id="loglist-${roomId}">
        <div class="log-empty">Belum ada perubahan relay</div>
      </div>`;
    grid.appendChild(div);
  });
}

// ═══════════════════════════════════════════
// UPDATE: SENSOR
// ═══════════════════════════════════════════
function updateSensorUI(roomId, data) {
  // Mapping field ESP → frontend
  const volt   = data.volt   ?? data.voltage ?? 0;
  const ampere = data.ampere ?? 0;
  const watt   = data.watt   ?? data.power   ?? 0;

  Object.assign(state[roomId], { volt, ampere, watt });

  setVal(`vvolt-${roomId}`, volt.toFixed(1),   "V");
  setVal(`vamp-${roomId}`,  ampere.toFixed(2), "A");
  setVal(`vwatt-${roomId}`, Math.round(watt),  "W");

  updateStars(roomId);

  const v = { watt, volt, ampere }[energyMetric] ?? 0;
  pushChart(energyChart, ROOMS.indexOf(roomId), parseFloat(v.toFixed(2)));
  updateFooter();
}

function updateStars(roomId) {
  const s = state[roomId];
  if (s.person_count === 0) return;
  const rating = parseFloat(calcOccupancyRating(s.person_count, s.watt));
  const row = document.getElementById(`starsrow-${roomId}`);
  const num = document.getElementById(`ratingnum-${roomId}`);
  const cnt = document.getElementById(`ratingcount-${roomId}`);
  if (row) {
    const starsEl = row.querySelector(".stars");
    if (starsEl) starsEl.outerHTML = renderStars(rating).match(/<div class="stars">.*<\/div>/s)?.[0] || "";
  }
  if (num) num.textContent = rating.toFixed(1);
  if (cnt) cnt.textContent = `${s.person_count} orang`;
}

// ═══════════════════════════════════════════
// UPDATE: OCCUPANCY (dari students ESP)
// ═══════════════════════════════════════════
function updateOccupancyUI(roomId, data) {
  // Mapping: students → person_count
  const personCount = data.person_count ?? data.students ?? 0;
  const isOccupied  = personCount >= 5;

  state[roomId].person_count = personCount;
  state[roomId].is_occupied  = isOccupied;

  const occStr = isOccupied ? "Terisi" : "Kosong";
  const occCls = isOccupied ? "occ"    : "emp";

  const card = document.getElementById(`card-${roomId}`);
  const occ  = document.getElementById(`occ-${roomId}`);
  const pnum = document.getElementById(`pnum-${roomId}`);
  const pbar = document.getElementById(`pbar-${roomId}`);

  if (card) card.dataset.occ = isOccupied ? "true" : "false";
  if (occ)  { occ.textContent = occStr; occ.className = `occ-badge ${occCls}`; }
  if (pnum) pnum.textContent  = personCount;
  if (pbar) pbar.style.width  = Math.min(100, (personCount / 10) * 100) + "%";

  updateStars(roomId);
  pushChart(personChart, ROOMS.indexOf(roomId), personCount);
  updateFooter();
}

// ═══════════════════════════════════════════
// UPDATE: RELAY
// ═══════════════════════════════════════════
function updateRelayUI(roomId, relayId, status, logEntry) {
  state[roomId].relays[relayId] = status;
  const btn = document.getElementById(`rbtn-${roomId}-${relayId}`);
  if (btn) {
    btn.className = "relay-btn " + (status ? "on" : "off");
    btn.querySelector(".relay-btn-name").textContent = relayId.toUpperCase();
  }
  // Hanya tambah log jika ada logEntry (berarti aksi manual)
  if (logEntry) addLog(roomId, logEntry);
}

// ═══════════════════════════════════════════
// LOG — Hanya Manual On/Off
// ═══════════════════════════════════════════
function addLog(roomId, entry) {
  relayLogs[roomId].unshift(entry);
  if (relayLogs[roomId].length > 50) relayLogs[roomId].pop();
  renderLog(roomId);
}

function renderLog(roomId) {
  const list  = document.getElementById(`loglist-${roomId}`);
  const count = document.getElementById(`logcount-${roomId}`);
  const logs  = relayLogs[roomId];
  if (!list) return;
  count.textContent = `${logs.length} entri`;
  if (!logs.length) {
    list.innerHTML = '<div class="log-empty">Belum ada perubahan relay</div>';
    return;
  }
  list.innerHTML = logs.map(l => {
    const cls = l.status ? "on" : "off";
    return `<div class="log-item ${cls}">
      <span class="log-rid">${l.relay_id.toUpperCase()}</span>
      <span class="log-lbl">${RELAY_LABELS[l.relay_id] || l.relay_id}</span>
      <span class="log-st ${cls}">${l.status ? "ON" : "OFF"}</span>
      <span class="log-src ${l.source || 'manual'}">${(l.source || 'manual') === 'auto' ? 'AUTO' : 'MANUAL'}</span>
      <span class="log-time">${fmtTime(l.changed_at)}</span>
    </div>`;
  }).join("");
}

function fmtTime(ts) {
  if (!ts) return "—";
  const d = new Date(ts);
  if (isNaN(d)) return ts;
  return d.toLocaleString("id-ID", { day:"2-digit", month:"2-digit", hour:"2-digit", minute:"2-digit", second:"2-digit" });
}

// ═══════════════════════════════════════════
// SEND RELAY (Manual only + log waktu)
// ═══════════════════════════════════════════
async function sendRelay(roomId, relayId) {
  if (state[roomId].mode === 'auto') {
    showToast("Ruangan dalam mode Auto. Ubah ke Manual untuk mengontrol.", true);
    return;
  }

  const btn = document.getElementById(`rbtn-${roomId}-${relayId}`);
  if (!btn || btn.disabled) return;
  const newStatus = !state[roomId].relays[relayId];
  btn.disabled = true;

  try {
    const res = await fetch(`${API}/api/relay/${roomId}/${relayId}`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ status: newStatus })
    });
    if (res.ok) {
      // Tambah log dengan timestamp sekarang
      const logEntry = {
        relay_id:   relayId,
        status:     newStatus,
        changed_at: new Date().toISOString()
      };
      updateRelayUI(roomId, relayId, newStatus, logEntry);
      showToast(`${ROOM_LABELS[roomId]} — ${RELAY_LABELS[relayId]} ${newStatus ? "dinyalakan ✓" : "dimatikan ✗"}`);
    } else {
      showToast("Gagal mengirim perintah", true);
    }
  } catch {
    showToast("Tidak bisa terhubung ke server", true);
  } finally {
    btn.disabled = false;
  }
}

// ═══════════════════════════════════════════
// SEARCH / FILTER
// ═══════════════════════════════════════════
function filterRooms(query) {
  const q = query.toLowerCase();
  ROOMS.forEach(roomId => {
    const card = document.getElementById(`card-${roomId}`);
    if (!card) return;
    const label = ROOM_LABELS[roomId].toLowerCase();
    card.style.display = label.includes(q) ? "" : "none";
  });
}

// ═══════════════════════════════════════════
// LOAD INITIAL DATA
// ═══════════════════════════════════════════
async function loadInitialData() {
  try {
    const res = await fetch(`${API}/api/dashboard/overview`);
    if (res.ok) {
      const json = await res.json();
      json.rooms.forEach(room => {
        if (!ROOMS.includes(room.room_id)) return;
        if (room.sensor)    updateSensorUI(room.room_id, room.sensor);
        if (room.occupancy) updateOccupancyUI(room.room_id, room.occupancy);
        if (room.relays)    room.relays.forEach(r => updateRelayUI(room.room_id, r.relay_id, r.status));
      });
    }
  } catch (e) { console.warn("[SCEMS] Gagal load data awal:", e.message); }

  for (const roomId of ROOMS) {
    try {
      const res = await fetch(`${API}/api/relay/${roomId}/log?limit=30`);
      if (!res.ok) continue;
      const json = await res.json();
      relayLogs[roomId] = json.logs || [];
      renderLog(roomId);
    } catch { /* skip */ }
  }
}

// ═══════════════════════════════════════════
// WEBSOCKET — Realtime dari FastAPI
// ═══════════════════════════════════════════
let ws = null, wsTimer = null;

function connectWS() {
  clearTimeout(wsTimer);
  const dot    = document.getElementById("wsDot");
  const status = document.getElementById("wsStatus");
  try {
    ws = new WebSocket(WS);
    ws.onopen = () => {
      if (dot)    dot.className = "ws-dot on";
      if (status) status.textContent = "Online";
    };
    ws.onmessage = (ev) => {
      try {
        const msg = JSON.parse(ev.data);
        // Abaikan pesan ping keepalive dari server
        if (msg.type === "ping") return;
        const { type, room_id, data } = msg;
        if (!ROOMS.includes(room_id)) return;
        if (type === "sensor")    updateSensorUI(room_id, data);
        if (type === "occupancy") updateOccupancyUI(room_id, data);
        if (type === "relay") {
          const logEntry = {
            relay_id  : data.relay_id,
            status    : data.status,
            source    : data.source || "auto",
            changed_at: new Date().toISOString()
          };
          updateRelayUI(room_id, data.relay_id, data.status, logEntry);
        }
      } catch { /* skip */ }
    };
    ws.onclose = () => {
      if (dot)    dot.className = "ws-dot off";
      if (status) status.textContent = "Offline";
      wsTimer = setTimeout(connectWS, 5000);
    };
    ws.onerror = () => ws.close();
  } catch { wsTimer = setTimeout(connectWS, 5000); }
}

// ═══════════════════════════════════════════
// CHARTS
// ═══════════════════════════════════════════
const EMPTY_LABELS = Array(MAX_PTS).fill("");

const chartBaseOpts = {
  responsive: true, maintainAspectRatio: false, animation: false,
  plugins: { legend: { display: false } },
  scales: {
    x: { display: false },
    y: {
      grid:   { color: "rgba(0,0,0,0.04)" },
      ticks:  { color: "#9ca3af", font: { size: 11, family: "Inter" } },
      border: { color: "rgba(0,0,0,0.06)" }
    }
  }
};

function buildCharts() {
  energyChart = new Chart(document.getElementById("energyChart").getContext("2d"), {
    type: "line",
    data: {
      labels: EMPTY_LABELS,
      datasets: ROOMS.map(id => ({
        data: [...energyBuf[id]],
        borderColor: CHART_COLORS[id],
        backgroundColor: CHART_COLORS[id] + "15",
        borderWidth: 2, pointRadius: 0, tension: 0.4, fill: true
      }))
    },
    options: chartBaseOpts
  });

  personChart = new Chart(document.getElementById("personChart").getContext("2d"), {
    type: "line",
    data: {
      labels: EMPTY_LABELS,
      datasets: [
        ...ROOMS.map(id => ({
          data: [...personBuf[id]],
          borderColor: CHART_COLORS[id],
          backgroundColor: CHART_COLORS[id] + "15",
          borderWidth: 2, pointRadius: 0, tension: 0.4, fill: true
        })),
        {
          data: Array(MAX_PTS).fill(5),
          borderColor: "rgba(239,68,68,0.5)",
          borderWidth: 1.5, borderDash: [5,5],
          pointRadius: 0, fill: false, tension: 0
        }
      ]
    },
    options: { ...chartBaseOpts, scales: { ...chartBaseOpts.scales, y: { ...chartBaseOpts.scales.y, min: 0 } } }
  });
}

function pushChart(chart, idx, val) {
  if (!chart) return;
  chart.data.datasets[idx].data.push(val);
  if (chart.data.datasets[idx].data.length > MAX_PTS) chart.data.datasets[idx].data.shift();
  chart.update("none");
}

function switchEnergy(metric, btn) {
  energyMetric = metric;
  document.querySelectorAll(".tab-btn").forEach(b => b.classList.remove("active"));
  btn.classList.add("active");
  ROOMS.forEach((id, i) => {
    const v = state[id][metric] || 0;
    energyChart.data.datasets[i].data = Array(MAX_PTS).fill(parseFloat(v.toFixed(2)));
  });
  energyChart.update();
}

// ═══════════════════════════════════════════
// FOOTER & HELPERS
// ═══════════════════════════════════════════
function updateFooter() {
  let wt=0, occ=0, prs=0;
  ROOMS.forEach(id => {
    wt  += state[id].watt;
    prs += state[id].person_count;
    if (state[id].is_occupied) occ++;
  });
  set("totalWatt",    Math.round(wt));
  set("occRooms",     occ);
  set("totalPersons", prs);
}

function set(id, v)     { const e = document.getElementById(id); if(e) e.textContent = v; }
function setVal(id,v,u) { const e = document.getElementById(id); if(e) e.innerHTML = `${v}<span class="su">${u}</span>`; }

let toastT = null;
function showToast(msg, err=false) {
  const t = document.getElementById("toast");
  t.textContent = msg;
  t.className = err ? "err" : "";
  t.style.display = "block";
  clearTimeout(toastT);
  toastT = setTimeout(() => t.style.display = "none", 3000);
}

setInterval(() => {
  document.getElementById("clock").textContent = new Date().toLocaleTimeString("id-ID");
}, 1000);

// ═══════════════════════════════════════════
// INIT
// ═══════════════════════════════════════════
buildRoomCards();
buildLogPanels();
buildCharts();
loadInitialData();
connectWS();
document.getElementById("clock").textContent = new Date().toLocaleTimeString("id-ID");
