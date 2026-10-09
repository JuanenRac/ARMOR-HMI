/* ARMOR-HMI panel - the pages. Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
   text.js (the seven languages) is joined in front of this file when the panel is packed, so LANGS and L exist here.
   No framework and no inline style: the panel is served with a strict content-security policy. */
"use strict";

// ---- small tools ---------------------------------------------------------------------------------------------------------------------

const $app = document.getElementById("app");
let lang = 0;
const t = (key, ...args) => {
  const row = L[key];
  let text = row ? (row[lang] || row[0]) : key;
  args.forEach((a, i) => { text = text.split("{" + i + "}").join(a); });
  return text;
};

function el(tag, attrs, ...kids) {
  const node = document.createElement(tag);
  for (const [name, value] of Object.entries(attrs || {})) {
    if (value === undefined || value === null || value === false) continue;
    if (name === "tip") { const text = L["tip_" + value] ? t("tip_" + value) : ""; if (text) node.title = text; continue; }   // the hover hint, in the panel's language
    if (name === "class") node.className = value;
    else if (name.startsWith("on")) node.addEventListener(name.slice(2), value);
    else if (name in node && name !== "list") node[name] = value;
    else node.setAttribute(name, value === true ? "" : value);
  }
  const add = kid => {
    if (Array.isArray(kid)) kid.forEach(add);
    else if (kid !== null && kid !== undefined && kid !== false) node.append(kid instanceof Node ? kid : document.createTextNode(String(kid)));
  };
  kids.forEach(add);
  return node;
}

const S = {
  session: null, cfg: null, saved: "", channelAuto: 1, firmware: "", status: null, screen: null, users: [],
  page: "overview", problems: {}, message: null, restartNeeded: false, scan: { busy: false, list: null, error: "" }, log: { next: 0, text: "" }, busy: false, rebooting: false,
};
const isAdmin = () => S.session && S.session.role === "admin";

async function api(method, path, body) {
  let response;
  try {
    response = await fetch("/api/v1/" + path, {
      method, credentials: "same-origin",
      headers: Object.assign({ "X-Requested-With": "armor" }, body !== undefined ? { "Content-Type": "application/json" } : {}),
      body: body !== undefined ? JSON.stringify(body) : undefined,
    });
  } catch (error) { return { ok: false, status: 0, data: { error: "network" } }; }
  let data = {};
  try { data = await response.json(); } catch (error) { /* an empty answer */ }
  if (response.status === 401 && S.session && S.session.authenticated) { S.session.authenticated = false; start(); }
  return { ok: response.ok, status: response.status, data };
}

const errorText = code => (L["e_" + code] ? t("e_" + code) : String(code || "?"));
const problemText = code => (L["p_" + code] ? t("p_" + code) : String(code));

function pickLanguage(preferred) {
  let code = "";
  try { code = localStorage.getItem("armor_lang") || ""; } catch (error) { /* storage may be blocked */ }
  if (!code) code = preferred || (navigator.language || "en").slice(0, 2);
  const index = LANGS.findIndex(l => l[0] === code);
  lang = index >= 0 ? index : 0;
  document.documentElement.lang = LANGS[lang][0];
}

// ---- the working copy of the settings -----------------------------------------------------------------------------------------------

const walk = (path, create) => {
  const parts = path.split(".");
  let obj = S.cfg;
  for (let i = 0; i < parts.length - 1; ++i) {
    const key = /^\d+$/.test(parts[i]) ? Number(parts[i]) : parts[i];
    if (obj[key] === undefined && create) obj[key] = {};
    obj = obj[key];
  }
  const last = parts[parts.length - 1];
  return [obj, /^\d+$/.test(last) ? Number(last) : last];
};
const getValue = path => { const [o, k] = walk(path); return o ? o[k] : undefined; };
function setValue(path, value) {
  const [o, k] = walk(path, true);
  o[k] = value;
  delete S.problems[path];
  refreshBar();
}
const isDirty = () => S.cfg && JSON.stringify(S.cfg) !== S.saved;

function field(labelKey, path, options = {}) {
  const current = getValue(path);
  const problem = S.problems[path];
  const disabled = !isAdmin() || options.disabled;
  let input;
  if (options.type === "checkbox") {
    input = el("input", { type: "checkbox", checked: !!current, disabled, onchange: e => { setValue(path, e.target.checked); if (options.rerender) render(); } });
    return el("label", { class: "check", tip: labelKey }, input, t(labelKey), problem ? el("span", { class: "err" }, problemText(problem)) : null);
  }
  if (options.type === "select") {
    input = el("select", { disabled, onchange: e => { const v = e.target.value; setValue(path, options.number ? Number(v) : v); if (options.after) options.after(v); if (options.rerender) render(); } },
      options.options.map(([value, text]) => el("option", { value: String(value), selected: String(current) === String(value) }, text)));
  } else if (options.type === "number") {
    input = el("input", { type: "number", min: options.min, max: options.max, step: options.step || 1, value: current === undefined ? "" : current, disabled,
      oninput: e => setValue(path, e.target.value === "" ? NaN : Number(e.target.value)) });
  } else if (options.type === "password") {
    const stored = getValue(path.replace(/password$/, "password_set"));
    input = el("input", { type: "password", value: "", autocomplete: "new-password", disabled, placeholder: stored ? "••••••••" : "", oninput: e => setValue(path, e.target.value) });
  } else {
    input = el("input", { type: "text", value: current === undefined ? "" : current, disabled, maxLength: options.max, placeholder: options.placeholder || "", oninput: e => setValue(path, e.target.value) });
  }
  if (problem) input.classList.add("bad");
  return el("label", { class: "field", tip: labelKey }, el("span", {}, t(labelKey)), input, problem ? el("span", { class: "err" }, problemText(problem)) : null, options.hint ? el("span", { class: "hint" }, options.hint) : null);
}

// ---- the save bar --------------------------------------------------------------------------------------------------------------------

let barNode = null;
function refreshBar() {
  if (!barNode) return;
  const dirty = isDirty();
  const message = S.message || (dirty ? { kind: "warn", text: t("unsaved") } : S.restartNeeded ? { kind: "warn", text: t("savedRestart") } : null);
  barNode.replaceChildren(...[
    el("span", { class: "msg " + (message ? message.kind : "") }, message ? message.text : ""),
    S.restartNeeded && !dirty ? el("button", { class: "b danger", tip: "restart", onclick: () => reboot() }, t("restart")) : null,
    dirty ? el("button", { class: "b", tip: "discard", onclick: discard }, t("discard")) : null,
    dirty ? el("button", { class: "b primary", tip: "save", disabled: S.busy, onclick: () => save(false) }, t("save")) : null,
    dirty ? el("button", { class: "b danger", tip: "saveRestart", disabled: S.busy, onclick: () => save(true) }, t("saveRestart")) : null].filter(Boolean));
  barNode.hidden = !(message || dirty || S.restartNeeded);
}

// The panel's language also lives in the node's settings (ui.language, which the exported file shows): choosing it here saves it there at once, with
// no restart, instead of leaving the node on the language it was set up in. Only an admin can change settings.
async function persistLanguage() {
  if (!S.cfg || !S.session || !S.session.authenticated || !isAdmin()) return;
  const code = LANGS[lang][0];
  if (S.cfg.ui && S.cfg.ui.language === code) return;
  const r = await api("PUT", "config", { ui: { language: code } });
  if (!r.ok) return;
  S.cfg.ui.language = code;
  try { const saved = JSON.parse(S.saved); saved.ui = Object.assign({}, saved.ui, { language: code }); S.saved = JSON.stringify(saved); } catch (error) { /* the next save sorts it out */ }
  refreshBar();
}

function discard() { S.cfg = JSON.parse(S.saved); S.problems = {}; S.message = null; render(); }

// Exports the stored configuration (secrets included, like the flash itself) as a .json file: cloning the Wi-Fi, broker and the rest
// of a bench node onto a batch of identical ones, without retyping any of it by hand.
async function exportConfig() {
  const r = await api("GET", "config/export");
  if (!r.ok) { S.message = { kind: "err", text: errorText(r.data.error) }; render(); return; }
  const blob = new Blob([JSON.stringify(r.data.config, null, 2)], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const a = el("a", { href: url, download: "armor-" + (r.data.config.node ? r.data.config.node.id : "node") + ".json" });
  document.body.appendChild(a); a.click(); a.remove();
  URL.revokeObjectURL(url);
}

// Loads a previously exported file on top of the form (not sent to the node yet): the usual Save/Save and restart bar applies it like
// any other edit. The node keeps its own identity - node.id is never overwritten by an import.
function importConfig(file) {
  const reader = new FileReader();
  reader.onload = () => {
    let parsed;
    try { parsed = JSON.parse(String(reader.result)); } catch (error) { S.message = { kind: "err", text: t("importBadFile") }; render(); return; }
    const keepId = S.cfg.node ? S.cfg.node.id : undefined;
    S.cfg = Object.assign({}, S.cfg, parsed, { node: Object.assign({}, S.cfg.node, parsed.node, { id: keepId }) });
    S.problems = {}; S.message = { kind: "warn", text: t("importLoaded") };
    render();
  };
  reader.onerror = () => { S.message = { kind: "err", text: t("importBadFile") }; render(); };
  reader.readAsText(file);
}

async function save(restartAfter) {
  S.busy = true; S.message = { kind: "warn", text: t("working") }; refreshBar();
  const r = await api("PUT", "config", S.cfg);
  S.busy = false;
  if (r.ok) {
    S.problems = {};
    await loadConfig();
    S.restartNeeded = S.restartNeeded || !!r.data.restart_required;
    S.message = { kind: "ok", text: r.data.restart_required ? t("savedRestart") : t("saved") };
    if (restartAfter && S.restartNeeded) { reboot(); return; }
  } else if (r.status === 422 && r.data.problems) {
    S.problems = {};
    r.data.problems.forEach(p => { S.problems[p.path] = p.code; });
    S.message = { kind: "bad", text: t("e_invalid") };
  } else S.message = { kind: "bad", text: errorText(r.data.error) };
  render();
}

async function loadConfig() {
  const r = await api("GET", "config");
  if (!r.ok) return;
  S.cfg = r.data.config; S.saved = JSON.stringify(S.cfg); S.channelAuto = r.data.channel_auto; S.firmware = r.data.firmware;
}

async function reboot() {
  await api("POST", "reboot", {});
  S.rebooting = true; render();
  const wait = async () => {
    const r = await api("GET", "session");
    if (r.ok) location.reload(); else setTimeout(wait, 2000);
  };
  setTimeout(wait, 4000);
}

// ---- pages -----------------------------------------------------------------------------------------------------------------------------

const PAGES = [
  { id: "overview", icon: "◎", label: "navOverview", group: "navPanel" },
  { id: "server", icon: "⇋", label: "navServer", group: "navPanel" },
  { id: "screen", icon: "▭", label: "navScreen", group: "navPanel" },
  { id: "voice", icon: "♪", label: "navVoice", group: "navPanel" },
  { id: "network", icon: "⇄", label: "navNetwork", group: "navNetwork" },
  { id: "wifi", icon: "◌", label: "navWifi", group: "navNetwork" },
  { id: "broker", icon: "⌁", label: "navBroker", group: "navNetwork" },
  { id: "users", icon: "☺", label: "navUsers", group: "navSystem" },
  { id: "update", icon: "⟳", label: "navUpdate", group: "navSystem" },
  { id: "help", icon: "?", label: "navHelp", group: "navSystem" },
  { id: "about", icon: "ⓘ", label: "navAbout", group: "navSystem" },
];

const card = (title, ...kids) => el("section", { class: "card" }, title ? el("h2", {}, title) : null, ...kids);
const kv = rows => el("dl", { class: "kv" }, rows.filter(Boolean).flatMap(([k, v]) => [el("dt", {}, k), el("dd", {}, v)]));
const note = (text, kind) => el("p", { class: "note " + (kind || "") }, text);

const fmtBytes = b => b >= 1048576 ? (Math.round(b / 104857.6) / 10) + " MB" : b >= 1024 ? Math.round(b / 1024) + " kB" : b + " B";
const utcLabel = minutes => "UTC" + (minutes < 0 ? "-" : "+") + String(Math.floor(Math.abs(minutes) / 60)).padStart(2, "0") + ":" + String(Math.abs(minutes) % 60).padStart(2, "0");

// Common zones as POSIX rules (summer time changes by itself); anything else can be typed in as its own rule.
const TIME_ZONES = [
  ["UTC0", "UTC"], ["WET0WEST,M3.5.0/1,M10.5.0", "Lisbon · London · Dublin"], ["CET-1CEST,M3.5.0,M10.5.0/3", "Madrid · Paris · Berlin · Rome (CET/CEST)"],
  ["EET-2EEST,M3.5.0/3,M10.5.0/4", "Athens · Helsinki · Kyiv (EET/EEST)"], ["MSK-3", "Moscow · Istanbul"], ["GMT0", "Reykjavik · Dakar"],
  ["<-03>3", "Buenos Aires · São Paulo"], ["EST5EDT,M3.2.0,M11.1.0", "New York · Toronto"], ["CST6CDT,M3.2.0,M11.1.0", "Chicago · Mexico City"],
  ["MST7MDT,M3.2.0,M11.1.0", "Denver"], ["PST8PDT,M3.2.0,M11.1.0", "Los Angeles · Vancouver"], ["<-05>5", "Bogotá · Lima"], ["<-04>4", "Caracas · La Paz"],
  ["GST-4", "Dubai"], ["IST-5:30", "India"], ["<+07>-7", "Bangkok · Jakarta"], ["CST-8", "Beijing · Singapore · Hong Kong"], ["JST-9", "Tokyo · Seoul"],
  ["AEST-10AEDT,M10.1.0,M4.1.0/3", "Sydney · Melbourne"], ["NZST-12NZDT,M9.5.0,M4.1.0/3", "Auckland"], ["<+02>-2", "Cairo · Johannesburg"],
];
const zoneName = rule => { const hit = TIME_ZONES.find(z => z[0] === rule); return hit ? hit[1] + " (" + rule + ")" : rule; };

function flashCard(flash) {
  const parts = flash.partitions || [];
  const rows = [[t("flashTotal"), fmtBytes(flash.total)], [t("flashAllocated"), fmtBytes(flash.allocated) + " (" + Math.round(flash.allocated * 100 / flash.total) + " %)"], [t("flashUnallocated"), fmtBytes(flash.total - flash.allocated)]];
  parts.forEach(p => {
    const label = p.label.toUpperCase();
    if (p.app) {
      const state = p.running ? " · " + t("slotRunning") : p.next_boot ? " · " + t("slotNextBoot") : "";
      rows.push([label, p.used ? fmtBytes(p.used) + " / " + fmtBytes(p.size) + " (" + Math.round(p.used * 100 / p.size) + " %) · " + fmtBytes(p.size - p.used) + " " + t("flashFree") + " · v" + p.version + state : t("slotEmpty") + " · " + fmtBytes(p.size)]);
    } else rows.push([label, fmtBytes(p.size)]);
  });
  return card(t("ovFlash"), kv(rows));
}

function formatUptime(seconds) {
  const d = Math.floor(seconds / 86400), h = Math.floor(seconds % 86400 / 3600), m = Math.floor(seconds % 3600 / 60);
  return (d ? d + " d " : "") + (h || d ? h + " h " : "") + m + " min";
}
const reasonText = code => (L["reason" + code.charAt(0).toUpperCase() + code.slice(1)] ? t("reason" + code.charAt(0).toUpperCase() + code.slice(1)) : t("reasonOther"));



const linkPill = state => el("span", { class: "pill " + (state === "online" ? "ok" : state === "denied" ? "bad" : state === "stale" ? "warn" : "") }, t("ls_" + state));

function overviewPage() {
  const s = S.status;
  if (!s) return el("p", { class: "muted" }, t("loading"));
  const n = s.network, m = s.mqtt, scr = S.screen;
  const layoutNote = n.ap_setup ? t("setupNetwork") : n.ap_active ? t("ownNetwork") : "";
  const summary = scr && scr.summary, link = scr && scr.link;
  return el("div", { class: "grid" },
    card(t("ovScreen"), link ? [linkPill(link.state), link.action_pending ? el("p", { class: "muted" }, t("actionPending")) : null] : el("p", { class: "muted" }, t("loading")),
      summary && link.have_summary ? kv([[t("mode"), t(summary.mode)], [t("nodesOnline"), summary.nodes_online + " / " + summary.nodes_total], [t("alarmsActive"), summary.alarms_active + " (" + summary.alarms_unacknowledged + " " + t("alarmsUnack") + ")"]]) : null),
    card(t("ovVoice"), scr ? el("p", {}, t("vs_" + scr.voice.state)) : null, el("p", { class: "muted" }, s.sound ? t("soundOk") + " ✓" : t("soundNo"))),
    card(t("ovNode"), kv([[t("nodeId"), s.node_id], [t("nodeName"), s.name], [t("firmware"), s.version + " (" + s.partition + ")"], [t("uptime"), formatUptime(s.uptime_s)],
      [t("resetReason"), reasonText(s.reset_reason)], [t("memory"), Math.round(s.heap_free / 1024) + " kB" + (s.psram_free ? " + " + Math.round(s.psram_free / 1048576 * 10) / 10 + " MB PSRAM" : "")], [t("board"), s.board]])),
        s.time ? card(t("ovTime"), kv([[t("localTime"), s.time.set ? s.time.local + " (" + utcLabel(s.time.utc_offset_min) + ")" : t("clockNotSet")],
          [t("timeSource"), !s.time.set ? "—" : s.time.synced ? t("timeFromNtp") : s.time.ntp ? t("timeNtpWaiting") : t("timeManual")], [t("timeZone"), zoneName(s.time.zone)]])) : null,
        s.flash ? flashCard(s.flash) : null,
        s.hardware ? card(t("ovHardware"), kv([[t("chip"), s.hardware.chip.toUpperCase() + " rev " + s.hardware.chip_revision + " · " + s.hardware.cores + " " + t("cores")],
      [t("flash"), s.hardware.flash_mb + " MB"], [t("psram"), s.hardware.psram_mb ? s.hardware.psram_mb + " MB" : t("none")],
      [t("appIdf"), s.hardware.app_idf], [t("bootloaderIdf"), s.hardware.bootloader_idf]])) : null,
    card(t("ovNetwork"), kv([[t("layout"), n.layout], [t("link"), n.link_up ? t("linkUp") : t("linkDown")], [t("address"), n.has_ip ? n.ip : "—"], [t("netmask"), n.netmask || "—"], [t("gateway"), n.gateway || "—"],
      [t("dns"), n.dns || "—"], [t("mac"), n.mac],
      n.ap_active ? [t("apActive"), "“" + n.ap_ssid + "” · " + t("channel") + " " + n.ap_channel + " · " + n.ap_clients + " " + t("apClients") + " · " + layoutNote] : null,
      n.sta_ssid ? [t("station"), n.sta_ssid + (n.sta_connected ? " · " + n.sta_rssi + " dBm" : " · " + t("notConnected"))] : null])),
    card(t("ovBroker"), kv([[t("navBroker"), !m.enabled ? t("notConfigured") : m.connected ? t("connected") : t("notConnected")], [t("clock"), m.clock_set ? t("clockSet") : t("clockNotSet")], [t("messages"), m.published],
      m.dropped ? [t("mqDropped"), m.dropped] : null])));
}

const hasEthernet = () => !!(S.session && S.session.ethernet);
const isWired = () => hasEthernet() && S.cfg.uplink === "ethernet";

function networkPage() {
  const cfg = S.cfg, wired = isWired();
  return el("div", { class: "grid wide" },
    hasEthernet() ? card(t("netTitle"),
      field("uplink", "uplink", { type: "select", rerender: true, options: [["ethernet", t("uplinkEthernet")], ["wifi", t("uplinkWifi")]] }),
      wired ? field("dhcp", "ip.dhcp", { type: "checkbox", rerender: true }) : null,
      wired && !cfg.ip.dhcp ? el("div", { class: "row" }, field("address", "ip.address"), field("netmask", "ip.netmask"), field("gateway", "ip.gateway"), field("dns1", "ip.dns1"), field("dns2", "ip.dns2")) : null,
      S.status && S.status.network && S.status.network.ethernet_ok === false ? note(t("ethernetMissing"), "bad") : null,
      note(t("wiredNote"), "info")) : null,
    card(t("nodeTitle"), field("hostname", "node.hostname", { hint: t("hostnameHint") }), field("nodeName", "node.name"), field("nodeId", "node.id"), note(t("netRestartNote"), "info")),
    card(t("webTitle"), field("webMode", "web.mode", { type: "select", options: [["both", t("webBoth")], ["https", t("webHttps")], ["http", t("webHttp")]] }),
      S.status && S.status.web ? el("p", { class: "muted" }, t(S.status.web.https ? "webRunning" : "webNotRunning")) : null,
      S.status && S.status.web && S.status.web.cert_sha256 ? el("p", { class: "muted mono" }, t("webFingerprint") + ": " + S.status.web.cert_sha256) : null, note(t("webNote"), "info")),
    card(t("clockTitle"),
      field("timeZone", "time.zone", { type: "select", options: TIME_ZONES.some(z => z[0] === S.cfg.time.zone) ? TIME_ZONES.map(z => z) : [[S.cfg.time.zone, S.cfg.time.zone]].concat(TIME_ZONES) }),
      field("ntpEnabled", "time.ntp_enabled", { type: "checkbox", rerender: true }),
      cfg.time.ntp_enabled ? field("ntp", "time.ntp") : el("div", { class: "actions" }, el("button", { class: "b", tip: "setTimeFromBrowser", disabled: !isAdmin(), onclick: setClockFromBrowser }, t("setTimeFromBrowser"))),
      S.status && S.status.time ? el("p", { class: "muted" }, t("localTime") + ": " + (S.status.time.set ? S.status.time.local : t("clockNotSet"))) : null,
      note(t("clockNote"), "info")),
    card(t("bleTitle"), field("bleMode", "ble.mode", { type: "select", options: [["setup", t("bleSetup")], ["always", t("bleAlways")], ["off", t("bleOff")]] }), note(t("bleNote"), "info")),
    card(t("systemTitle"), field("autoRestart", "system.auto_restart_hours", { type: "select", number: true,
      options: [[0, t("autoRestartNever")], [1, t("autoRestart1")], [2, t("autoRestart2")], [3, t("autoRestart3")], [4, t("autoRestart4")], [6, t("autoRestart6")], [12, t("autoRestart12")], [24, t("autoRestart24")], [48, t("autoRestart48")]] }),
      note(t("autoRestartNote"), "info")),
    isAdmin() ? card(t("configBackupTitle"),
      el("div", { class: "row" },
        el("button", { class: "b", tip: "exportConfig", onclick: exportConfig }, t("exportConfig")),
        el("label", { class: "b", tip: "importConfig" }, t("importConfig"), el("input", { type: "file", accept: "application/json", hidden: true, onchange: e => { if (e.target.files[0]) importConfig(e.target.files[0]); e.target.value = ""; } }))),
      note(t("configBackupNote"), "info")) : null);
}

async function scanNetworks() {
  S.scan = { busy: true, list: null, error: "" }; render();
  const r = await api("GET", "wifi/scan");
  S.scan = r.ok ? { busy: false, list: r.data.networks || [], error: "" } : { busy: false, list: null, error: errorText(r.data.error) };
  render();
}

function scanResults() {
  const scan = S.scan;
  if (scan.busy) return el("p", { class: "muted" }, t("scanning"));
  if (scan.error) return el("p", { class: "err" }, scan.error);
  if (!scan.list) return null;
  if (!scan.list.length) return el("p", { class: "muted" }, t("noNetworks"));
  return el("div", { class: "scroll" }, el("table", {}, el("thead", {}, el("tr", {}, el("th", {}, t("ssid")), el("th", {}, t("signal")), el("th", {}, t("wifiChannel")), el("th", {}, t("security")), el("th", {}))),
    el("tbody", {}, scan.list.map(n => el("tr", {}, el("td", {}, n.ssid), el("td", { class: "mono" }, n.rssi + " dBm"), el("td", {}, n.channel), el("td", {}, n.security),
      el("td", {}, el("button", { class: "b", disabled: !isAdmin(), onclick: () => { setValue("sta.enabled", true); setValue("sta.ssid", n.ssid); render(); } }, t("useNetwork"))))))));
}

function wifiPage() {
  const cfg = S.cfg, ap = cfg.ap;
  const channels = [[0, t("channelAuto")]].concat(Array.from({ length: 13 }, (_, i) => [i + 1, String(i + 1)]));
  return el("div", { class: "grid wide" },
    card(t("wifiAp"),
      field("apEnable", "ap.enabled", { type: "checkbox", rerender: true }),
      ap.enabled ? [
        el("div", { class: "row" }, field("ssid", "ap.ssid", { max: 32 }),
          field("security", "ap.security", { type: "select", rerender: true, options: [["wpa2", t("secWpa2")], ["wpa3", t("secWpa3")], ["wpa2wpa3", t("secWpa2Wpa3")], ["open", t("secOpen")]] })),
        ap.security !== "open" ? field("wifiPassword", "ap.password", { type: "password" }) : null,
        el("div", { class: "row" },
          field("wifiChannel", "ap.channel", { type: "select", number: true, options: channels, hint: ap.channel === 0 ? t("channelNow", S.channelAuto) : "" }),
          field("maxClients", "ap.max_clients", { type: "number", min: 1, max: 10 }), field("txPower", "ap.tx_power_dbm", { type: "number", min: 2, max: 20 }),
          field("bandwidth", "ap.bandwidth_mhz", { type: "select", number: true, options: [[20, t("bw20")], [40, t("bw40")]] }), field("country", "ap.country", { max: 2 })),
        field("hidden", "ap.hidden", { type: "checkbox" }),
        note(t("noWifiWarn"), "info")] : null),
    isWired() ? null : card(t("wifiSta"), field("staEnable", "sta.enabled", { type: "checkbox", rerender: true }),
      cfg.sta.enabled ? [el("div", { class: "row" }, field("ssid", "sta.ssid", { max: 32 }), field("wifiPassword", "sta.password", { type: "password" }))] : null,
      el("div", { class: "actions" }, el("button", { class: "b", tip: "scanNetworks", disabled: !isAdmin() || S.scan.busy, onclick: scanNetworks }, t("scanNetworks"))), scanResults(),
      el("p", { class: "hint" }, t("scanNote")), note(t("staNote"), "info"),
      cfg.sta.enabled ? backupNetworks() : null));
}

const MAX_BACKUP_NETWORKS = 3;
function backupNetworks() {
  const list = S.cfg.sta.backup;
  return el("div", {}, el("h3", {}, t("backupNetworksTitle")), el("p", { class: "hint" }, t("backupNetworksHint")),
    list.map((network, i) => {
      const base = "sta.backup." + i + ".";
      return el("div", { class: "row" }, field("ssid", base + "ssid", { max: 32 }), field("wifiPassword", base + "password", { type: "password" }),
        el("button", { class: "b danger", tip: "removeNetwork", disabled: !isAdmin(), onclick: () => { list.splice(i, 1); refreshBar(); render(); } }, t("removeNetwork")));
    }),
    isAdmin() && list.length < MAX_BACKUP_NETWORKS
      ? el("div", { class: "actions" }, el("button", { class: "b", tip: "addBackupNetwork", onclick: () => { list.push({ ssid: "", password: "" }); refreshBar(); render(); } }, t("addBackupNetwork")))
      : null);
}

const MAX_BACKUP_BROKERS = 2;
function backupBrokers() {
  const list = S.cfg.mqtt.backup;
  return el("div", {}, el("h3", {}, t("backupBrokersTitle")), el("p", { class: "hint" }, t("backupBrokersHint")),
    list.map((broker, i) => {
      const base = "mqtt.backup." + i + ".";
      return el("div", {}, el("div", { class: "row" }, field("brokerUri", base + "uri", { placeholder: "mqtt://192.168.0.180:18883" }),
        el("button", { class: "b danger", tip: "removeNetwork", disabled: !isAdmin(), onclick: () => { list.splice(i, 1); refreshBar(); render(); } }, t("removeNetwork"))),
        el("div", { class: "row" }, field("brokerUser", base + "username"), field("brokerPassword", base + "password", { type: "password" })));
    }),
    isAdmin() && list.length < MAX_BACKUP_BROKERS
      ? el("div", { class: "actions" }, el("button", { class: "b", tip: "addBackupBroker", onclick: () => { list.push({ uri: "", username: "", password: "" }); refreshBar(); render(); } }, t("addBackupBroker")))
      : null);
}

function brokerPage() {
  const cfg = S.cfg, mq = cfg.mqtt;
  return el("div", { class: "grid wide" },
    card(t("brokerTitle"), field("brokerEnable", "mqtt.enabled", { type: "checkbox", rerender: true }),
      mq.enabled ? [field("brokerUri", "mqtt.uri", { placeholder: "mqtt://192.168.0.180:18883" }),
        el("div", { class: "row" }, field("brokerUser", "mqtt.username"), field("brokerPassword", "mqtt.password", { type: "password" })),
        el("div", { class: "row" }, field("heartbeat", "mqtt.heartbeat_s", { type: "number", min: 2, max: 300 })),
        backupBrokers()] : null,
      note(t("brokerNote"), "info")));
}

// ---- the link to the server, the screen, the sound and the voice -----------------------------------------------------------------------------

function serverPage() {
  const sv = S.cfg.server;
  return el("div", { class: "grid wide" },
    card(t("serverTitle"), field("serverEnable", "server.enabled", { type: "checkbox", rerender: true }),
      sv.enabled ? [
        el("div", { class: "row" }, field("serverHost", "server.host", { max: 64, placeholder: "192.168.0.180" }), field("serverPort", "server.port", { type: "number", min: 1, max: 65535 })),
        field("serverTls", "server.tls", { type: "checkbox", rerender: true }),
        sv.tls ? field("serverInsecure", "server.insecure", { type: "checkbox" }) : null,
        el("div", { class: "row" }, field("serverUser", "server.user", { max: 48 }), field("serverPassword", "server.password", { type: "password" })),
        field("serverPoll", "server.poll_s", { type: "number", min: 1, max: 60 })] : null,
      S.screen ? linkPill(S.screen.link.state) : null,
      note(t("serverNote"), "info")));
}

function screenPage() {
  return el("div", { class: "grid wide" },
    card(t("screenTitle"), el("div", { class: "row" }, field("brightness", "display.brightness", { type: "number", min: 5, max: 100 }), field("sleepAfter", "display.sleep_s", { type: "number", min: 0, max: 3600 })),
      el("div", { class: "row" }, field("nightBrightness", "display.night_brightness", { type: "number", min: 1, max: 100 }), field("nightFrom", "display.night_from", { type: "number", min: 0, max: 23 }), field("nightTo", "display.night_to", { type: "number", min: 0, max: 23 })),
      note(t("screenNote"), "info")),
    card(t("soundTitle"), field("volume", "audio.volume", { type: "number", min: 0, max: 100 }), field("alarmSound", "audio.alarm_sound", { type: "checkbox" })));
}

function voicePage() {
  const v = S.cfg.voice;
  return el("div", { class: "grid wide" },
    card(t("voiceTitle"), field("voiceEnable", "voice.enabled", { type: "checkbox", rerender: true }),
      v.enabled ? [field("voiceUrl", "voice.url", { max: 160, placeholder: "http://192.168.0.180:8765" }),
        el("div", { class: "row" }, field("voiceWake", "voice.wake_name", { max: 24 }), field("voiceListen", "voice.listen_s", { type: "number", min: 2, max: 30 }))] : null,
      S.screen ? el("p", { class: "muted" }, t("vs_" + S.screen.voice.state)) : null,
      note(t("voiceNote"), "info")));
}

// ---- users and system ------------------------------------------------------------------------------------------------------------------

const usersMessage = { text: "", kind: "" };
async function usersAction(promise, done) {
  const r = await promise;
  usersMessage.text = r.ok ? (done || t("saved")) : errorText(r.data.error); usersMessage.kind = r.ok ? "ok" : "bad";
  const list = await api("GET", "users");
  if (list.ok) S.users = list.data;
  render();
  return r;
}

function usersPage() {
  const form = { name: "", password: "", role: "viewer" }, mine = { current: "", password: "" };
  const rows = S.users.map(u => {
    const pw = el("input", { type: "password", placeholder: t("newPassword"), autocomplete: "new-password" });
    return el("tr", {}, el("td", {}, u.name, u.name === S.session.user ? el("span", { class: "hint" }, " (" + t("you") + ")") : null),
      el("td", {}, el("select", { onchange: e => usersAction(api("PUT", "users/" + u.name, { role: e.target.value })) },
        [["admin", t("roleAdmin")], ["viewer", t("roleViewer")]].map(([v, text]) => el("option", { value: v, selected: u.role === v }, text)))),
      el("td", {}, pw), el("td", { class: "actions" }, el("button", { class: "b", onclick: () => usersAction(api("PUT", "users/" + u.name, { password: pw.value })) }, t("setPassword")),
        el("button", { class: "b danger", onclick: () => confirm(t("confirmAsk")) && usersAction(api("DELETE", "users/" + u.name)) }, t("delete"))));
  });
  return el("div", { class: "grid wide" },
    isAdmin() ? card(t("usersTitle"), el("div", { class: "scroll" }, el("table", {}, el("thead", {}, el("tr", {}, el("th", {}, t("user")), el("th", {}, t("role")), el("th", {}, t("newPassword")), el("th", {}))), el("tbody", {}, rows))),
      el("h3", {}, t("addUser")),
      el("div", { class: "row" }, el("label", { class: "field" }, el("span", {}, t("user")), el("input", { oninput: e => { form.name = e.target.value; } })),
        el("label", { class: "field" }, el("span", {}, t("newPassword")), el("input", { type: "password", autocomplete: "new-password", oninput: e => { form.password = e.target.value; } })),
        el("label", { class: "field" }, el("span", {}, t("role")), el("select", { onchange: e => { form.role = e.target.value; } }, [["viewer", t("roleViewer")], ["admin", t("roleAdmin")]].map(([v, text]) => el("option", { value: v }, text))))),
      el("div", { class: "actions" }, el("button", { class: "b primary", onclick: () => usersAction(api("POST", "users", form)) }, t("add"))),
      usersMessage.text ? el("p", { class: usersMessage.kind === "ok" ? "hint" : "err" }, usersMessage.text) : null) : null,
    card(t("myAccount"),
      el("label", { class: "field" }, el("span", {}, t("currentPassword")), el("input", { type: "password", autocomplete: "current-password", oninput: e => { mine.current = e.target.value; } })),
      el("label", { class: "field" }, el("span", {}, t("newPassword")), el("input", { type: "password", autocomplete: "new-password", oninput: e => { mine.password = e.target.value; } })),
      el("div", { class: "actions" }, el("button", { class: "b primary", onclick: async () => {
        const r = await api("PUT", "account", mine);
        if (r.ok) { alert(t("passwordChanged")); S.session.authenticated = false; start(); } else { usersMessage.text = errorText(r.data.error); usersMessage.kind = "bad"; render(); }
      } }, t("changePassword"))), !isAdmin() && usersMessage.text ? el("p", { class: "err" }, usersMessage.text) : null));
}

function uploadFirmware(file, progressBar, label, done) {
  const request = new XMLHttpRequest();
  request.open("POST", "/api/v1/ota");
  request.setRequestHeader("X-Requested-With", "armor");
  request.upload.onprogress = e => { if (e.lengthComputable) { const pct = Math.round(e.loaded * 100 / e.total); progressBar.style.width = pct + "%"; label.textContent = t("uploading", pct); } };
  request.onload = () => {
    let data = {};
    try { data = JSON.parse(request.responseText); } catch (error) { /* keep {} */ }
    done(request.status === 200 ? { ok: true, version: data.version } : { ok: false, error: data.error || "network" });
  };
  request.onerror = () => done({ ok: false, error: "network" });
  request.send(file);
}

function aboutPage() {
  const s = S.status;
  return el("div", { class: "grid wide" },
    card(null,
      el("p", { class: "eyebrow" }, "AUTONOMOUS RADAR & MULTIMODAL OBSERVATION RANGE"),
      el("h2", {}, "A.R.M.O.R. HMI"),
      el("p", {}, t("aboutDescription")),
      kv([[t("nodeId"), s ? s.node_id : "—"], [t("nodeName"), s ? s.name : "—"], [t("firmware"), s ? s.version : "—"],
        [t("author"), "JuanenRac · Electro Hobby 3D"], [t("license"), "GPL-3.0-or-later"]]),
      el("p", { class: "muted mono" }, "github.com/JuanenRac/ARMOR-HMI")));
}

const HELP_TOPICS = ["overview", "server", "screen", "voice", "network", "wifi", "broker", "users", "update"];
let helpTopic = "overview";
function helpPage() {
  const paragraphs = t("help_" + helpTopic + "_text").split("\n\n");
  return el("div", { class: "help" },
    el("div", { class: "help-nav" }, HELP_TOPICS.map(topic => el("button", { class: topic === helpTopic ? "active" : "", onclick: () => { helpTopic = topic; render(); } }, t("navHelpTab_" + topic)))),
    card(t("help_" + helpTopic + "_title"), ...paragraphs.map(p => el("p", {}, p))));
}

// The two firmware slots and the switch between them: the node boots the other one at the next restart.
function slotCard() {
  const parts = ((S.status && S.status.flash && S.status.flash.partitions) || []).filter(p => p.app);
  const other = parts.find(p => !p.running);
  const usable = !!(other && other.used);
  const result = el("p", { class: "muted" });
  return card(t("slotsTitle"), el("p", { class: "muted" }, t("slotsHelp")),
    kv(parts.map(p => [p.label, p.used ? "v" + p.version + (p.running ? " · " + t("slotRunning") : p.next_boot ? " · " + t("slotNextBoot") : "") : t("slotEmpty")])), result,
    el("div", { class: "actions" }, el("button", { class: "b danger", disabled: !isAdmin() || !usable, onclick: async () => {
      if (!usable || !confirm(t("switchAsk", other.label, other.version))) return;
      const r = await api("POST", "ota/switch", {});
      if (!r.ok) { result.textContent = errorText(r.data.error); result.className = "err"; return; }
      S.rebooting = true; render();
      const wait = async () => { const q = await api("GET", "session"); if (q.ok) location.reload(); else setTimeout(wait, 2000); };
      setTimeout(wait, 4000);
    } }, usable ? t("switchSlot", other.label) : t("switchSlotNone"))));
}

function updatePage() {
  const s = S.status;
  const progress = el("i"), label = el("p", { class: "muted" }), file = el("input", { type: "file", accept: ".bin" });
  const logBox = el("pre", { class: "log", id: "log-box" }, S.log.text);
  const confirmBox = el("input", { placeholder: "RESET" });
  const result = el("p", { class: "muted" });
  return el("div", { class: "grid wide" },
    card(t("updateTitle"), s ? kv([[t("firmware"), s.version], [t("slot"), s.partition]]) : null, el("p", { class: "muted" }, t("updateHelp")),
      el("label", { class: "field" }, el("span", {}, t("chooseFile")), file), el("div", { class: "progress" }, progress), label, result,
      el("div", { class: "actions" }, el("button", { class: "b primary", disabled: !isAdmin(), onclick: () => {
        if (!file.files[0]) return;
        uploadFirmware(file.files[0], progress, label, r => {
          if (r.ok) { result.textContent = t("updateDone", r.version); S.rebooting = true; render(); setTimeout(() => { const wait = async () => { const q = await api("GET", "session"); if (q.ok) location.reload(); else setTimeout(wait, 2000); }; wait(); }, 6000); }
          else { result.textContent = errorText(r.error); result.className = "err"; }
        });
      } }, t("upload")))),
    slotCard(),
    card(t("maintenance"), el("div", { class: "actions" }, el("button", { class: "b", tip: "rebootNode", disabled: !isAdmin(), onclick: () => confirm(t("confirmAsk")) && reboot() }, t("rebootNode"))),
      el("h3", {}, t("factoryTitle")), el("p", { class: "muted" }, t("factoryHelp")),
      el("div", { class: "actions" }, confirmBox, el("button", { class: "b danger", disabled: !isAdmin(), onclick: async () => {
        const r = await api("POST", "factory-reset", { confirm: confirmBox.value });
        if (r.ok) { S.rebooting = true; render(); setTimeout(() => location.reload(), 6000); } else alert(errorText(r.data.error));
      } }, t("factoryTitle")))),
    card(t("logTitle"), logBox, el("div", { class: "actions" }, el("button", { class: "b", onclick: async () => { S.log = { next: 0, text: "" }; await refreshLog(); } }, t("refresh")))));
}

// ---- data, polling and the shell -------------------------------------------------------------------------------------------------------

async function setClockFromBrowser() {
  const r = await api("POST", "time", { epoch: Math.floor(Date.now() / 1000) });
  S.message = r.ok ? { kind: "ok", text: t("timeSetOk") } : { kind: "err", text: errorText(r.data.error) };
  await refreshLive(); render();
}

async function refreshLive() {
  const [status, screen] = await Promise.all([api("GET", "status"), api("GET", "screen")]);
  if (status.ok) S.status = status.data;
  if (screen.ok) S.screen = screen.data;
}

async function refreshLog() {
  const r = await api("GET", "log?from=" + S.log.next);
  if (!r.ok) return;
  S.log.text = (S.log.text + r.data.text).slice(-24000); S.log.next = r.data.next;
  const box = document.getElementById("log-box");
  if (box) { const bottom = box.scrollTop + box.clientHeight >= box.scrollHeight - 30; box.textContent = S.log.text; if (bottom) box.scrollTop = box.scrollHeight; }
}

let pollTimer = null;
async function poll() {
  if (!S.session || !S.session.authenticated || S.rebooting) return;
  await refreshLive();
  if (S.page === "overview") render(false);
  else if (S.page === "update") await refreshLog();
  updatePills();
}

function updatePills() {
  const box = document.getElementById("pills");
  if (!box || !S.status) return;
  const s = S.status, n = s.network;
  box.replaceChildren(...[
    el("span", { class: "pill " + (n.has_ip ? "ok" : "bad") }, n.has_ip ? n.ip : t("linkDown")),
    s.mqtt.enabled ? el("span", { class: "pill " + (s.mqtt.connected ? "ok" : "warn") }, "MQTT " + (s.mqtt.connected ? t("connected") : t("notConnected"))) : null,
    n.ap_active ? el("span", { class: "pill" }, "AP " + n.ap_clients) : null,
    el("span", { class: "pill" }, "v" + s.version)].filter(Boolean));
}

function shell(content) {
  const groups = [...new Set(PAGES.map(p => p.group))];
  const nav = el("nav", { class: "nav" }, groups.map(g => [el("div", { class: "nav-title" }, t(g)),
    PAGES.filter(p => p.group === g).map(p => el("button", { class: p.id === S.page ? "active" : "", tip: p.label, onclick: () => go(p.id) }, el("span", { class: "ico" }, p.icon), t(p.label)))]));
  const langSelect = el("select", { "aria-label": t("language"), onchange: e => { lang = Number(e.target.value); try { localStorage.setItem("armor_lang", LANGS[lang][0]); } catch (error) { /* ignore */ } document.documentElement.lang = LANGS[lang][0]; persistLanguage(); render(); } },
    LANGS.map((l, i) => el("option", { value: String(i), selected: i === lang }, l[1])));
  const page = PAGES.find(p => p.id === S.page);
  barNode = el("div", { class: "bar", hidden: true });
  const view = el("div", { class: "shell" },
    el("aside", { class: "side" }, el("div", { class: "brand" }, el("div", { class: "brand-mark" }, "A"), el("div", {}, el("strong", {}, "A.R.M.O.R."), el("small", {}, S.session.node_id))), nav,
      el("div", { class: "side-foot" }, el("div", {}, el("span", { class: "dot " + (S.status && S.status.network.has_ip ? "ok" : "bad") }), S.session.user + " · " + (S.session.role === "admin" ? t("roleAdmin") : t("roleViewer"))),
        langSelect, el("button", { class: "b", tip: "signOut", onclick: async () => { await api("POST", "logout", {}); S.session.authenticated = false; start(); } }, t("signOut")))),
    el("main", {}, el("header", { class: "top" }, el("div", {}, el("p", { class: "eyebrow" }, t(page.group)), el("h1", {}, t(page.label))), el("div", { class: "pills", id: "pills" })), content),
    barNode);
  return view;
}

function go(id) { S.page = id; S.message = S.message && S.message.kind === "ok" ? null : S.message; location.hash = "#/" + id; }

function render(full = true) {
  if (S.rebooting) { $app.replaceChildren(el("div", { class: "center" }, el("div", { class: "login" }, el("h1", {}, t("restarting")), el("p", { class: "muted" }, t("loading"))))); return; }
  if (!S.session || S.session.setup) { $app.replaceChildren(setupScreen()); return; }
  if (!S.session.authenticated) { $app.replaceChildren(loginScreen()); return; }
  if (!S.cfg) { $app.replaceChildren(el("p", { class: "muted" }, t("loading"))); return; }
  const focus = document.activeElement && document.activeElement.tagName === "INPUT" && !full;
  if (focus) return;
  let content;
  switch (S.page) {
    case "network": content = networkPage(); break;
    case "wifi": content = wifiPage(); break;
    case "broker": content = brokerPage(); break;
    case "server": content = serverPage(); break;
    case "screen": content = screenPage(); break;
    case "voice": content = voicePage(); break;
    case "users": content = usersPage(); break;
    case "update": content = updatePage(); break;
    case "help": content = helpPage(); break;
    case "about": content = aboutPage(); break;
    default: content = overviewPage();
  }
  $app.replaceChildren(shell(content));
  updatePills();
  refreshBar();
}

// ---- login and set-up screens -----------------------------------------------------------------------------------------------------------

// A password field with an eye to show what was typed - the login and set-up screens are the one place a mistyped password locks
// someone out with no other field to cross-check it against.
function passwordField(labelKey, inputAttrs) {
  const input = el("input", Object.assign({ type: "password" }, inputAttrs));
  const toggle = el("button", { type: "button", class: "eye-toggle", "aria-label": t("showPassword"),
    onclick: () => { input.type = input.type === "password" ? "text" : "password"; toggle.textContent = input.type === "password" ? "👁" : "🙈"; } }, "👁");
  return el("label", { class: "field" }, el("span", {}, t(labelKey)), el("div", { class: "password-row" }, input, toggle));
}

function langPicker() {
  return el("select", { "aria-label": t("language"), onchange: e => { lang = Number(e.target.value); try { localStorage.setItem("armor_lang", LANGS[lang][0]); } catch (error) { /* ignore */ } render(); } },
    LANGS.map((l, i) => el("option", { value: String(i), selected: i === lang }, l[1])));
}

function loginScreen() {
  const form = { user: "", password: "", remember: false };
  const message = el("p", { class: "err" });
  const submit = async e => {
    e.preventDefault();
    const r = await api("POST", "login", form);
    if (r.ok) { await start(); return; }
    message.textContent = r.data.error === "too_many_attempts" && r.data.wait_s ? errorText("too_many_attempts") + " (" + r.data.wait_s + " s)" : errorText(r.data.error);
  };
  return el("div", { class: "center" }, el("form", { class: "login", onsubmit: submit },
    el("div", { class: "brand" }, el("div", { class: "brand-mark" }, "A"), el("div", {}, el("strong", {}, "A.R.M.O.R."), el("small", {}, S.session.node_id))), el("h1", {}, t("signIn")),
    el("label", { class: "field" }, el("span", {}, t("user")), el("input", { autocomplete: "username", autofocus: true, oninput: e => { form.user = e.target.value; } })),
    passwordField("password", { autocomplete: "current-password", oninput: e => { form.password = e.target.value; } }),
    el("label", { class: "check" }, el("input", { type: "checkbox", onchange: e => { form.remember = e.target.checked; } }), t("rememberMe")),
    message, el("button", { class: "b primary", type: "submit" }, t("signIn")), langPicker()));
}

function setupScreen() {
  const form = { code: "", user: "admin", password: "", language: LANGS[lang][0], wifi_ssid: "", wifi_password: "" };
  const message = el("p", { class: "err" });
  const submit = async e => {
    e.preventDefault();
    form.language = LANGS[lang][0];
    const r = await api("POST", "setup", form);
    if (r.ok) { S.rebooting = true; render(); setTimeout(() => { const wait = async () => { const q = await api("GET", "session"); if (q.ok) location.reload(); else setTimeout(wait, 2000); }; wait(); }, 6000); return; }
    message.textContent = errorText(r.data.error);
  };
  return el("div", { class: "center" }, el("form", { class: "login", onsubmit: submit },
    el("div", { class: "brand" }, el("div", { class: "brand-mark" }, "A"), el("div", {}, el("strong", {}, "A.R.M.O.R."), el("small", {}, S.session.node_id))), el("h1", {}, t("setupTitle")),
    el("p", { class: "muted" }, t("setupIntro")), S.session.setup_ssid ? el("p", { class: "hint" }, t("setupWifi") + ": " + S.session.setup_ssid) : null,
    S.session.mac ? el("p", { class: "hint mono" }, t("mac") + ": " + S.session.mac) : null,
    el("label", { class: "field" }, el("span", {}, t("setupCode")), el("input", { autocomplete: "off", autocapitalize: "characters", oninput: e => { form.code = e.target.value.trim().toUpperCase(); } })),
    el("label", { class: "field" }, el("span", {}, t("adminName")), el("input", { value: "admin", autocomplete: "username", oninput: e => { form.user = e.target.value; } })),
    passwordField("newPassword", { autocomplete: "new-password", oninput: e => { form.password = e.target.value; } }),
    ...([
      el("h3", {}, t(hasEthernet() ? "setupWifiOptionalTitle" : "setupWifiTitle")), el("p", { class: "hint" }, t(hasEthernet() ? "setupWifiOptionalHelp" : "setupWifiHelp")),
      el("label", { class: "field" }, el("span", {}, t("ssid")), el("input", { autocomplete: "off", maxLength: 32, oninput: e => { form.wifi_ssid = e.target.value; } })),
      passwordField("wifiPassword", { autocomplete: "off", oninput: e => { form.wifi_password = e.target.value; } })]),
    message, el("button", { class: "b primary", type: "submit" }, t("createAdmin")), langPicker()));
}

// ---- start ---------------------------------------------------------------------------------------------------------------------------------

async function start() {
  const r = await api("GET", "session");
  if (!r.ok) { $app.replaceChildren(el("div", { class: "center" }, el("p", { class: "note bad" }, t("unreachable")))); setTimeout(start, 3000); return; }
  S.session = r.data;
  pickLanguage(S.session.language);
  if (S.session.authenticated) {
    await loadConfig();
    await refreshLive();
    const users = isAdmin() ? await api("GET", "users") : null;
    if (users && users.ok) S.users = users.data;
    S.page = (location.hash.replace("#/", "") || "overview");
    if (!PAGES.some(p => p.id === S.page)) S.page = "overview";
    if (S.page === "update") await refreshLog();
  }
  render();
  clearInterval(pollTimer);
  pollTimer = setInterval(poll, 3000);
}

window.addEventListener("hashchange", () => {
  const id = location.hash.replace("#/", "");
  if (PAGES.some(p => p.id === id) && S.session && S.session.authenticated) { S.page = id; if (id === "update") refreshLog(); render(); }
});
start();
