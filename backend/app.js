// Express app that turns three simple calls (open / close / status) into
// signed requests to the Tuya IoT cloud. The Tuya secret stays here, on the
// server; the phone only knows a revocable API key.
//
// createApp() receives its dependencies so tests can pass a fake Tuya client.

import crypto from "node:crypto";
import express from "express";

/** Constant-time comparison, so the key cannot be guessed byte by byte. */
function safeEqual(a, b) {
  const x = Buffer.from(String(a ?? ""));
  const y = Buffer.from(String(b ?? ""));
  return x.length === y.length && crypto.timingSafeEqual(x, y);
}

/**
 * @param {object} deps
 * @param {{request: Function}} deps.tuya  Tuya client (TuyaContext or a fake)
 * @param {string} deps.apiKey             key the app sends in x-api-key
 * @param {string} deps.deviceId           Tuya device id of the valve
 * @param {string} deps.valveCode          data-point code of the switch
 */
export function createApp({ tuya, apiKey, deviceId, valveCode }) {
  const app = express();
  app.disable("x-powered-by");
  app.use(express.json());

  // Health check without a key: open it in a browser after deploying.
  app.get("/", (_req, res) => {
    res.json({ ok: true, service: "smart-valve-backend" });
  });

  // Everything below requires the API key.
  app.use((req, res, next) => {
    if (!apiKey || !safeEqual(req.header("x-api-key"), apiKey)) {
      return res.status(401).json({ error: "unauthorized: missing or wrong x-api-key" });
    }
    next();
  });

  const setValve = (on) =>
    tuya.request({
      method: "POST",
      path: `/v1.0/devices/${deviceId}/commands`,
      body: { commands: [{ code: valveCode, value: on }] },
    });

  const fail = (res, what, e) =>
    res.status(502).json({ success: false, msg: `${what}: ${e?.message ?? e}` });

  app.post("/valve/open", async (_req, res) => {
    try {
      res.json(await setValve(true));
    } catch (e) {
      fail(res, "open failed", e);
    }
  });

  app.post("/valve/close", async (_req, res) => {
    try {
      res.json(await setValve(false));
    } catch (e) {
      fail(res, "close failed", e);
    }
  });

  app.get("/valve/status", async (_req, res) => {
    try {
      const r = await tuya.request({ method: "GET", path: `/v1.0/devices/${deviceId}/status` });
      if (!r.success) return res.status(502).json({ open: null, msg: r.msg });
      const dp = r.result?.find((d) => d.code === valveCode);
      res.json({ open: typeof dp?.value === "boolean" ? dp.value : null });
    } catch (e) {
      fail(res, "status failed", e);
    }
  });

  // Diagnostics: lists the device's real data-point codes. Use it once to
  // find the right VALVE_CODE ("switch", "switch_1", ...).
  app.get("/valve/functions", async (_req, res) => {
    try {
      res.json(await tuya.request({ method: "GET", path: `/v1.0/devices/${deviceId}/functions` }));
    } catch (e) {
      fail(res, "functions failed", e);
    }
  });

  return app;
}
