// TMF8828 WebSerial Visualization
// Receives pre-assembled 8x8 frames from the sensor and displays them

let port = null;
let reader = null;
let writer = null;
let keepReading = false;
let lineBuffer = '';

let inFrame = false;
let frameTemp = 0;
const frameDistances = new Array(64).fill(0);
const frameConfidences = new Array(64).fill(0);

const connectBtn = document.getElementById('connect-btn');
const disconnectBtn = document.getElementById('disconnect-btn');
const periodSelect = document.getElementById('period-select');
const kiterSelect = document.getElementById('kiter-select');
const statusDot = document.getElementById('status-dot');
const statusText = document.getElementById('status-text');
const currentPeriodDisplay = document.getElementById('current-period');
const currentTempDisplay = document.getElementById('current-temp');
const distanceGrid = document.getElementById('distance-grid');
const serialLog = document.getElementById('serial-log');
const logToggle = document.getElementById('log-toggle');

document.addEventListener('DOMContentLoaded', () => {
  if (!('serial' in navigator)) {
    statusText.textContent = 'Web Serial not supported (use Chrome/Edge)';
    connectBtn.disabled = true;
    return;
  }

  connectBtn.addEventListener('click', connect);
  disconnectBtn.addEventListener('click', disconnect);
  periodSelect.addEventListener('change', () => sendCommand(`PERIOD:${periodSelect.value}`));
  kiterSelect.addEventListener('change', () => sendCommand(`KITER:${kiterSelect.value}`));
  logToggle.addEventListener('click', toggleLog);

  initializeGrid();
});

function initializeGrid() {
  distanceGrid.innerHTML = '';
  distanceGrid.style.gridTemplateColumns = 'repeat(8, 1fr)';
  for (let i = 0; i < 64; i++) {
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
    try { await reader.cancel(); await reader.releaseLock(); } catch (e) { /* ignore */ }
    reader = null;
  }
  if (writer) {
    try { await writer.close(); } catch (e) { /* ignore */ }
    writer = null;
  }
  if (port) {
    try { await port.close(); } catch (e) { /* ignore */ }
    port = null;
  }

  connectBtn.disabled = false;
  disconnectBtn.disabled = true;
  periodSelect.disabled = true;
  kiterSelect.disabled = true;
  statusDot.classList.remove('connected');
  statusText.textContent = 'Disconnected';
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
      processData(decoder.decode(value));
    } catch (error) {
      if (keepReading) {
        console.error('Read error:', error);
        addLog(`Read error: ${error.message}`, 'error');
      }
      break;
    }
  }
  if (keepReading) disconnect();
}

function processData(data) {
  lineBuffer += data;
  let idx;
  while ((idx = lineBuffer.indexOf('\n')) !== -1) {
    const line = lineBuffer.substring(0, idx).trim();
    lineBuffer = lineBuffer.substring(idx + 1);
    if (line) parseLine(line);
  }
}

function parseLine(line) {
  if (!line.startsWith('R')) {
    addLog(line);
  }

  if (line === 'FRAME_START') {
    inFrame = true;
    return;
  }

  if (line === 'FRAME_END') {
    inFrame = false;
    renderGrid();
    return;
  }

  if (!inFrame) {
    handleCommandResponse(line);
    return;
  }

  if (line.startsWith('TEMP:')) {
    frameTemp = parseInt(line.substring(5), 10) || 0;
  } else if (line.startsWith('R') && line.indexOf(':') > 0) {
    // Format: R0:d,d,d,d,d,d,d,d|c,c,c,c,c,c,c,c
    const rowNum = parseInt(line.substring(1, line.indexOf(':')), 10);
    if (rowNum < 0 || rowNum > 7) return;
    const payload = line.substring(line.indexOf(':') + 1);
    const parts = payload.split('|');
    const dists = parts[0].split(',').map(v => parseInt(v, 10));
    const confs = parts.length > 1 ? parts[1].split(',').map(v => parseInt(v, 10)) : [];

    for (let col = 0; col < 8; col++) {
      const idx = rowNum * 8 + col;
      frameDistances[idx] = dists[col] || 0;
      frameConfidences[idx] = confs[col] || 0;
    }
  }
}

function handleCommandResponse(line) {
  if (line.startsWith('OK PERIOD:')) {
    const period = parseInt(line.substring(10), 10);
    periodSelect.value = String(period);
    currentPeriodDisplay.textContent = `${period} ms`;
    addLog(line, 'status');
  } else if (line.startsWith('OK KITER:')) {
    addLog(line, 'status');
  }
}

function renderGrid() {
  currentTempDisplay.textContent = `${frameTemp}°C`;
  currentPeriodDisplay.textContent = `${periodSelect.value} ms`;

  for (let i = 0; i < 64; i++) {
    const cell = document.getElementById(`cell-${i}`);
    if (!cell) continue;

    const dist = frameDistances[i];
    const conf = frameConfidences[i];
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
    await writer.write(new TextEncoder().encode(cmd + '\n'));
    addLog(`Sent: ${cmd}`, 'command');
  } catch (error) {
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
  toggleText.textContent = serialLog.classList.contains('expanded')
    ? '▲ Click to collapse'
    : '▼ Click to expand';
  if (serialLog.classList.contains('expanded')) {
    serialLog.scrollTop = serialLog.scrollHeight;
  }
}
