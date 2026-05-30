# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

This is a PlatformIO project for ESP32-C3. Use `pio` commands:

```bash
# Build the project
pio run

# Upload to device (default: COM3)
pio run -t upload

# Serial monitor (115200 baud)
pio device monitor -p COM3 -b 115200

# Build and upload in one command
pio run -t upload && pio device monitor
```

## Architecture Overview

This is an ESP32-C3 based brushless motor control system with WebSocket connectivity to a FluxPanel backend.

### Core Modules (src/)

- **config.h** - Central configuration: WiFi, WebSocket server, GPIO pins, PID parameters, safety thresholds
- **main.cpp** - Initialization sequence and command dispatch
- **motor_state.h** - Data structures for `MotorState`, `DeviceStatus`, `SystemStatus`, `MotorCommand`

### FreeRTOS Task Structure

Four tasks with different priorities:

| Task | Priority | Period | Purpose |
|------|----------|--------|---------|
| MotorTask | 3 (highest) | 10ms | State machine, PID control, safety checks |
| WiFiTask | 2 | 10ms | WiFi/WebSocket event handling |
| SensorTask | 1 | 100ms | INA219 current/voltage, Hall sensor RPM |
| DisplayTask | 1 | 100ms | LCD refresh |

Thread safety via `motorStateMutex` and `displayMutex`.

### Motor State Machine (motor_fsm.h/cpp)

```
IDLE ──start──→ STARTING ──armed──→ RUNNING
  ↑                                   │
  └─────────── stop/error ────────────┘
                    │
                    ↓ error
                  ERROR ──reset──→ IDLE
```

Events: `FSM_EVENT_START`, `FSM_EVENT_STOP`, `FSM_EVENT_ERROR`, `FSM_EVENT_RESET`, `FSM_EVENT_ARMED`

### WebSocket Protocol

Connect: `ws://<server>:<port>/ws/device?device_id=XXX&token=XXX`

Commands from server: `start`, `stop`, `speed`, `direction`, `lcd`, `pid_mode`, `pid_tune`

All commands receive an ACK via `ws_send_ack()`.

### Safety (Fail-safe)

- WebSocket disconnect → emergency stop
- Command timeout (10s default, `SAFETY_TIMEOUT`) → emergency stop
- WiFi disconnect → FSM error event, alarm trigger
- Hardware alarms: overcurrent, overtemperature, stall detection

### Key Dependencies

- Adafruit ST7735 TFT LCD (SPI)
- ArduinoJson for WebSocket messages
- WebSockets library
- WiFiManager for AP-based provisioning
- Adafruit INA219 current sensor (I2C)

## Configuration

Edit `src/config.h` before building:

- `WIFI_USE_CONFIG`: `true` = hardcoded credentials, `false` = WiFiManager AP provisioning
- `WS_SERVER`, `WS_PORT`: Backend server address
- `DEVICE_ID`, `DEVICE_TOKEN`: Device identification
- GPIO pins for TFT, relay, ESC PWM, INA219, Hall sensor
- PID tuning: `PID_KP`, `PID_KI`, `PID_KD`
- Safety thresholds: `ALARM_OVERCURRENT_THRESHOLD`, `ALARM_OVERTEMP_THRESHOLD`
