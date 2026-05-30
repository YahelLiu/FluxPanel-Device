/**
 * @file websocket.cpp
 * @brief WebSocket Communication Module Implementation
 *
 * 改进: 使用命令队列传递命令，防止覆盖丢失
 */

#include "websocket.h"
#include "config.h"
#include "rtos_tasks.h"  // 改进: 命令队列

#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// WebSocket 客户端
static WebSocketsClient webSocket;

// 连接状态
static bool wsConnected = false;

// 命令回调 (保留兼容性)
static CommandCallback commandCallback = nullptr;

// 最后错误消息
static String lastError = "";

// 心跳定时器
static unsigned long lastHeartbeat = 0;
#define HEARTBEAT_INTERVAL 30000  // 心跳间隔 30 秒

// ============================================================================
// WebSocket 事件处理
// ============================================================================

static void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            wsConnected = false;
            DEBUG_PRINTLN("[WS] Disconnected");
            lastError = "WebSocket disconnected";
            break;

        case WStype_CONNECTED:
            wsConnected = true;
            lastError = "";
            DEBUG_PRINTLN("[WS] Connected to server");
            break;

        case WStype_TEXT: {
            DEBUG_PRINTF("[WS] Received: %s\n", (char*)payload);

            // 解析 JSON
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (error) {
                DEBUG_PRINTF("[WS] JSON parse error: %s\n", error.c_str());
                lastError = "JSON parse error";
                break;
            }

            const char* msgType = doc["type"];

            // 处理系统状态消息 (来自服务器的 Yves-PC 数据)
            if (msgType && strcmp(msgType, "system_status") == 0) {
                MotorCommand cmd;
                cmd.command_id = 0;
                cmd.command = "system_status";

                if (doc["data"].is<JsonObject>()) {
                    JsonObject data = doc["data"];
                    // content 用于传递完整数据 (需要 main.cpp 解析)
                    String statusStr = "";

                    float cpuLoad = data["cpu_load"].is<float>() ? data["cpu_load"].as<float>() : 0;
                    float cpuTemp = data["cpu_temp"].is<float>() ? data["cpu_temp"].as<float>() : 0;
                    float cpuPower = data["cpu_power"].is<float>() ? data["cpu_power"].as<float>() : 0;
                    float gpuLoad = data["gpu_load"].is<float>() ? data["gpu_load"].as<float>() : 0;
                    float gpuTemp = data["gpu_temp"].is<float>() ? data["gpu_temp"].as<float>() : 0;
                    float gpuPower = data["gpu_power"].is<float>() ? data["gpu_power"].as<float>() : 0;
                    int memLoad = data["mem_load"].is<int>() ? data["mem_load"].as<int>() : 0;

                    statusStr += String(cpuLoad) + ",";
                    statusStr += String(cpuTemp) + ",";
                    statusStr += String(cpuPower) + ",";
                    statusStr += String(gpuLoad) + ",";
                    statusStr += String(gpuTemp) + ",";
                    statusStr += String(gpuPower) + ",";
                    statusStr += String(memLoad);
                    cmd.content = statusStr;
                }

                if (commandCallback) {
                    commandCallback(cmd);
                }
            }
            // 处理命令消息
            else if (msgType && strcmp(msgType, "command") == 0 && commandCallback) {
                MotorCommand cmd;
                cmd.command_id = doc["command_id"] | 0;
                cmd.command = doc["command"].as<String>();
                cmd.speed_percent = 0;
                cmd.direction = 1;

                // 解析 params
                if (doc["params"].is<JsonObject>()) {
                    JsonObject params = doc["params"];
                    if (params["speed_percent"].is<int>()) {
                        cmd.speed_percent = params["speed_percent"].as<int>();
                    }
                    if (params["direction"].is<int>()) {
                        cmd.direction = params["direction"].as<int>();
                    }
                    // LCD 参数
                    if (params["content"].is<const char*>()) {
                        cmd.content = params["content"].as<String>();
                    }
                    if (params["line"].is<int>()) {
                        cmd.line = params["line"].as<int>();
                    }
                    if (params["clear"].is<bool>()) {
                        cmd.clear_screen = params["clear"].as<bool>();
                    }
                    // PID 参数
                    if (params["enabled"].is<bool>()) {
                        cmd.pid_enabled = params["enabled"].as<bool>();
                    }
                    if (params["target_rpm"].is<float>()) {
                        cmd.target_rpm = params["target_rpm"].as<float>();
                    }
                    if (params["kp"].is<float>()) {
                        cmd.pid_kp = params["kp"].as<float>();
                    }
                    if (params["ki"].is<float>()) {
                        cmd.pid_ki = params["ki"].as<float>();
                    }
                    if (params["kd"].is<float>()) {
                        cmd.pid_kd = params["kd"].as<float>();
                    }
                }

                DEBUG_PRINTF("[WS] Command: %s (id: %u)\n",
                    cmd.command.c_str(), cmd.command_id);

                // ---- 改进: 发送命令到队列 (防止覆盖) ----
                if (commandQueue != nullptr) {
                    if (xQueueSend(commandQueue, &cmd, pdMS_TO_TICKS(CMD_QUEUE_TIMEOUT)) != pdPASS) {
                        DEBUG_PRINTLN("[WS] WARNING: Command queue full, command dropped!");
                        lastError = "Command queue full";
                    } else {
                        DEBUG_PRINTLN("[WS] Command sent to queue");
                    }
                } else {
                    // 队列未初始化，使用旧方式 (兼容性)
                    if (commandCallback) {
                        commandCallback(cmd);
                    }
                }
            }
            // 处理心跳响应
            else if (msgType && strcmp(msgType, "pong") == 0) {
                DEBUG_PRINTLN("[WS] Heartbeat response received");
            }
            break;
        }

        case WStype_BIN:
            DEBUG_PRINTF("[WS] Binary data received, length: %u\n", length);
            break;

        case WStype_ERROR:
            DEBUG_PRINTLN("[WS] Error");
            lastError = "WebSocket error";
            break;

        default:
            break;
    }
}

// ============================================================================
// 公共函数
// ============================================================================

void ws_init() {
    // 构建 WebSocket URL
    String url = String(WS_PATH) +
                 "?device_id=" + String(DEVICE_ID) +
                 "&token=" + String(DEVICE_TOKEN);

    DEBUG_PRINTF("[WS] Connecting to ws://%s:%d%s\n", WS_SERVER, WS_PORT, url.c_str());

    // 初始化 WebSocket 客户端
    webSocket.begin(WS_SERVER, WS_PORT, url);
    webSocket.onEvent(webSocketEvent);

    // 设置自动重连
    webSocket.setReconnectInterval(WS_RECONNECT_INTERVAL);

    lastHeartbeat = millis();
}

void ws_loop() {
    webSocket.loop();

    // 发送心跳
    if (wsConnected && (millis() - lastHeartbeat >= HEARTBEAT_INTERVAL)) {
        lastHeartbeat = millis();
        ws_send_heartbeat();
    }
}

bool ws_is_connected() {
    return wsConnected;
}

void ws_send_status(MotorState& state) {
    if (!wsConnected) return;

    // 构建 JSON
    JsonDocument doc;
    doc["type"] = "status";
    doc["device_id"] = DEVICE_ID;

    JsonObject data = doc["data"].to<JsonObject>();
    data["is_running"] = state.is_running;
    data["speed_percent"] = state.speed_percent;
    data["direction"] = state.direction;
    data["temperature"] = roundf(state.temperature * 10.0f) / 10.0f;
    data["current"] = roundf(state.current * 100.0f) / 100.0f;
    data["voltage"] = roundf(state.voltage * 10.0f) / 10.0f;

    String output;
    serializeJson(doc, output);

    webSocket.sendTXT(output);

    DEBUG_PRINTF("[WS] Sent status: %s\n", output.c_str());
}

void ws_send_ack(uint32_t command_id, bool success, String message) {
    if (!wsConnected) return;

    JsonDocument doc;
    doc["type"] = "command_ack";
    doc["command_id"] = command_id;
    doc["success"] = success;
    doc["message"] = message;

    String output;
    serializeJson(doc, output);

    webSocket.sendTXT(output);

    DEBUG_PRINTF("[WS] Sent ack: %s\n", output.c_str());
}

void ws_set_command_callback(CommandCallback callback) {
    commandCallback = callback;
}

void ws_send_heartbeat() {
    if (!wsConnected) return;

    JsonDocument doc;
    doc["type"] = "ping";
    doc["device_id"] = DEVICE_ID;

    String output;
    serializeJson(doc, output);

    webSocket.sendTXT(output);
}

String ws_get_last_error() {
    return lastError;
}