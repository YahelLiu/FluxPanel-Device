/**
 * @file mqtt.cpp
 * @brief MQTT Client Module Implementation
 */

#include "mqtt.h"
#include "config.h"

#if MQTT_ENABLED

#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ============================================================================
// 全局对象
// ============================================================================

static WiFiClient mqttWifiClient;
static PubSubClient mqttClient(mqttWifiClient);

// 状态
static bool mqttConnected = false;
static unsigned long lastPublishTime = 0;
static unsigned long lastReconnectTime = 0;

// ============================================================================
// 初始化
// ============================================================================

void mqtt_init() {
    DEBUG_PRINTLN("[MQTT] Initializing...");

    // 配置 MQTT 服务器
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

    // 设置回调 (用于接收命令，可选)
    // mqttClient.setCallback(mqtt_callback);

    // 设置缓冲区大小 (默认 256 字节，JSON 可能需要更大)
    mqttClient.setBufferSize(512);

    DEBUG_PRINTF("[MQTT] Server: %s:%d\n", MQTT_SERVER, MQTT_PORT);
}

// ============================================================================
// 连接管理
// ============================================================================

bool mqtt_connect() {
    if (mqttClient.connected()) {
        return true;
    }

    // 检查 WiFi 是否连接
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("[MQTT] WiFi not connected, skip");
        return false;
    }

    DEBUG_PRINTLN("[MQTT] Connecting to broker...");

    // 生成 Client ID (使用设备 ID)
    String clientId = "FluxPanel_";
    clientId += DEVICE_ID;

    // 连接 (根据配置选择认证方式)
    bool connected = false;

#if defined(MQTT_BROKER_EMQX)
    // EMQX 公共 Broker 无需认证
    connected = mqttClient.connect(clientId.c_str());

#elif defined(MQTT_BROKER_ALIYUN)
    // 阿里云 IoT 需要用户名密码
    // 用户名: DeviceName&ProductKey
    // 密码: 通过 HMAC-SHA1 签名生成 (需要额外实现)
    String username = String(MQTT_DEVICE_NAME) + "&" + String(MQTT_PRODUCT_KEY);
    connected = mqttClient.connect(clientId.c_str(), username.c_str(), MQTT_DEVICE_SECRET);

#elif defined(MQTT_BROKER_CUSTOM)
    // 自定义 Broker
    connected = mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD);
#endif

    if (connected) {
        mqttConnected = true;
        DEBUG_PRINTLN("[MQTT] Connected!");

        // 发布上线消息
        mqtt_publish_online(true);

        // 订阅命令主题 (可选)
        mqttClient.subscribe(MQTT_TOPIC_COMMAND, MQTT_QOS);
        DEBUG_PRINTF("[MQTT] Subscribed to: %s\n", MQTT_TOPIC_COMMAND);

        return true;
    } else {
        mqttConnected = false;
        DEBUG_PRINTF("[MQTT] Connection failed, state: %d\n", mqttClient.state());
        return false;
    }
}

void mqtt_disconnect() {
    if (mqttClient.connected()) {
        // 发布离线消息
        mqtt_publish_online(false);
        mqttClient.disconnect();
    }
    mqttConnected = false;
}

bool mqtt_is_connected() {
    return mqttClient.connected();
}

// ============================================================================
// 主循环
// ============================================================================

void mqtt_loop() {
    // 检查连接状态
    if (!mqttClient.connected()) {
        mqttConnected = false;

        // 尝试重连
        unsigned long now = millis();
        if (now - lastReconnectTime >= MQTT_RECONNECT_MS) {
            lastReconnectTime = now;
            mqtt_connect();
        }
        return;
    }

    // 维持连接 (处理心跳、接收消息)
    mqttClient.loop();
}

// ============================================================================
// 消息发布
// ============================================================================

void mqtt_publish_data(MotorState& motorState, DeviceStatus& deviceStatus) {
    if (!mqttClient.connected()) {
        return;
    }

    // 创建 JSON 文档
    JsonDocument doc;

    doc["device_id"] = DEVICE_ID;
    doc["timestamp"] = millis();

    // 电机状态
    JsonObject motor = doc["motor"].to<JsonObject>();
    motor["running"] = motorState.is_running;
    motor["speed"] = motorState.speed_percent;
    motor["direction"] = motorState.direction;
    motor["rpm"] = motorState.current_rpm;
    motor["target_rpm"] = motorState.target_rpm;

    // 设备状态
    JsonObject device = doc["device"].to<JsonObject>();
    device["uptime"] = deviceStatus.uptime;
    device["wifi_rssi"] = WiFi.RSSI();

    // 序列化
    char buffer[512];
    size_t len = serializeJson(doc, buffer, sizeof(buffer));

    // 发布
    if (mqttClient.publish(MQTT_TOPIC_DATA, buffer, len, false)) {
        DEBUG_PRINTF("[MQTT] Published data: %d bytes\n", len);
    } else {
        DEBUG_PRINTLN("[MQTT] Publish failed");
    }
}

void mqtt_publish_system_status(SystemStatus& systemStatus) {
    if (!mqttClient.connected() || !systemStatus.valid) {
        return;
    }

    // 创建 JSON 文档
    JsonDocument doc;

    doc["device_id"] = DEVICE_ID;
    doc["timestamp"] = millis();

    // PC 状态
    JsonObject pc = doc["pc"].to<JsonObject>();
    pc["cpu_load"] = systemStatus.cpu_load;
    pc["cpu_temp"] = systemStatus.cpu_temp;
    pc["cpu_power"] = systemStatus.cpu_power;
    pc["gpu_load"] = systemStatus.gpu_load;
    pc["gpu_temp"] = systemStatus.gpu_temp;
    pc["gpu_power"] = systemStatus.gpu_power;
    pc["mem_load"] = systemStatus.mem_load;

    // 序列化
    char buffer[512];
    size_t len = serializeJson(doc, buffer, sizeof(buffer));

    // 发布
    if (mqttClient.publish(MQTT_TOPIC_DATA, buffer, len, false)) {
        DEBUG_PRINTF("[MQTT] Published system status: %d bytes\n", len);
    }
}

void mqtt_publish_alarm(const char* alarm_type, const char* message) {
    if (!mqttClient.connected()) {
        return;
    }

    // 创建 JSON 文档
    JsonDocument doc;

    doc["device_id"] = DEVICE_ID;
    doc["timestamp"] = millis();
    doc["type"] = alarm_type;
    doc["message"] = message;

    // 序列化
    char buffer[256];
    size_t len = serializeJson(doc, buffer, sizeof(buffer));

    // 发布 (保留消息，确保告警不丢失)
    if (mqttClient.publish(MQTT_TOPIC_ALARM, buffer, len, false)) {
        DEBUG_PRINTF("[MQTT] Published alarm: %s - %s\n", alarm_type, message);
    }
}

void mqtt_publish_online(bool online) {
    if (!mqttClient.connected() && online) {
        return;
    }

    // 简单的在线状态消息
    String payload = "{\"device_id\":\"" + String(DEVICE_ID) +
                     "\",\"online\":" + (online ? "true" : "false") +
                     ",\"timestamp\":" + String(millis()) + "}";

    // LWT (Last Will and Testament) 会在断开时自动发送
    // 这里手动发布上线消息
    if (online) {
        mqttClient.publish(MQTT_TOPIC_STATUS, payload.c_str(), true); // retain
        DEBUG_PRINTLN("[MQTT] Published online status");
    }
}

// ============================================================================
// 消息回调 (订阅命令)
// ============================================================================

// 如果需要接收 MQTT 命令，取消注释以下代码
/*
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    DEBUG_PRINTF("[MQTT] Message received on: %s\n", topic);

    // 解析 JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, length);

    if (error) {
        DEBUG_PRINTLN("[MQTT] JSON parse failed");
        return;
    }

    // 处理命令
    const char* cmd = doc["command"];
    if (cmd) {
        DEBUG_PRINTF("[MQTT] Command: %s\n", cmd);
        // 转发给命令处理模块...
    }
}
*/

#else // MQTT_ENABLED = false

// ============================================================================
// MQTT 禁用时的空实现
// ============================================================================

void mqtt_init() {}
void mqtt_loop() {}
bool mqtt_connect() { return false; }
void mqtt_disconnect() {}
bool mqtt_is_connected() { return false; }
void mqtt_publish_data(MotorState& motorState, DeviceStatus& deviceStatus) {}
void mqtt_publish_system_status(SystemStatus& systemStatus) {}
void mqtt_publish_alarm(const char* alarm_type, const char* message) {}
void mqtt_publish_online(bool online) {}

#endif // MQTT_ENABLED
