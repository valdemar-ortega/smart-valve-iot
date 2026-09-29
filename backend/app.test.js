// Run with: npm test   (Node's built-in test runner, no extra dependencies)

import assert from "node:assert/strict";
import { after, before, beforeEach, describe, it } from "node:test";
import { createApp } from "./app.js";

const KEY = "test-key";

/** Records every request and answers with whatever the test sets. */
const fakeTuya = {
  calls: [],
  reply: { success: true, result: true },
  async request(req) {
    this.calls.push(req);
    if (this.reply instanceof Error) throw this.reply;
    return this.reply;
  },
};

let server;
let base;

before(async () => {
  const app = createApp({ tuya: fakeTuya, apiKey: KEY, deviceId: "dev123", valveCode: "switch_1" });
  await new Promise((resolve) => {
    server = app.listen(0, resolve);
  });
  base = `http://127.0.0.1:${server.address().port}`;
});

after(() => server.close());

beforeEach(() => {
  fakeTuya.calls = [];
  fakeTuya.reply = { success: true, result: true };
});

const call = (method, path, key = KEY) =>
  fetch(base + path, { method, headers: key ? { "x-api-key": key } : {} });

describe("auth", () => {
  it("health check needs no key", async () => {
    const res = await call("GET", "/", null);
    assert.equal(res.status, 200);
  });

  it("rejects a missing key", async () => {
    const res = await call("POST", "/valve/open", null);
    assert.equal(res.status, 401);
    assert.equal(fakeTuya.calls.length, 0);
  });

  it("rejects a wrong key", async () => {
    const res = await call("POST", "/valve/open", "nope");
    assert.equal(res.status, 401);
  });
});

describe("valve", () => {
  it("open sends switch=true to the configured device", async () => {
    const res = await call("POST", "/valve/open");
    assert.equal(res.status, 200);
    assert.deepEqual(fakeTuya.calls[0], {
      method: "POST",
      path: "/v1.0/devices/dev123/commands",
      body: { commands: [{ code: "switch_1", value: true }] },
    });
  });

  it("close sends switch=false", async () => {
    await call("POST", "/valve/close");
    assert.equal(fakeTuya.calls[0].body.commands[0].value, false);
  });

  it("status reads the configured data point", async () => {
    fakeTuya.reply = {
      success: true,
      result: [
        { code: "countdown", value: 0 },
        { code: "switch_1", value: true },
      ],
    };
    const body = await (await call("GET", "/valve/status")).json();
    assert.deepEqual(body, { open: true });
  });

  it("status is null when the data point is missing", async () => {
    fakeTuya.reply = { success: true, result: [] };
    const body = await (await call("GET", "/valve/status")).json();
    assert.equal(body.open, null);
  });

  it("a cloud failure becomes 502 with success=false", async () => {
    fakeTuya.reply = new Error("token expired");
    const res = await call("POST", "/valve/open");
    assert.equal(res.status, 502);
    const body = await res.json();
    assert.equal(body.success, false);
    assert.match(body.msg, /token expired/);
  });
});
