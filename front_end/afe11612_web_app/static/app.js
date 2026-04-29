const ADC_CHANNELS = 16;
const TEMP_CHANNELS = 3;
const DAC_CHANNELS = 12;
const POLL_MS = 700;

let polling = true;

/* ================= ADC ================= */

async function updateADC() {
  const r = await fetch("/api/adc");
  const d = await r.json();

  let html = `
    <thead>
      <tr>
        <th>CH</th>
        <th>V</th>
      </tr>
    </thead>
    <tbody>
  `;

  d.forEach((v, i) => {
    html += `
      <tr>
        <td>${i}</td>
        <td>${v.toFixed(4)}</td>
      </tr>
    `;
  });

  html += "</tbody>";
  document.getElementById("adc-table").innerHTML = html;
}

/* ================= TEMP ================= */

async function updateTemp() {
  const r = await fetch("/api/temp");
  const d = await r.json();

  let html = `
    <thead>
      <tr>
        <th>CH</th>
        <th>°C</th>
      </tr>
    </thead>
    <tbody>
  `;

  d.forEach((v, i) => {
    html += `
      <tr>
        <td>${i}</td>
        <td>${v.toFixed(2)}</td>
      </tr>
    `;
  });

  html += "</tbody>";
  document.getElementById("temp-table").innerHTML = html;
}

/* ================= DAC ================= */

function buildDAC() {
  let html = `
    <thead>
      <tr>
        <th>CH</th>
        <th>mV</th>
        <th></th>
        <th>Status</th>
      </tr>
    </thead>
    <tbody>
  `;

  for (let i = 0; i < DAC_CHANNELS; i++) {
    html += `
      <tr>
        <td>${i}</td>
        <td>
          <input type="number"
                 id="dac${i}"
                 min="0"
                 max="5000"
                 value="0">
        </td>
        <td>
          <button onclick="setDAC(${i})">Set</button>
        </td>
        <td id="dac${i}-s">—</td>
      </tr>
    `;
  }

  html += "</tbody>";
  document.getElementById("dac-table").innerHTML = html;
}

async function setDAC(ch) {
  const input = document.getElementById(`dac${ch}`);
  const status = document.getElementById(`dac${ch}-s`);
  const mv = parseInt(input.value, 10);

  if (isNaN(mv) || mv < 0 || mv > 5000) {
    status.textContent = "Invalid";
    status.style.color = "red";
    return;
  }

  polling = false;

  try {
    const r = await fetch(`/api/dac/${ch}/${mv}`, { method: "POST" });
    status.textContent = r.ok ? "OK" : "ERR";
    status.style.color = r.ok ? "green" : "red";
  } finally {
    polling = true;
  }
}

/* ================= DEVICE ID ================= */

async function readDeviceID() {
  const r = await fetch("/api/device_id");
  const d = await r.json();
  document.getElementById("top-status").textContent =
    "Device ID: " + d.device_id;
}

/* ================= INIT ================= */

buildDAC();
updateADC();
updateTemp();

setInterval(() => {
  if (polling) {
    updateADC();
    updateTemp();
  }
}, POLL_MS);
