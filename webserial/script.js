// TMF8828 WebSerial Visualization
// Parses structured frames from the sensor and displays distance grids

let port = null;
let reader = null;
let writer = null;
let keepReading = false;
let lineBuffer = '';

let currentFrame = {
  mode: '8x8',
  sub: 0,
  temp: 0,
  distances: [],
  confidences: []
};
let inFrame = false;

const subcaptures = [
  { distances: new Array(36).fill(0), confidences: new Array(36).fill(0), seen: false },
  { distances: new Array(36).fill(0), confidences: new Array(36).fill(0), seen: false },
  { distances: new Array(36).fill(0), confidences: new Array(36).fill(0), seen: false },
  { distances: new Array(36).fill(0), confidences: new Array(36).fill(0), seen: false }
];

const connectBtn = document.getElementById('connect-btn');
const disconnectBtn = document.getElementById('disconnect-btn');
const modeSelect = document.getElementById('mode-select');
const periodSelect = document.getElementById('period-select');
const kiterSelect = document.getElementById('kiter-select');
const statusDot = document.getElementById('status-dot');
const statusText = document.getElementById('status-text');
const currentModeDisplay = document.getElementById('current-mode');
const currentPeriodDisplay = document.getElementById('current-period');
const currentTempDisplay = document.getElementById('current-temp');
const distanceGrid = document.getElementById('distance-grid');
const serialLog = document.getElementById('serial-log');
const logToggle = document.getElementById('log-toggle');

const rowOffset = [0, 0, 2, 2];
const colOffset = [0, 2, 0, 2];

// Initialize
document.addEventListener('DOMContentLoaded', () => {
  if (!('serial' in navigator)) {
    statusText.textContent = 'Web Serial not supported (use Chrome/Edge)';
    connectBtn.disabled = true;
    return;
  }

  connectBtn.addEventListener('click', connect);
  disconnectBtn.addEventListener('click', disconnect);
  modeSelect.addEventListener('change', () => sendCommand(`MODE:${modeSelect.value}`));
  periodSelect.addEventListener('change', () => sendCommand(`PERIOD:${periodSelect.value}`));
  kiterSelect.addEventListener('change', () => sendCommand(`KITER:${kiterSelect.value}`));
  logToggle.addEventListener('click', toggleLog);

  initializeGrid(8);
});

function initializeGrid(size) {
  distanceGrid.innerHTML = '';
  distanceGrid.style.gridTemplateColumns = `repeat(${size}, 1fr)`;
  for (let i = 0; i < size * size; i++) {
    const cell = document.createElement('div');
    cell.className = 'grid-cell';
    cell.id = `cell-${i}`;
    cell.textContent = '--';
    cell.style.backgroundColor = '#4a4a6a';
    distanceGrid.appendChild(cell);
  }
}

async function connect() {
  try {
    port = await navigator.serial.requestPort();
    await port.open({ baudRate: 115200 });

    reader = port.readable.getReader();
    writer = port.writable.getWriter();

    keepReading = true;
    readLoop();

    connectBtn.disabled = true;
    disconnectBtn.disabled = false;
    modeSelect.disabled = false;
    periodSelect.disabled = false;
    kiterSelect.disabled = false;
    statusDot.classList.add('connected');
    statusText.textContent = 'Connected';
    addLog('Connected to device', 'status');
  } catch (error) {
    console.error('Connection error:', error);
    addLog(`Connection failed: ${error.message}`, 'error');
    statusText.textContent = 'Connection failed';
  }
}

async function disconnect() {
  keepReading = false;

  if (reader) {
    try {
      await reader.cancel();
      await reader.releaseLock();
    } catch (e) {
      /* ignore */
    }
    reader = null;
  }

  if (writer) {
    try {
      await writer.close();
    } catch (e) {
      /* ignore */
    }
    writer = null;
  }

  if (port) {
    try {
      await port.close();
    } catch (e) {
      /* ignore */
    }
    port = null;
  }

  connectBtn.disabled = false;
  disconnectBtn.disabled = true;
  modeSelect.disabled = true;
  periodSelect.disabled = true;
  kiterSelect.disabled = true;
  statusDot.classList.remove('connected');
  statusText.textContent = 'Disconnected';
  currentModeDisplay.textContent = '--';
  currentPeriodDisplay.textContent = '--';
  currentTempDisplay.textContent = '--';
  addLog('Disconnected', 'status');
}

async function readLoop() {
  const decoder = new TextDecoder();

  while (port && keepReading) {
    try {
      const { value, done } = await reader.read();
      if (done) break;
      const text = decoder.decode(value);
      processData(text);
    } catch (error) {
      if (keepReading) {
        console.error('Read error:', error);
        addLog(`Read error: ${error.message}`, 'error');
      }
      break;
    }
  }

  if (keepReading) {
    disconnect();
  }
}

function processData(data) {
  lineBuffer += data;

  let newlineIdx;
  while ((newlineIdx = lineBuffer.indexOf('\n')) !== -1) {
    const line = lineBuffer.substring(0, newlineIdx).trim();
    lineBuffer = lineBuffer.substring(newlineIdx + 1);

    if (line) {
      parseLine(line);
    }
  }
}

function parseLine(line) {
  if (!line.startsWith('D:') && !line.startsWith('C:')) {
    addLog(line);
  }

  if (line === 'FRAME_START') {
    inFrame = true;
    currentFrame = {
      mode: '8x8',
      sub: 0,
      temp: 0,
      distances: [],
      confidences: []
    };
    return;
  }

  if (line === 'FRAME_END') {
    inFrame = false;
    handleFrame();
    return;
  }

  if (!inFrame) {
    handleCommandResponse(line);
    return;
  }

  if (line.startsWith('MODE:')) {
    currentFrame.mode = line.substring(5).trim();
  } else if (line.startsWith('SUB:')) {
    currentFrame.sub = parseInt(line.substring(4), 10) || 0;
  } else if (line.startsWith('TEMP:')) {
    currentFrame.temp = parseInt(line.substring(5), 10) || 0;
  } else if (line.startsWith('D:')) {
    currentFrame.distances = line.substring(2).split(',').map(v => parseInt(v, 10));
  } else if (line.startsWith('C:')) {
    currentFrame.confidences = line.substring(2).split(',').map(v => parseInt(v, 10));
  }
}

function handleCommandResponse(line) {
  if (line.startsWith('OK MODE:')) {
    const mode = line.substring(8).trim();
    modeSelect.value = mode;
    currentModeDisplay.textContent = (mode === '8X8') ? '8×8' : 'Legacy';
    addLog(line, 'status');
  } else if (line.startsWith('OK PERIOD:')) {
    const period = parseInt(line.substring(10), 10);
    periodSelect.value = String(period);
    currentPeriodDisplay.textContent = `${period} ms`;
    addLog(line, 'status');
  } else if (line.startsWith('OK KITER:')) {
    const kiter = parseInt(line.substring(9), 10);
    kiterSelect.value = String(kiter);
    addLog(line, 'status');
  }
}

function handleFrame() {
  const mode = currentFrame.mode.toUpperCase();
  const temp = currentFrame.temp;
  currentTempDisplay.textContent = `${temp}°C`;

  if (mode === '8X8') {
    updateSubcapture(currentFrame.sub, currentFrame.distances, currentFrame.confidences);
    if (subcaptures.every(entry => entry.seen)) {
      const composite = compositeSubcaptures();
      renderGrid(composite.distances, composite.confidences, 8);
      resetSubcaptures();
    }
    currentModeDisplay.textContent = '8×8';
  } else {
    renderGrid(currentFrame.distances, currentFrame.confidences, 6);
    resetSubcaptures();
    currentModeDisplay.textContent = 'Legacy';
  }

  currentPeriodDisplay.textContent = `${periodSelect.value} ms`;
}

function updateSubcapture(sub, distances, confidences) {
  const idx = sub & 0x03;
  subcaptures[idx].distances = distances.slice(0, 36);
  subcaptures[idx].confidences = confidences.slice(0, 36);
  subcaptures[idx].seen = true;
}

function resetSubcaptures() {
  for (const entry of subcaptures) {
    entry.seen = false;
  }
}

function compositeSubcaptures() {
  const compositeDistances = new Array(64).fill(0);
  const compositeConfidences = new Array(64).fill(0);

  for (let s = 0; s < 4; s++) {
    const ro = rowOffset[s];
    const co = colOffset[s];
    const distances = subcaptures[s].distances;
    const confidences = subcaptures[s].confidences;

    for (let row = 0; row < 6; row++) {
      for (let col = 0; col < 6; col++) {
        const idx6 = row * 6 + col;
        const r = row + ro;
        const c = col + co;
        if (r >= 8 || c >= 8) {
          continue;
        }
        const idx8 = r * 8 + c;
        const conf = confidences[idx6] || 0;
        if (conf >= (compositeConfidences[idx8] || 0)) {
          compositeConfidences[idx8] = conf;
          compositeDistances[idx8] = distances[idx6] || 0;
        }
      }
    }
  }

  return { distances: compositeDistances, confidences: compositeConfidences };
}

function renderGrid(distances, confidences, size) {
  if (distanceGrid.children.length !== size * size) {
    initializeGrid(size);
  }

  for (let i = 0; i < size * size; i++) {
    const cell = document.getElementById(`cell-${i}`);
    if (!cell) continue;

    const dist = distances[i] || 0;
    const conf = confidences[i] || 0;
    if (conf > 0 && dist > 0 && dist < 4000) {
      cell.textContent = dist;
      cell.style.backgroundColor = distanceToColor(dist);
    } else {
      cell.textContent = '--';
      cell.style.backgroundColor = '#4a4a6a';
    }
  }
}

function distanceToColor(dist) {
  const maxDist = 2000;
  const clamped = Math.min(Math.max(dist, 0), maxDist);
  const hue = (clamped / maxDist) * 260;
  const saturation = 70 + (clamped / maxDist) * 10;
  const lightness = 45 + (clamped / maxDist) * 10;
  return `hsl(${hue}, ${saturation}%, ${lightness}%)`;
}

async function sendCommand(cmd) {
  if (!writer) return;

  try {
    const encoder = new TextEncoder();
    await writer.write(encoder.encode(cmd + '\n'));
    addLog(`Sent: ${cmd}`, 'command');
  } catch (error) {
    console.error('Send error:', error);
    addLog(`Send failed: ${error.message}`, 'error');
  }
}

function addLog(msg, type = 'data') {
  const entry = document.createElement('div');
  entry.className = `log-entry ${type}`;
  entry.textContent = msg;
  serialLog.appendChild(entry);

  while (serialLog.children.length > 500) {
    serialLog.removeChild(serialLog.firstChild);
  }

  if (serialLog.classList.contains('expanded')) {
    serialLog.scrollTop = serialLog.scrollHeight;
  }
}

function toggleLog() {
  serialLog.classList.toggle('expanded');
  const toggleText = logToggle.querySelector('.log-toggle');
  if (serialLog.classList.contains('expanded')) {
    toggleText.textContent = '▲ Click to collapse';
    serialLog.scrollTop = serialLog.scrollHeight;
  } else {
    toggleText.textContent = '▼ Click to expand';
  }
}
