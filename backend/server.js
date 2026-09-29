// Entry point: reads the configuration from the environment (.env locally,
// the host's dashboard in production) and starts the HTTP server.

import "dotenv/config";
import { TuyaContext } from "@tuya/tuya-connector-nodejs";
import { createApp } from "./app.js";

const REQUIRED = ["TUYA_ACCESS_ID", "TUYA_ACCESS_SECRET", "TUYA_ENDPOINT", "DEVICE_ID", "API_KEY"];
const missing = REQUIRED.filter((k) => !process.env[k]);
if (missing.length > 0) {
  console.error(`Missing environment variables: ${missing.join(", ")}`);
  console.error("Copy .env.example to .env (local) or set them in your host's dashboard.");
  process.exit(1);
}

const tuya = new TuyaContext({
  baseUrl: process.env.TUYA_ENDPOINT,
  accessKey: process.env.TUYA_ACCESS_ID,
  secretKey: process.env.TUYA_ACCESS_SECRET,
});

const app = createApp({
  tuya,
  apiKey: process.env.API_KEY,
  deviceId: process.env.DEVICE_ID,
  valveCode: process.env.VALVE_CODE || "switch",
});

const port = Number(process.env.PORT) || 3000;
app.listen(port, () => console.log(`smart-valve-backend listening on port ${port}`));
