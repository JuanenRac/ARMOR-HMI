#!/usr/bin/env node
/**
 * ARMOR-HMI - a stand-in for the touch panel, to work on its web page without a board.
 * Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
 *
 *   node tools/panel_mock.mjs [port]          (default 8090)     open http://127.0.0.1:8090/
 *
 * It serves the panel from panel/ (text.js is joined in front of app.js, as tools/pack_panel.py does) and answers /api/v1 like the firmware
 * does, from memory: a fresh mock is in set-up (code TESTCODE); `--user admin:adminpass123` starts it with an administrator. It is a development
 * tool: it checks far less than the firmware, and its numbers are made up. The screen answers as the firmware's /api/v1/screen does: online, with an armed system and two alarms.
 */
import { createServer } from "node:http";
import { readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const panel = path.join(here, "..", "panel");
const port = Number(process.argv.find(a => /^\d+$/.test(a)) ?? 8090);
// --board s3-eth: the mock plays the Waveshare ESP32-S3-ETH (a cable, the W5500 pins reserved); the default is the s3-wifi board
const board = "lcd7box";
const wired = false;
const seeded = process.argv.includes("--user") ? process.argv[process.argv.indexOf("--user") + 1] : "";

const users = new Map();
if (seeded) { const [name, password] = seeded.split(":"); users.set(name, { password, role: "admin" }); }
const sessions = new Map();
const started = Date.now();
let rebootAt = 0;
let logText = "I (1200) armor-hmi: A.R.M.O.R. touch panel hmi-a1b2c3, firmware 0.0.2\nI (1500) armor-net: link up\nI (2600) armor-net: address 192.168.0.181, gateway 192.168.0.1, netmask 255.255.255.0 (wire)\nW (9000) armor-link: signed in to 192.168.0.180 as \"panel-salon\"\n";

const config = {
  v: 1, node: { id: "hmi-a1b2c3", name: "Living room panel", hostname: "" },
  uplink: "wifi", ip: { dhcp: true, address: "", netmask: "255.255.255.0", gateway: "", dns1: "", dns2: "" },
  ap: { enabled: true, ssid: "ARMOR-HMI-A1B2C3", security: "wpa2", password_set: true, channel: 0, hidden: false, max_clients: 8, tx_power_dbm: 15, bandwidth_mhz: 20, country: "ES" },
  sta: { enabled: true, ssid: "HomeRouter", password_set: true },
  mqtt: { enabled: true, uri: "mqtt://192.168.0.180:18883", username: "hmi-node-hmi-a1b2c3", password_set: true, heartbeat_s: 10, ntp: "pool.ntp.org" },
  server: { enabled: true, host: "192.168.0.180", port: 18080, tls: false, insecure: false, user: "panel-salon", password_set: true, poll_s: 3 },
  display: { brightness: 80, sleep_s: 300, night_brightness: 15, night_from: 22, night_to: 7 },
  audio: { volume: 60, alarm_sound: true },
  voice: { enabled: false, url: "", wake_name: "armor", listen_s: 8 },
  web: { mode: "both" },
  ble: { mode: "setup" },
  ui: { language: "en" },
};

const screen = () => ({
  link: { state: "online", have_summary: true, action_pending: false, action_error: "" },
  summary: { mode: "armed", nodes_online: 3, nodes_total: 4, alarms_active: 2, alarms_unacknowledged: 1,
    alarms: [{ id: "a1", code: "intrusion", acknowledged: false }, { id: "a2", code: "low_battery", acknowledged: true }] },
  voice: { state: config.voice.enabled ? "idle" : "off", failure: "" },
});

const json = (response, code, body, headers = {}) => { response.writeHead(code, { "Content-Type": "application/json", "Cache-Control": "no-store", ...headers }); response.end(JSON.stringify(body)); };
const bodyOf = request => new Promise(resolve => { const chunks = []; request.on("data", c => chunks.push(c)); request.on("end", () => resolve(Buffer.concat(chunks))); });
const tokenOf = request => /armor_session=([0-9a-f]+)/.exec(request.headers.cookie ?? "")?.[1] ?? "";

function status() {
  return {
    node_id: config.node.id, name: config.node.name, version: "0.0.2", uptime_s: Math.floor((Date.now() - started) / 1000) + 5400, reset_reason: "power_on", heap_free: 182000, heap_min: 151000, psram_free: 14400000, partition: "ota_0",
    network: { board, ethernet_available: wired, ethernet_ok: true, layout: wired ? "ethernet+ap" : "wifi-station+ap", link_up: true, has_ip: true, ip: "192.168.0.181", netmask: "255.255.255.0", gateway: "192.168.0.1", dns: "192.168.0.1", mac: "34:85:18:a1:b2:c3",
      ap_active: config.ap.enabled, ap_setup: users.size === 0, ap_ssid: users.size === 0 ? "ARMOR-SETUP-A1B2C3" : config.ap.ssid, ap_channel: 6, ap_clients: 1, sta_connected: true, sta_ssid: config.sta.ssid, sta_rssi: -52 },
    mqtt: { enabled: config.mqtt.enabled, connected: true, clock_set: true, published: 400 + Math.floor((Date.now() - started) / 5000), dropped: 0 },
    web: { mode: config.web.mode, https: config.web.mode !== "http", cert_sha256: "a3f1c07d9e2b4c58a7106f3de9b2c4815d6e7f80a1b2c3d4e5f60718293a4b5c" },
    board: "Waveshare ESP32-S3-Touch-LCD-7C-BOX", sound: true,
  };
}

const problems = doc => {
  const list = [];
  if (doc.ap?.enabled && !String(doc.ap.ssid ?? "").trim()) list.push({ path: "ap.ssid", code: "required" });
  if (doc.ap?.enabled && doc.ap.security !== "open" && doc.ap.password !== undefined && doc.ap.password.length > 0 && doc.ap.password.length < 8) list.push({ path: "ap.password", code: "invalid_key" });
  if (doc.mqtt?.enabled && !/^mqtts?:\/\/.+/.test(doc.mqtt.uri ?? "")) list.push({ path: "mqtt.uri", code: "invalid" });
  if (doc.server?.enabled && !String(doc.server.host ?? config.server.host).trim()) list.push({ path: "server.host", code: "required" });
  if (doc.server?.insecure && doc.server.tls === false) list.push({ path: "server.insecure", code: "needs_tls" });
  if (doc.display?.brightness !== undefined && !(doc.display.brightness >= 5 && doc.display.brightness <= 100)) list.push({ path: "display.brightness", code: "range" });
  if (doc.audio?.volume !== undefined && !(doc.audio.volume >= 0 && doc.audio.volume <= 100)) list.push({ path: "audio.volume", code: "range" });
  if (doc.voice?.enabled && !/^https?:\/\/[^/@]+\/?$/.test(doc.voice.url ?? config.voice.url)) list.push({ path: "voice.url", code: doc.voice.url ? "invalid" : "required" });
  return list;
};

const server = createServer(async (request, response) => {
  const url = new URL(request.url, "http://x");
  if (request.method === "GET" && ["/", "/index.html"].includes(url.pathname)) { response.writeHead(200, { "Content-Type": "text/html" }); return response.end(readFileSync(path.join(panel, "index.html"))); }
  if (request.method === "GET" && url.pathname === "/style.css") { response.writeHead(200, { "Content-Type": "text/css" }); return response.end(readFileSync(path.join(panel, "style.css"))); }
  if (request.method === "GET" && url.pathname === "/app.js") { response.writeHead(200, { "Content-Type": "text/javascript" }); return response.end(readFileSync(path.join(panel, "text.js"), "utf8") + "\n" + readFileSync(path.join(panel, "app.js"), "utf8")); }
  if (!url.pathname.startsWith("/api/v1/")) return json(response, 404, { error: "not_found" });

  const route = url.pathname.slice(8), method = request.method;
  const raw = await bodyOf(request);
  let body = {};
  if (raw.length && !url.pathname.endsWith("/ota")) { try { body = JSON.parse(raw.toString("utf8")); } catch { return json(response, 400, { error: "not_json" }); } }
  const session = sessions.get(tokenOf(request));
  const setup = users.size === 0;

  if (method === "GET" && route === "session") return json(response, 200, { setup, authenticated: !!session, user: session?.user ?? "", role: session?.role ?? "", node_id: config.node.id, language: config.ui.language, version: "0.2.3", setup_ssid: setup ? "ARMOR-SETUP-A1B2C3" : "", mac: "34:85:18:a1:b2:c3", board, ethernet: wired });
  if (method === "POST" && route === "setup") {
    if (!setup) return json(response, 403, { error: "forbidden" });
    if (body.code !== "TESTCODE") return json(response, 403, { error: "wrong_code" });
    if (!/^[a-z0-9_.-]{3,32}$/.test(body.user ?? "")) return json(response, 422, { error: "invalid_name" });
    if ((body.password ?? "").length < 8) return json(response, 422, { error: "weak_password" });
    users.set(body.user, { password: body.password, role: "admin" });
    return json(response, 200, { ok: true, restart_required: true });
  }
  if (method === "POST" && route === "login") {
    const user = users.get(body.user);
    if (!user || user.password !== body.password) return json(response, 401, { error: "wrong_credentials" });
    const token = [...Array(48)].map(() => "0123456789abcdef"[Math.floor(Math.random() * 16)]).join("");
    sessions.set(token, { user: body.user, role: user.role });
    return json(response, 200, { ok: true, restart_required: false }, { "Set-Cookie": `armor_session=${token}; Path=/; HttpOnly; SameSite=Strict` });
  }
  if (method === "POST" && route === "logout") { sessions.delete(tokenOf(request)); return json(response, 200, { ok: true }); }
  if (setup) return json(response, 403, { error: "setup_required" });
  if (!session) return json(response, 401, { error: "unauthorized" });
  if (method !== "GET" && request.headers["x-requested-with"] !== "armor") return json(response, 403, { error: "forbidden" });
  const admin = session.role === "admin";
  const needAdmin = () => { if (!admin) { json(response, 403, { error: "forbidden" }); return false; } return true; };

  if (method === "GET" && route === "status") return json(response, 200, status());
  if (method === "GET" && route === "wifi/scan") { if (!needAdmin()) return; return json(response, 200, { networks: [{ ssid: "HomeRouter", rssi: -48, channel: 6, security: "wpa2" }, { ssid: "Neighbour", rssi: -71, channel: 11, security: "wpa2wpa3" }, { ssid: "CafeOpen", rssi: -80, channel: 1, security: "open" }] }); }
  if (method === "GET" && route === "config") return json(response, 200, { config, channel_auto: 6, firmware: "0.2.3" });
  if (method === "PUT" && route === "config") {
    if (!needAdmin()) return;
    const list = problems(body);
    if (list.length) return json(response, 422, { error: "invalid", problems: list });
    for (const key of Object.keys(body)) {
      const value = body[key];
      if (value && typeof value === "object" && !Array.isArray(value) && config[key] && typeof config[key] === "object") Object.assign(config[key], value);
      else config[key] = value;
    }
    for (const section of [config.ap, config.sta, config.mqtt, config.server]) if (typeof section.password === "string" && section.password) { section.password_set = true; delete section.password; } else delete section.password;
    return json(response, 200, { ok: true, restart_required: true });
  }
  if (method === "GET" && route === "screen") return json(response, 200, screen());
  if (method === "GET" && route === "users") { if (!needAdmin()) return; return json(response, 200, [...users].map(([name, u]) => ({ name, role: u.role }))); }
  if (method === "POST" && route === "users") {
    if (!needAdmin()) return;
    if (!/^[a-z0-9_.-]{3,32}$/.test(body.name ?? "")) return json(response, 422, { error: "invalid_name" });
    if ((body.password ?? "").length < 8) return json(response, 422, { error: "weak_password" });
    if (users.has(body.name)) return json(response, 409, { error: "exists" });
    users.set(body.name, { password: body.password, role: body.role === "admin" ? "admin" : "viewer" });
    return json(response, 200, { ok: true, restart_required: false });
  }
  if (method === "PUT" && route.startsWith("users/")) {
    if (!needAdmin()) return;
    const user = users.get(route.slice(6));
    if (!user) return json(response, 404, { error: "not_found" });
    if (body.role) user.role = body.role;
    if (body.password) user.password = body.password;
    return json(response, 200, { ok: true, restart_required: false });
  }
  if (method === "DELETE" && route.startsWith("users/")) {
    if (!needAdmin()) return;
    const name = route.slice(6);
    if (users.get(name)?.role === "admin" && [...users.values()].filter(u => u.role === "admin").length === 1) return json(response, 409, { error: "last_admin" });
    users.delete(name);
    return json(response, users.has(name) ? 500 : 200, { ok: true, restart_required: false });
  }
  if (method === "PUT" && route === "account") {
    const user = users.get(session.user);
    if (user.password !== body.current) return json(response, 403, { error: "wrong_password" });
    if ((body.password ?? "").length < 8) return json(response, 422, { error: "weak_password" });
    user.password = body.password;
    return json(response, 200, { ok: true, restart_required: false });
  }
  if (method === "GET" && route === "log") return json(response, 200, { next: logText.length, text: logText.slice(Number(url.searchParams.get("from") ?? 0)) });
  if (method === "POST" && route === "ota/switch") { if (!needAdmin()) return; rebootAt = Date.now(); return json(response, 200, { ok: true, restart_required: true, slot: "ota_1", version: "0.0.0" }); }
  if (method === "POST" && route === "reboot") { if (!needAdmin()) return; rebootAt = Date.now(); return json(response, 200, { ok: true, restart_required: false }); }
  if (method === "POST" && route === "factory-reset") { if (!needAdmin()) return; if (body.confirm !== "RESET") return json(response, 422, { error: "confirm_required" }); users.clear(); return json(response, 200, { ok: true, restart_required: true }); }
  if (method === "POST" && route === "ota") { if (!needAdmin()) return; return json(response, 200, { ok: true, restart_required: true, version: "0.2.4", bytes: raw.length, sha256: "0".repeat(64) }); }
  return json(response, 404, { error: "not_found" });
});
server.listen(port, "127.0.0.1", () => console.log(`ARMOR-HMI panel mock on http://127.0.0.1:${port}/ (${users.size ? "signed-in users: " + [...users.keys()].join(", ") : "set-up code TESTCODE"})`));
