# Protocol

Two independent paths connect the phone to the hardware:

| Path | Transport | Direction | Why |
|---|---|---|---|
| Pressure | MQTT over TLS | ESP32 → broker → app | Live stream, many listeners, works from any network |
| Valve | HTTPS REST | app → backend → Tuya cloud → valve | The valve is a Tuya device; its cloud secret must not live in the APK |

## MQTT

Default topic names below; both ends read them from their configuration
(`menuconfig` on the firmware, `secrets.properties` on the app).

### `smartvalve/pressure` (retained, QoS 1)

Published by the ESP32 every sample period (2 s by default).

```json
{"psi":87.0,"voltage":1.659,"raw":2051,"fault":false,"uptime_ms":123456}
```

| Field | Type | Meaning |
|---|---|---|
| `psi` | number, 1 decimal | Pressure. `0.0` when `fault` is true |
| `voltage` | number, 3 decimals | Calibrated voltage at the ADC pin (after the divider) |
| `raw` | integer 0–4095 | Averaged raw ADC code, for diagnostics |
| `fault` | boolean | Pin voltage outside the valid window: open wire, unpowered sensor or short |
| `uptime_ms` | integer | Milliseconds since boot; a reset shows up as a drop |

Retained means a client that subscribes later receives the last reading
immediately instead of waiting for the next one.

Consumers must ignore unknown fields, so new ones can be added without
breaking older apps.

### `smartvalve/status` (retained, QoS 1)

| Payload | Sent by | When |
|---|---|---|
| `online` | ESP32 | Right after every (re)connection to the broker |
| `offline` | Broker (Last Will) | When the ESP32 disappears without disconnecting cleanly (power loss, Wi-Fi drop) |

## Backend REST API

Base URL: wherever you deploy `backend/` (e.g. `https://your-app.onrender.com/`).
Every `/valve/*` call needs the header `x-api-key: <API_KEY>`.

| Method | Path | Response |
|---|---|---|
| GET | `/` | `{"ok":true,"service":"smart-valve-backend"}` (no key needed) |
| POST | `/valve/open` | Tuya's reply, e.g. `{"success":true,"result":true,"t":...}` |
| POST | `/valve/close` | Same as open |
| GET | `/valve/status` | `{"open":true}`, `{"open":false}` or `{"open":null}` if unknown |
| GET | `/valve/functions` | The device's data points (use it to find `VALVE_CODE`) |

Errors: `401` for a missing or wrong key; `502` with
`{"success":false,"msg":"..."}` when the Tuya cloud fails.
