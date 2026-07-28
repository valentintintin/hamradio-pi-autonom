// ============================================================================
// Simulateur rp2040-lora-aprs — front-end du tableau de bord.
//
// Aucune dépendance : vanilla JS, un seul fichier. Contrat serveur (cf.
// native_sim/web/server.py) :
//   GET  /api/state   -> JSON, instantané complet (SimWorld + telemetry)
//   POST /api/sensor  -> { temp_c, humidity, pressure_hpa, battery_mv,
//                          battery_ma, solar_mv, solar_ma, board5v_mv, board5v_ma }
//   POST /api/rx      -> { which: mesh|aprs|fsk, format: hex|ascii, payload }
//   POST /api/relay   -> { n, on }
// ============================================================================

// ------------------------------------------------------------------ utils

async function post(url, params) {
  const body = new URLSearchParams(params).toString();
  const r = await fetch(url, { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body });
  if (!r.ok) throw new Error(`HTTP ${r.status}`);
  return r;
}

function fmt(n, digits = 1) {
  return (typeof n === 'number') ? n.toFixed(digits) : n;
}

function toast(message, isError = false) {
  const el = document.createElement('div');
  el.className = 'toast' + (isError ? ' error' : '');
  el.textContent = message;
  document.getElementById('toasts').appendChild(el);
  setTimeout(() => el.remove(), 3200);
}

// Synchronise un <input type=range> et un <input type=number> : déplacer
// l'un met à jour l'autre. Le number reste libre d'accepter une valeur hors
// bornes du slider (utile pour tester des cas extrêmes) sans se faire
// écraser au prochain rafraîchissement (cf. suppressUntil dans refresh()).
function bindSlider(rangeId, numberId) {
  const range = document.getElementById(rangeId);
  const number = document.getElementById(numberId);
  range.value = number.value;
  range.addEventListener('input', () => { number.value = range.value; });
  number.addEventListener('input', () => {
    const v = parseFloat(number.value);
    if (!Number.isNaN(v)) range.value = Math.min(Math.max(v, +range.min), +range.max);
  });
}

// ------------------------------------------------------------------- tabs

function initTabs() {
  const tabs = document.querySelectorAll('.tab');
  tabs.forEach(tab => {
    tab.addEventListener('click', () => {
      tabs.forEach(t => { t.classList.remove('active'); t.setAttribute('aria-selected', 'false'); });
      tab.classList.add('active');
      tab.setAttribute('aria-selected', 'true');
      document.querySelectorAll('.panel').forEach(p => p.classList.remove('active'));
      document.getElementById('panel-' + tab.dataset.tab).classList.add('active');
    });
  });
}

// -------------------------------------------------------------- rendering

function radioCard(title, r, cfg, showFsk) {
  const mode = showFsk ? (r.fsk_mode ? '📻 FSK (WH65B)' : 'LoRa') : 'LoRa';
  return `
    <div class="card">
      <b>${title}</b> <span class="mode">${mode}</span>
      <table>
        <tr><td>Fréquence</td><td>${fmt(cfg.freq, 3)} MHz</td></tr>
        <tr><td>BW / SF / CR</td><td>${fmt(cfg.bw, 1)} kHz / SF${cfg.sf} / 4:${cfg.cr}</td></tr>
        <tr><td>Puissance TX</td><td>${cfg.tx_power_dbm} dBm</td></tr>
        <tr><td>Dernier RSSI/SNR</td><td>${fmt(r.last_rssi, 0)} dBm / ${fmt(r.last_snr, 1)} dB</td></tr>
        <tr><td>Paquets reçus/envoyés</td><td>${r.packets_recv} / ${r.packets_sent}</td></tr>
      </table>
    </div>`;
}

function setConnStatus(ok, label) {
  const el = document.getElementById('connStatus');
  el.classList.remove('ok', 'err');
  el.classList.add(ok ? 'ok' : 'err');
  document.getElementById('connText').textContent = label;
}

function renderPower(t, si) {
  document.getElementById('cards-power').innerHTML = `
    <div class="card">
      <b>Batterie</b>
      <table>
        <tr><td>INA3221</td><td>${fmt(t.battery_ina.voltage_mv, 0)} mV / ${fmt(t.battery_ina.current_ma, 0)} mA</td></tr>
        <tr><td>MPPT/Victron</td><td>${fmt(t.battery_mppt.voltage_mv, 0)} mV / ${fmt(t.battery_mppt.current_ma, 0)} mA</td></tr>
      </table>
    </div>
    <div class="card">
      <b>Solaire</b>
      <table>
        <tr><td>INA3221</td><td>${fmt(t.solar_ina.voltage_mv, 0)} mV / ${fmt(t.solar_ina.current_ma, 0)} mA</td></tr>
        <tr><td>MPPT/Victron</td><td>${fmt(t.solar_mppt.voltage_mv, 0)} mV / ${fmt(t.solar_mppt.current_ma, 0)} mA</td></tr>
      </table>
    </div>
    <div class="card"><b>Board 5V</b><br>${fmt(t.board_5v.voltage_mv, 0)} mV / ${fmt(t.board_5v.current_ma, 0)} mA</div>
    <div class="card">
      <b>Chargeur</b>
      <table>
        <tr><td>MPPT</td><td>${si.mppt_present ? '✅ présent' : '⬜ absent'} ${si.mppt_alert ? '⚠️ ALERTE' : ''}</td></tr>
        <tr><td>Statut</td><td>${t.mppt_status_name}</td></tr>
        <tr><td>Victron</td><td>${si.victron_present ? `✅ présent — SOC ${fmt(t.victron_soc, 0)}%` : '⬜ absent'}</td></tr>
      </table>
    </div>`;
}

function renderWeather(t) {
  document.getElementById('cards-weather').innerHTML = `
    <div class="card">
      <b>Météo intérieure (BME280)</b><br>
      ${fmt(t.weather_inside.temperature_c)}°C · ${fmt(t.weather_inside.humidity, 0)}% · ${fmt(t.weather_inside.pressure_hpa, 1)} hPa
    </div>
    <div class="card">
      <b>Météo extérieure (WH65B)</b>
      ${t.weather_outside.is_valid ? `
        <table>
          <tr><td>Vent moy/rafale</td><td>${fmt(t.weather_outside.wind_avg_ms)} / ${fmt(t.weather_outside.wind_max_ms)} m/s</td></tr>
          <tr><td>Direction</td><td>${t.weather_outside.wind_dir_deg}°</td></tr>
          <tr><td>Pluie</td><td>${fmt(t.weather_outside.rain_mm)} mm</td></tr>
          <tr><td>Lumière / UV</td><td>${fmt(t.weather_outside.light_lux, 0)} lux / UV${t.weather_outside.uv_index}</td></tr>
        </table>` : '<span class="hint">(aucune trame WH65B reçue — injecte-la depuis l\'onglet Simulation)</span>'}
    </div>`;
}

function renderRadio(s) {
  document.getElementById('cards-radio').innerHTML =
    radioCard('Radio mesh (868 MHz)', s.radio_mesh, s.radio_mesh, false) +
    radioCard('Radio APRS (433 MHz)', s.radio_aprs, s.aprs_config, true);
}

function relayToggle(n, on) {
  post('/api/relay', { n, on: on ? 1 : 0 })
    .then(() => { toast(`Relais ${n} ${on ? 'allumé' : 'éteint'}`); refresh(); })
    .catch(e => toast(`Échec relais ${n} : ${e.message}`, true));
}

function renderRelays(relays) {
  document.getElementById('relays').innerHTML = relays.map((on, i) => `
    <div class="relay-row">
      <span class="pill ${on ? 'on' : 'off'}">relais ${i + 1} · ${on ? 'ON' : 'off'}</span>
      <button onclick="relayToggle(${i + 1},${on ? 0 : 1})">${on ? 'Éteindre' : 'Allumer'}</button>
    </div>`).join('');
}

let lastLogLines = [];

function renderJournal(s) {
  document.getElementById('radiolog').textContent =
    s.radio_log.map(e => `[${e.ms}ms] ${e.dir} ${e.chan}: ${e.hex}`).join('\n') || '(aucune)';
  lastLogLines = s.logs;
  applyLogFilter();
}

function applyLogFilter() {
  const q = document.getElementById('logFilter').value.trim().toLowerCase();
  const lines = q ? lastLogLines.filter(l => l.toLowerCase().includes(q)) : lastLogLines;
  document.getElementById('logs').textContent = lines.join('\n') || '(aucun log)';
}

// -------------------------------------------------------------- refresh loop

async function refresh() {
  let s;
  try {
    const r = await fetch('/api/state');
    if (!r.ok) {
      setConnStatus(false, `Indisponible (${r.status})`);
      return;
    }
    s = await r.json();
  } catch (e) {
    setConnStatus(false, 'Hors ligne');
    return;
  }

  setConnStatus(true, 'Connecté');
  document.getElementById('uptimeBadge').textContent = `⏱ ${s.telemetry.uptime_s} s`;

  renderPower(s.telemetry, s.sim_inputs);
  renderWeather(s.telemetry);
  renderRadio(s);
  renderRelays(s.relays);
  syncMpptVictronFromState(s.sim_inputs);
  renderJournal(s);
}

// ------------------------------------------------------------------- forms

function initForms() {
  bindSlider('battery_mv_r', 'battery_mv');
  bindSlider('battery_ma_r', 'battery_ma');
  bindSlider('solar_mv_r', 'solar_mv');
  bindSlider('solar_ma_r', 'solar_ma');
  bindSlider('board5v_mv_r', 'board5v_mv');
  bindSlider('board5v_ma_r', 'board5v_ma');
  bindSlider('temp_c_r', 'temp_c');
  bindSlider('humidity_r', 'humidity');
  bindSlider('pressure_hpa_r', 'pressure_hpa');

  document.getElementById('form-battery').addEventListener('submit', e => {
    e.preventDefault();
    post('/api/sensor', { battery_mv: v('battery_mv'), battery_ma: v('battery_ma') })
      .then(() => { toast('Batterie mise à jour'); refresh(); })
      .catch(err => toast(`Échec : ${err.message}`, true));
  });

  document.getElementById('form-solar').addEventListener('submit', e => {
    e.preventDefault();
    post('/api/sensor', { solar_mv: v('solar_mv'), solar_ma: v('solar_ma') })
      .then(() => { toast('Solaire mis à jour'); refresh(); })
      .catch(err => toast(`Échec : ${err.message}`, true));
  });

  document.getElementById('form-board5v').addEventListener('submit', e => {
    e.preventDefault();
    post('/api/sensor', { board5v_mv: v('board5v_mv'), board5v_ma: v('board5v_ma') })
      .then(() => { toast('Board 5V mis à jour'); refresh(); })
      .catch(err => toast(`Échec : ${err.message}`, true));
  });

  document.getElementById('form-weather').addEventListener('submit', e => {
    e.preventDefault();
    post('/api/sensor', { temp_c: v('temp_c'), humidity: v('humidity'), pressure_hpa: v('pressure_hpa') })
      .then(() => { toast('Météo intérieure mise à jour'); refresh(); })
      .catch(err => toast(`Échec : ${err.message}`, true));
  });

  document.getElementById('form-rx').addEventListener('submit', e => {
    e.preventDefault();
    const which = document.getElementById('rx_which').value;
    const format = document.getElementById('rx_format').value;
    const payload = document.getElementById('rx_payload').value;
    if (!payload.trim()) { toast('Trame vide', true); return; }
    post('/api/rx', { which, format, payload })
      .then(() => { toast(`Trame ${which} injectée`); refresh(); })
      .catch(err => toast(`Échec injection : ${err.message}`, true));
  });

  document.getElementById('rx_example').addEventListener('click', () => {
    document.getElementById('rx_which').value = 'aprs';
    document.getElementById('rx_format').value = 'ascii';
    document.getElementById('rx_payload').value =
      'N0CALL>APRS,WIDE1-1:!4903.50N/07201.75W_090/006g015t077r000p003P003h50b10132';
  });

  document.getElementById('logFilter').addEventListener('input', applyLogFilter);
}

function v(id) { return document.getElementById(id).value; }

// ---------------------------------------------------------------- presets

const PRESETS = [
  {
    label: '☀️ Plein soleil',
    values: { temp_c: 28, humidity: 35, pressure_hpa: 1018, battery_mv: 13600, battery_ma: 1800, solar_mv: 20500, solar_ma: 1400, board5v_mv: 5000, board5v_ma: 140 },
  },
  {
    label: '🌙 Nuit, batterie faible',
    values: { temp_c: 12, humidity: 70, pressure_hpa: 1015, battery_mv: 11400, battery_ma: -320, solar_mv: 0, solar_ma: 0, board5v_mv: 4950, board5v_ma: 110 },
  },
  {
    label: '⛈️ Orage',
    values: { temp_c: 16, humidity: 92, pressure_hpa: 995, battery_mv: 12200, battery_ma: -260, solar_mv: 800, solar_ma: 40, board5v_mv: 4980, board5v_ma: 130 },
  },
  {
    label: '↺ Valeurs par défaut',
    values: { temp_c: 21.5, humidity: 48, pressure_hpa: 1013, battery_mv: 12800, battery_ma: -180, solar_mv: 18000, solar_ma: 350, board5v_mv: 5000, board5v_ma: 120 },
  },
];

function applyPreset(preset) {
  const p = preset.values;
  post('/api/sensor', p)
    .then(() => {
      Object.entries(p).forEach(([id, val]) => {
        const el = document.getElementById(id);
        if (el) el.value = val;
        const range = document.getElementById(id + '_r');
        if (range) range.value = val;
      });
      toast(`Préréglage appliqué : ${preset.label}`);
      refresh();
    })
    .catch(err => toast(`Échec préréglage : ${err.message}`, true));
}

function initPresets() {
  document.getElementById('presets').innerHTML =
    PRESETS.map((p, i) => `<button type="button" data-i="${i}">${p.label}</button>`).join('');
  document.querySelectorAll('#presets button').forEach(btn => {
    btn.addEventListener('click', () => applyPreset(PRESETS[+btn.dataset.i]));
  });
}

// --------------------------------------------------------------- cmd (générique)

function sendCmd(cmd) {
  return post('/api/cmd', { cmd });
}

// ------------------------------------------------------------------- RTC

function initRtc() {
  document.getElementById('rtc_now').addEventListener('click', () => {
    const d = new Date();
    const iso = new Date(d.getTime() - d.getTimezoneOffset() * 60000).toISOString();
    document.getElementById('rtc_datetime').value = iso.slice(0, 19);
  });

  document.getElementById('form-rtc').addEventListener('submit', e => {
    e.preventDefault();
    const raw = document.getElementById('rtc_datetime').value;
    if (!raw) { toast('Choisis une date/heure', true); return; }
    const [datePart, timePart] = raw.split('T');
    const [yyyy, mm, dd] = datePart.split('-');
    const [HH, MM, SS] = (timePart || '00:00:00').split(':');
    const cmd = `clockdate ${dd}/${mm}/${yyyy.slice(2)} ${HH}:${MM}:${SS || '00'}`;
    sendCmd(cmd)
      .then(() => toast('Horloge réglée'))
      .catch(err => toast(`Échec : ${err.message}`, true));
  });
}

// ----------------------------------------------------------- MPPT / Victron

let mpptVictronInitialized = false;

function syncMpptVictronFromState(si) {
  if (mpptVictronInitialized || !si) return;
  mpptVictronInitialized = true;
  document.getElementById('mppt_present').checked = !!si.mppt_present;
  document.getElementById('mppt_alert').checked = !!si.mppt_alert;
  const statusSelect = document.getElementById('mppt_status');
  const name = (si.mppt_status_forced || 'NIGHT').toLowerCase();
  if ([...statusSelect.options].some(o => o.value === name)) statusSelect.value = name;
  document.getElementById('victron_present').checked = !!si.victron_present;
  if (typeof si.victron_soc_pct === 'number') {
    document.getElementById('victron_soc').value = si.victron_soc_pct;
    document.getElementById('victron_soc_r').value = si.victron_soc_pct;
  }
  if (typeof si.victron_state === 'number') {
    const stateSelect = document.getElementById('victron_state');
    if ([...stateSelect.options].some(o => +o.value === si.victron_state)) stateSelect.value = si.victron_state;
  }
}

function initMpptVictron() {
  bindSlider('victron_soc_r', 'victron_soc');

  document.getElementById('mppt_apply').addEventListener('click', async () => {
    try {
      await sendCmd(`sim set mppt present ${document.getElementById('mppt_present').checked ? 'on' : 'off'}`);
      await sendCmd(`sim set mppt alert ${document.getElementById('mppt_alert').checked ? 'on' : 'off'}`);
      await sendCmd(`sim set mppt status ${document.getElementById('mppt_status').value}`);
      toast('MPPT mis à jour');
      refresh();
    } catch (err) {
      toast(`Échec MPPT : ${err.message}`, true);
    }
  });

  document.getElementById('victron_apply').addEventListener('click', async () => {
    try {
      await sendCmd(`sim set victron present ${document.getElementById('victron_present').checked ? 'on' : 'off'}`);
      await sendCmd(`sim set victron soc ${v('victron_soc')}`);
      await sendCmd(`sim set victron state ${document.getElementById('victron_state').value}`);
      toast('Victron mis à jour');
      refresh();
    } catch (err) {
      toast(`Échec Victron : ${err.message}`, true);
    }
  });
}

// ------------------------------------------------------------ commande brute

function initRawCmd() {
  document.getElementById('form-rawcmd').addEventListener('submit', e => {
    e.preventDefault();
    const cmd = v('rawcmd').trim();
    if (!cmd) return;
    sendCmd(cmd)
      .then(() => { toast(`Commande envoyée : ${cmd}`); refresh(); })
      .catch(err => toast(`Échec : ${err.message}`, true));
  });
}

// ================================================================
// Construction de trame APRS — formats spec-compatibles avec
// lib/Aprs/src/Aprs.cpp (mêmes largeurs de champs que les encodeurs
// C++, vérifié contre appendPosition/appendWeather/appendMessage/
// appendTelemetry/encodeObjectItem) afin que les trames injectées en
// RX soient réellement décodées par le firmware, pas juste loggées.
// ================================================================

const kMphToKnots = 0.868976;

function padInt(n, width) {
  n = Math.max(0, Math.round(n));
  return String(n).padStart(width, '0');
}

// "DDMM.mmN" (8 car.) — même arithmétique (+ report de retenue) que
// appendPositionUncompressed côté C++.
function formatLat(lat) {
  const absLat = Math.abs(lat);
  let deg = Math.floor(absLat);
  let hund = Math.floor((absLat - deg) * 6000 + 0.5);
  if (hund >= 6000) { hund -= 6000; deg++; }
  return `${padInt(deg, 2)}${padInt(Math.floor(hund / 100), 2)}.${padInt(hund % 100, 2)}${lat < 0 ? 'S' : 'N'}`;
}
// "DDDMM.mmW" (9 car.)
function formatLon(lon) {
  const absLon = Math.abs(lon);
  let deg = Math.floor(absLon);
  let hund = Math.floor((absLon - deg) * 6000 + 0.5);
  if (hund >= 6000) { hund -= 6000; deg++; }
  return `${padInt(deg, 3)}${padInt(Math.floor(hund / 100), 2)}.${padInt(hund % 100, 2)}${lon < 0 ? 'W' : 'E'}`;
}

function padDest(dest) {
  return (dest || '').slice(0, 9).padEnd(9, ' ');
}

function buildAprsPayload(type) {
  switch (type) {
    case 'position': {
      const lat = parseFloat(v('aprs_lat')) || 0;
      const lon = parseFloat(v('aprs_lon')) || 0;
      const symTable = v('aprs_sym_table') || '/';
      const symCode = v('aprs_sym_code') || '>';
      const course = parseFloat(v('aprs_course')) || 0;
      const speed = parseFloat(v('aprs_speed')) || 0;
      const alt = parseFloat(v('aprs_alt')) || 0;
      const comment = v('aprs_comment') || '';
      let s = '=' + formatLat(lat) + symTable + formatLon(lon) + symCode;
      if (course > 0 || speed > 0) s += `${padInt(course % 360, 3)}/${padInt(speed, 3)}`;
      if (alt > 0) s += `/A=${padInt(alt, 6)}`;
      return s + comment;
    }
    case 'weather': {
      const lat = parseFloat(v('aprs_wx_lat')) || 0;
      const lon = parseFloat(v('aprs_wx_lon')) || 0;
      const windMs = parseFloat(v('aprs_wx_wind')) || 0;
      const gustMs = parseFloat(v('aprs_wx_gust')) || 0;
      const dir = parseFloat(v('aprs_wx_dir')) || 0;
      const tempC = parseFloat(v('aprs_wx_temp')) || 0;
      const rain1h = parseFloat(v('aprs_wx_rain1h')) || 0;
      const rain24h = parseFloat(v('aprs_wx_rain24h')) || 0;
      const hum = parseFloat(v('aprs_wx_hum')) || 0;
      const pres = parseFloat(v('aprs_wx_pres')) || 0;

      const windMph = windMs * 2.23694;
      const gustMph = gustMs * 2.23694;
      const windKnots = windMph * kMphToKnots;
      const tempF = Math.round(tempC * 9 / 5 + 32);
      const rain1hHundredths = Math.round(rain1h * 3.93701);
      const rain24hHundredths = Math.round(rain24h * 3.93701);

      let s = '=' + formatLat(lat) + '/' + formatLon(lon) + '_';
      s += `${padInt(dir % 360, 3)}/${padInt(windKnots, 3)}`;
      s += `g${padInt(gustMph, 3)}`;
      s += `t${tempF < 0 ? '-' + padInt(-tempF, 2) : padInt(tempF, 3)}`;
      s += `r${padInt(rain1hHundredths, 3)}`;
      s += `p${padInt(rain24hHundredths, 3)}`;
      s += `P...`;  // pas de champ "depuis minuit" distinct dans le formulaire
      s += `h${padInt(hum >= 100 ? 0 : hum, 2)}`;
      s += `b${padInt(pres * 10, 5)}`;
      return s;
    }
    case 'status':
      return '>' + (v('aprs_status_text') || '');
    case 'message': {
      const to = padDest(v('aprs_msg_to'));
      const text = v('aprs_msg_text') || '';
      const ack = v('aprs_msg_ack').trim();
      return `:${to}:${text}` + (ack ? `{${ack}}` : '');
    }
    case 'object': {
      const name = (v('aprs_obj_name') || 'OBJECT').slice(0, 9).padEnd(9, ' ');
      const alive = v('aprs_obj_alive') === '1';
      const now = new Date();
      const hh = padInt(now.getUTCHours(), 2), mm = padInt(now.getUTCMinutes(), 2), ss = padInt(now.getUTCSeconds(), 2);
      const lat = parseFloat(v('aprs_obj_lat')) || 0;
      const lon = parseFloat(v('aprs_obj_lon')) || 0;
      const symTable = v('aprs_obj_sym_table') || '/';
      const symCode = v('aprs_obj_sym_code') || 'o';
      const comment = v('aprs_obj_comment') || '';
      return `;${name}${alive ? '*' : '_'}${hh}${mm}${ss}h${formatLat(lat)}${symTable}${formatLon(lon)}${symCode}${comment}`;
    }
    case 'item': {
      const name = (v('aprs_item_name') || 'ITEM').slice(0, 9);
      const alive = v('aprs_item_alive') === '1';
      const lat = parseFloat(v('aprs_item_lat')) || 0;
      const lon = parseFloat(v('aprs_item_lon')) || 0;
      const comment = v('aprs_item_comment') || '';
      return `)${name}${alive ? '!' : '_'}${formatLat(lat)}/${formatLon(lon)}o${comment}`;
    }
    case 'query': {
      const type = v('aprs_query_type');
      const dest = v('aprs_query_dest').trim();
      return dest ? `:${padDest(dest)}:?${type}?` : `?${type}?`;
    }
    case 'telemetry': {
      const seq = padInt(parseInt(v('aprs_tlm_seq')) || 0, 3);
      const vals = ['aprs_tlm_a1', 'aprs_tlm_a2', 'aprs_tlm_a3', 'aprs_tlm_a4', 'aprs_tlm_a5']
        .map(id => padInt(Math.min(255, parseInt(v(id)) || 0), 3));
      const bits = (v('aprs_tlm_bits') || '00000000').padEnd(8, '0').slice(0, 8);
      return `T#${seq},${vals.join(',')},${bits}`;
    }
    case 'raw':
      return v('aprs_raw_content') || '';
    default:
      return '';
  }
}

const APRS_TYPES = ['position', 'weather', 'status', 'message', 'object', 'item', 'query', 'telemetry', 'raw'];

function updateAprsPreview() {
  const type = v('aprs_type');
  const payload = buildAprsPayload(type);
  const source = (v('aprs_source') || 'NOCALL').toUpperCase();
  const dest = (v('aprs_dest') || 'APRS').toUpperCase();
  const path = v('aprs_path').trim();
  const frame = `${source}>${dest}${path ? ',' + path : ''}:${payload}`;
  document.getElementById('aprs_preview').value = frame;
  return { frame, payload };
}

function updateAprsFieldVisibility() {
  const type = v('aprs_type');
  APRS_TYPES.forEach(t => {
    const el = document.getElementById('aprs_fields_' + t);
    if (el) el.style.display = (t === type) ? '' : 'none';
  });
}

function initAprsBuilder() {
  const panel = document.getElementById('panel-trames');
  updateAprsFieldVisibility();
  updateAprsPreview();

  panel.addEventListener('input', () => { updateAprsFieldVisibility(); updateAprsPreview(); });
  panel.addEventListener('change', () => { updateAprsFieldVisibility(); updateAprsPreview(); });

  document.getElementById('aprs_inject').addEventListener('click', () => {
    const { frame } = updateAprsPreview();
    post('/api/rx', { which: 'aprs', format: 'ascii', payload: frame })
      .then(() => { toast('Trame APRS injectée (RX simulée)'); refresh(); })
      .catch(err => toast(`Échec injection : ${err.message}`, true));
  });

  document.getElementById('aprs_transmit').addEventListener('click', () => {
    const { payload } = updateAprsPreview();
    if (!payload) { toast('Contenu vide', true); return; }
    sendCmd(`send aprs ${payload}`)
      .then(() => { toast('Trame émise depuis la station (TX réel)'); refresh(); })
      .catch(err => toast(`Échec émission : ${err.message}`, true));
  });
}

// ================================================================
// Construction de trame WH65B — encodeur inverse exact de
// lib/FineOffsetWH65B/FineOffsetWH65B.cpp::decode (même CRC8
// poly 0x31 / init 0x00, mêmes largeurs de bits par champ). Seuls les
// 17 octets utiles sont produits ; le firmware complète lui-même le
// reste du buffer WH65B_PAYLOAD_LEN=27 à zéro (cf. task_weather.cpp).
// ================================================================

function crc8Wh65b(bytes) {
  let crc = 0x00;
  for (const b of bytes) {
    crc ^= b;
    for (let i = 0; i < 8; i++) {
      crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) & 0xFF : (crc << 1) & 0xFF;
    }
  }
  return crc;
}

function computeUvi(uvRaw) {
  const upper = [432, 851, 1210, 1570, 2017, 2450, 2761, 3100, 3512, 3918, 4277, 4650, 5029];
  let uvi = 0;
  while (uvi < 13 && uvRaw > upper[uvi]) uvi++;
  return uvi;
}

function buildWh65bBytes() {
  const id = Math.min(255, Math.max(0, parseInt(v('wh_id')) || 0));
  const windMs = parseFloat(v('wh_wind')) || 0;
  const gustMs = parseFloat(v('wh_gust')) || 0;
  const dirDeg = Math.min(511, Math.max(0, parseInt(v('wh_dir')) || 0));
  const tempC = parseFloat(v('wh_temp')) || 0;
  const hum = Math.min(254, Math.max(0, parseInt(v('wh_hum')) || 0));
  const rainMm = parseFloat(v('wh_rain')) || 0;
  const lightLux = parseFloat(v('wh_light')) || 0;
  const uvRaw = Math.min(0xFFFE, Math.max(0, parseInt(v('wh_uv')) || 0));
  const battLow = v('wh_batt_low') === '1';

  const windDirRaw = dirDeg & 0x1FF;
  const tempRaw = Math.min(0x7FE, Math.max(0, Math.round(tempC * 10 + 400)));
  const windSpeedRaw = Math.min(0x1FE, Math.max(0, Math.round(windMs / (0.125 * 0.51))));
  const gustRaw = Math.min(254, Math.max(0, Math.round(gustMs / 0.51)));
  const rainRaw = Math.min(0xFFFF, Math.max(0, Math.round(rainMm / 0.254)));
  const lightRaw = Math.min(0xFFFFFE, Math.max(0, Math.round(lightLux / 0.1)));

  const b = new Uint8Array(17);
  b[0] = 0x24;
  b[1] = id;
  b[2] = windDirRaw & 0xFF;
  b[3] = ((windDirRaw >> 8) & 1) << 7
       | (battLow ? 1 : 0) << 3
       | ((windSpeedRaw >> 8) & 1) << 4
       | ((tempRaw >> 8) & 0x07);
  b[4] = tempRaw & 0xFF;
  b[5] = hum;
  b[6] = windSpeedRaw & 0xFF;
  b[7] = gustRaw;
  b[8] = (rainRaw >> 8) & 0xFF;
  b[9] = rainRaw & 0xFF;
  b[10] = (uvRaw >> 8) & 0xFF;
  b[11] = uvRaw & 0xFF;
  b[12] = (lightRaw >> 16) & 0xFF;
  b[13] = (lightRaw >> 8) & 0xFF;
  b[14] = lightRaw & 0xFF;
  b[15] = crc8Wh65b(b.slice(0, 15));
  let sum = 0;
  for (let i = 0; i < 16; i++) sum += b[i];  // inclut b[15] (crc), comme decode()
  b[16] = sum & 0xFF;
  return b;
}

function bytesToHex(bytes) {
  return [...bytes].map(x => x.toString(16).padStart(2, '0').toUpperCase()).join('');
}

function updateWhPreview() {
  const bytes = buildWh65bBytes();
  document.getElementById('wh_preview').value = bytesToHex(bytes);
  document.getElementById('wh_uvi_preview').textContent = computeUvi(parseInt(v('wh_uv')) || 0);
  return bytes;
}

function initWh65bBuilder() {
  const panel = document.getElementById('panel-trames');
  updateWhPreview();
  panel.addEventListener('input', updateWhPreview);
  panel.addEventListener('change', updateWhPreview);

  document.getElementById('wh_inject').addEventListener('click', () => {
    const hex = bytesToHex(buildWh65bBytes());
    post('/api/rx', { which: 'fsk', format: 'hex', payload: hex })
      .then(() => { toast('Trame WH65B injectée'); refresh(); })
      .catch(err => toast(`Échec injection : ${err.message}`, true));
  });
}

// -------------------------------------------------------------------- boot

initTabs();
initForms();
initPresets();
initRtc();
initMpptVictron();
initRawCmd();
initAprsBuilder();
initWh65bBuilder();
refresh();
setInterval(refresh, 1000);
