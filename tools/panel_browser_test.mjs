// ARMOR-HMI - the web page in a real browser (headless Microsoft Edge, its own temporary profile) against the stand-in panel of tools/panel_mock.mjs: sign in, every page in the
// seven languages without a script error, the overview with what the screen shows, and a setting saved from the page. Development tool; it needs Edge, and `ws` from ARMOR-SERVER's
// node_modules.
//   node tools/panel_browser_test.mjs
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
import { spawn } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, "..");
const require = createRequire(path.resolve(ROOT, "..", "ARMOR-SERVER", "package.json"));
const WebSocket = require("ws");
const OUT = path.join(os.tmpdir(), "armor-hmi-shots");
fs.mkdirSync(OUT, { recursive: true });
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
const tmp = fs.mkdtempSync(path.join(os.tmpdir(), "armor-hmi-panel-"));
const mock = spawn("node", ["tools/panel_mock.mjs", "18132", "--user", "admin:adminpass123"], { cwd: ROOT, stdio: "ignore" });
let edge;
const cleanup = () => { for (const child of [mock, edge]) try { child?.kill(); } catch { /* ignore */ } };
process.on("exit", cleanup);
const results = [];
const check = (name, ok, extra = "") => { results.push(ok); console.log(ok ? "PASS" : "FAIL", name, ok ? "" : extra); };

try {
  await sleep(1500);
  const login = await fetch("http://127.0.0.1:18132/api/v1/login", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ user: "admin", password: "adminpass123" }) });
  const cookie = login.headers.getSetCookie()[0].split(";")[0];
  const [cname, cvalue] = [cookie.slice(0, cookie.indexOf("=")), cookie.slice(cookie.indexOf("=") + 1)];
  edge = spawn("C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe", ["--headless=new", "--remote-debugging-port=9367", "--user-data-dir=" + path.join(tmp, "edge"), "--no-first-run", "--disable-gpu", "about:blank"], { stdio: "ignore" });
  let targets;
  for (let i = 0; i < 40; i += 1) { try { targets = await (await fetch("http://127.0.0.1:9367/json")).json(); if (targets.length) break; } catch { /* wait */ } await sleep(300); }
  const ws = new WebSocket(targets.find(target => target.type === "page").webSocketDebuggerUrl);
  await new Promise(resolve => ws.on("open", resolve));
  let id = 0;
  const waiting = new Map(), errors = [];
  ws.on("message", raw => {
    const message = JSON.parse(raw);
    if (message.id && waiting.has(message.id)) { waiting.get(message.id)(message.result ?? message.error); waiting.delete(message.id); }
    if (message.method === "Runtime.exceptionThrown") errors.push(message.params.exceptionDetails.exception?.description ?? message.params.exceptionDetails.text);
    if (message.method === "Runtime.consoleAPICalled" && message.params.type === "error") errors.push(message.params.args.map(arg => arg.value ?? arg.description).join(" "));
  });
  const send = (method, params = {}) => new Promise(resolve => { id += 1; waiting.set(id, resolve); ws.send(JSON.stringify({ id, method, params })); });
  await send("Page.enable"); await send("Network.enable"); await send("Runtime.enable");
  await send("Emulation.setDeviceMetricsOverride", { width: 1300, height: 900, deviceScaleFactor: 1, mobile: false });
  const evaluate = async expression => { const result = await send("Runtime.evaluate", { expression, returnByValue: true, awaitPromise: true }); return result.result?.value; };
  const shot = async file => { const result = await send("Page.captureScreenshot", { format: "png", captureBeyondViewport: true }); fs.writeFileSync(path.join(OUT, file), Buffer.from(result.data, "base64")); };
  const bodyText = () => evaluate("document.body.innerText");
  const go = async hash => { await evaluate(`location.hash = '#/${hash}'`); await sleep(700); };

  await send("Network.setCookie", { name: cname, value: cvalue, url: "http://127.0.0.1:18132/", path: "/" });
  await send("Page.navigate", { url: "http://127.0.0.1:18132/" });
  await sleep(1800);

  // the overview shows what the screen shows
  let text = await bodyText();
  check("the overview says the link is up and the system armed", /connected to the server/i.test(text) && /armed/i.test(text) && /3 \/ 4/.test(text), text.slice(0, 300));
  check("the overview shows the board", /Touch-LCD-7C-BOX/.test(text));
  await shot("overview.png");

  // every page, in every language, without a script error
  const pages = ["overview", "server", "screen", "voice", "network", "wifi", "broker", "users", "update"];
  for (const [code] of [["en"], ["es"], ["de"], ["fr"], ["it"], ["ja"], ["zh"]]) {
    await evaluate(`localStorage.setItem('armor_lang', '${code}')`);
    await send("Page.navigate", { url: "http://127.0.0.1:18132/" });
    await sleep(1200);
    for (const page of pages) { await go(page); const content = await bodyText(); if (!content || content.length < 20 || /undefined|NaN/.test(content)) check(`${code}/${page} has content`, false, content.slice(0, 120)); }
  }
  check("no page raised a script error", errors.length === 0, errors.slice(0, 3).join(" | "));
  await evaluate("localStorage.setItem('armor_lang', 'en')");
  await send("Page.navigate", { url: "http://127.0.0.1:18132/" });
  await sleep(1200);

  // the server page, and a setting saved from it
  await go("server");
  text = await bodyText();
  check("the server page has the fields of the link", /Address of the server/.test(text) && /User of this panel/.test(text) && /Password of that user/.test(text), text.slice(0, 200));
  await shot("server.png");
  const saved = await evaluate(`(async () => { const r = await fetch('/api/v1/config', { method: 'PUT', headers: { 'Content-Type': 'application/json', 'X-Requested-With': 'armor' }, body: JSON.stringify({ display: { brightness: 40 }, audio: { volume: 25 } }) }); const c = await (await fetch('/api/v1/config')).json(); return [r.status, c.config.display.brightness, c.config.audio.volume]; })()`);
  check("a setting is saved and read back", saved && saved[0] === 200 && saved[1] === 40 && saved[2] === 25, JSON.stringify(saved));
  const refused = await evaluate(`(async () => { const r = await fetch('/api/v1/config', { method: 'PUT', headers: { 'Content-Type': 'application/json', 'X-Requested-With': 'armor' }, body: JSON.stringify({ voice: { enabled: true, url: 'ftp://x' } }) }); return [r.status, (await r.json()).problems?.[0]?.path]; })()`);
  check("a wrong address of the speech service is refused", refused && refused[0] === 422 && refused[1] === "voice.url", JSON.stringify(refused));
  await go("screen");
  await shot("screen.png");
  await go("voice");
  await shot("voice.png");
} catch (error) {
  console.log("FAIL the test itself", error?.stack ?? error);
  results.push(false);
} finally {
  cleanup();
}
const failed = results.filter(ok => !ok).length;
console.log(`${results.length - failed}/${results.length} checks passed; screenshots in ${OUT}`);
process.exit(failed ? 1 : 0);
