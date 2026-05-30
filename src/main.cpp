/**
 * @file main.cpp
 * @brief FluxPanel Device - ESP32-C3 Main Entry
 *
 * 功能:
 * 1. WiFi 网页配网 (WiFiManager)
 * 2. WebSocket 连接到 FluxPanel 后端
 * 3. 接收前端控制命令并执行
 * 4. 定时上报设备状态
 * 5. ST7735 LCD 实时显示 (WS 连接后显示系统状态)
 */

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "motor_state.h"
#include "display.h"
#include "relay.h"
#include "esc_pwm.h"
#include "pid_controller.h"
#include "rtos_tasks.h"
#include "websocket.h"
#include "wifi_manager.h"
#include "hall_sensor.h"
#include "ina219_sensor.h"
#include "alarm.h"
#include "motor_fsm.h"
#include "mqtt.h"

// ============================================================================
// 全局状态
// ============================================================================

MotorState motorState;      // 电机状态
DeviceStatus deviceStatus;  // 设备状态
SystemStatus systemStatus;  // 系统状态 (Yves-PC 数据)

unsigned long lastStatusTime = 0;      // 上次状态上报时间
unsigned long lastDisplayTime = 0;     // 上次显示更新时间
unsigned long lastMqttTime = 0;        // 上次 MQTT 发布时间
unsigned long startTime = 0;           // 系统启动时间

bool currentWsState = false;           // 当前 WS 连接状态

// 安全相关
unsigned long lastCommandTime = 0;     // 最后收到命令的时间

// ============================================================================
// 解析系统状态字符串
// ============================================================================

void parseSystemStatus(const String& str, SystemStatus& status) {
    // 格式: cpu_load,cpu_temp,cpu_power,gpu_load,gpu_temp,gpu_power,mem_load
    int idx = 0;
    int start = 0;
    int end = str.indexOf(',');

    float values[7];
    for (int i = 0; i < 7 && end != -1; i++) {
        values[i] = str.substring(start, end).toFloat();
        start = end + 1;
        end = str.indexOf(',', start);
        idx = i + 1;
    }
    // 最后一个值
    if (idx < 7) {
        values[6] = str.substring(start).toInt();
    }

    status.cpu_load = values[0];
    status.cpu_temp = values[1];
    status.cpu_power = values[2];
    status.gpu_load = values[3];
    status.gpu_temp = values[4];
    status.gpu_power = values[5];
    status.mem_load = (int)values[6];
    status.valid = true;
    status.last_update = millis();
}

// ============================================================================
// Fail-safe 安全机制
// ============================================================================

/**
 * @brief 紧急停机
 *
 * 在检测到异常情况时立即停止电机
 * 包括: 通信断开、超时、WiFi 断开等
 */
void emergency_stop(const char* reason) {
    DEBUG_PRINTF("[SAFETY] Emergency stop: %s\n", reason);

    // 触发错误事件 (状态机处理停机)
    fsm_process_event(FSM_EVENT_ERROR);

    // 触发报警
    alarm_trigger(ALARM_COMM_LOST);
}

/**
 * @brief 安全检查
 *
 * 定期检查安全条件, 异常时触发紧急停机
 */
void safety_check() {
    // 只在电机运行时检查
    if (!motorState.is_running) {
        return;
    }

    // 检查 1: WebSocket 断开
    if (!deviceStatus.ws_connected) {
        emergency_stop("WebSocket disconnected");
        return;
    }

    // 检查 2: 通信超时
    if (millis() - lastCommandTime > SAFETY_TIMEOUT) {
        emergency_stop("Command timeout");
        return;
    }
}

// ============================================================================
// 命令处理回调
// ============================================================================

void onCommandReceived(MotorCommand& cmd) {
    DEBUG_PRINTF("[Main] Command received: %s (id: %u)\n",
        cmd.command.c_str(), cmd.command_id);

    // 更新最后命令时间 (用于 Fail-safe 超时检测)
    lastCommandTime = millis();

    bool success = true;
    String message = "";

    // 系统状态更新 (来自服务器推送)
    if (cmd.command == "system_status") {
        parseSystemStatus(cmd.content, systemStatus);
        display_system_status(systemStatus);
        return;  // 不需要发送 ack
    }

    // 执行命令
    if (cmd.command == "start") {
        // 启动电机 (使用状态机)
        motorState.speed_percent = cmd.speed_percent > 0 ? cmd.speed_percent : 50;
        motorState.direction = cmd.direction;

        // 触发启动事件
        fsm_process_event(FSM_EVENT_START);

        message = "Motor started at " + String(motorState.speed_percent) + "%";
        DEBUG_PRINTF("[Main] Motor start event sent, target %d%%\n", motorState.speed_percent);
    }
    else if (cmd.command == "stop") {
        // 停止电机 (使用状态机)
        fsm_process_event(FSM_EVENT_STOP);
        message = "Motor stopped";
        DEBUG_PRINTLN("[Main] Motor stop event sent");
    }
    else if (cmd.command == "speed") {
        // 设置转速
        if (cmd.speed_percent >= 0 && cmd.speed_percent <= 100) {
            motorState.speed_percent = cmd.speed_percent;
            // 运行中实时调速
            if (motorState.is_running) {
                esc_set_speed(cmd.speed_percent);
            }
            message = "Speed set to " + String(cmd.speed_percent) + "%";
            DEBUG_PRINTF("[Main] Speed set to %d%%\n", cmd.speed_percent);
        } else {
            success = false;
            message = "Invalid speed value";
        }
    }
    else if (cmd.command == "direction") {
        // 设置方向
        if (cmd.direction == 1 || cmd.direction == -1) {
            motorState.direction = cmd.direction;
            message = "Direction: " + String(cmd.direction == 1 ? "Forward" : "Reverse");
            DEBUG_PRINTF("[Main] Direction set to %d\n", cmd.direction);
        } else {
            success = false;
            message = "Invalid direction value";
        }
    }
    else if (cmd.command == "lcd") {
        // LCD 显示
        display_lcd_line(cmd.line, cmd.content, cmd.clear_screen);
        message = "LCD updated";
        DEBUG_PRINTF("[Main] LCD line %d: %s\n", cmd.line, cmd.content.c_str());
    }
    else if (cmd.command == "pid_mode") {
        // PID 模式切换
        motorState.pid_enabled = cmd.pid_enabled;
        if (cmd.target_rpm > 0) {
            motorState.target_rpm = cmd.target_rpm;
        }
        if (motorState.pid_enabled) {
            pid_reset();
            pid_set_setpoint(motorState.target_rpm);
            message = "PID mode enabled, target " + String(motorState.target_rpm) + " RPM";
        } else {
            message = "PID mode disabled";
        }
        DEBUG_PRINTF("[Main] PID mode: %s, target: %.1f RPM\n",
            motorState.pid_enabled ? "enabled" : "disabled", motorState.target_rpm);
    }
    else if (cmd.command == "pid_tune") {
        // PID 参数调整
        if (cmd.pid_kp > 0 || cmd.pid_ki > 0 || cmd.pid_kd > 0) {
            float kp, ki, kd;
            pid_get_tunings(&kp, &ki, &kd);
            if (cmd.pid_kp > 0) kp = cmd.pid_kp;
            if (cmd.pid_ki > 0) ki = cmd.pid_ki;
            if (cmd.pid_kd > 0) kd = cmd.pid_kd;
            pid_set_tunings(kp, ki, kd);
            message = "PID tuned: Kp=" + String(kp) + " Ki=" + String(ki) + " Kd=" + String(kd);
        } else {
            success = false;
            message = "Invalid PID parameters";
        }
    }
    else {
        // 未知命令
        success = false;
        message = "Unknown command: " + cmd.command;
        DEBUG_PRINTF("[Main] Unknown command: %s\n", cmd.command.c_str());
    }

    // 发送命令确认
    ws_send_ack(cmd.command_id, success, message);

    // 更新显示 (WS 已连接时显示系统状态)
    if (deviceStatus.ws_connected && systemStatus.valid) {
        display_system_status(systemStatus);
    }
}

// ============================================================================
// WiFiManager 回调
// ============================================================================

void onWiFiConfigMode() {
    // 进入配网模式时显示提示
    DEBUG_PRINTLN("[Main] WiFi config mode started");
    display_setup_mode(deviceStatus);
}

void onWiFiConnected(String ip) {
    // WiFi 连接成功时更新显示
    DEBUG_PRINTF("[Main] WiFi connected: %s\n", ip.c_str());
    deviceStatus.wifi_connected = true;
    deviceStatus.ip_address = ip;
    display_setup_mode(deviceStatus);
}

// ============================================================================
// 初始化
// ============================================================================

void setup() {
    // 初始化串口
    Serial.begin(DEBUG_BAUD);
    delay(1000);

    DEBUG_PRINTLN("\n========================================");
    DEBUG_PRINTLN("FluxPanel Device - ESP32-C3");
    DEBUG_PRINTLN("========================================");

    // 记录启动时间
    startTime = millis();

    // 初始化显示屏
    display_init();

    // 初始化继电器
    relay_init();

    // 初始化 ESC PWM (必须在 relay_init 之后)
    esc_init();

    // 初始化霍尔传感器
    hall_init();

    // 初始化 INA219 电流传感器
    ina219_init();

    // 初始化报警系统
    alarm_init();

    // 初始化状态机
    fsm_init();

    // 初始化电机状态
    motorState.reset();
    systemStatus.reset();

    // 设置 WiFiManager 回调
    wifi_manager_set_config_callback(onWiFiConfigMode);
    wifi_manager_set_connected_callback(onWiFiConnected);

    // WiFi 断开事件处理 (Fail-safe)
    WiFi.onEvent([](WiFiEvent_t event) {
        if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
            fsm_process_event(FSM_EVENT_ERROR);
            alarm_trigger(ALARM_COMM_LOST);
        }
    });

    // 初始化 WiFi (使用 WiFiManager 网页配网)
    if (wifi_manager_init()) {
        deviceStatus.wifi_connected = true;
        deviceStatus.ip_address = wifi_manager_get_ip();
        DEBUG_PRINTF("[Setup] WiFi connected: %s\n", deviceStatus.ip_address.c_str());
    } else {
        // 进入配网模式或连接失败
        DEBUG_PRINTLN("[Setup] WiFi in config mode or failed");
    }

    // 初始化 WebSocket
    ws_init();
    ws_set_command_callback(onCommandReceived);

    // 初始化 MQTT
    mqtt_init();

    // 初始化 PID 控制器
    pid_init(PID_KP, PID_KI, PID_KD);

    // 初始化 FreeRTOS
    rtos_init();
    rtos_start();

    DEBUG_PRINTLN("[Setup] Initialization complete");

    // 显示初始状态 (设置模式)
    display_setup_mode(deviceStatus);
}

// ============================================================================
// 主循环 (RTOS 模式)
// ============================================================================

void loop() {
    // FreeRTOS 模式: 大部分工作由任务处理
    // 这里只处理状态上报和 MQTT

    // MQTT 循环 (处理心跳、重连、消息)
    mqtt_loop();

    // 定时上报状态
    if (millis() - lastStatusTime >= STATUS_INTERVAL) {
        lastStatusTime = millis();

        // 更新运行时间
        deviceStatus.uptime = (millis() - startTime) / 1000;

        // WebSocket 状态上报
        if (deviceStatus.ws_connected) {
            ws_send_status(motorState);
        }
    }

    // 定时 MQTT 数据上报
    if (mqtt_is_connected() && millis() - lastMqttTime >= MQTT_PUBLISH_MS) {
        lastMqttTime = millis();

        // 发布设备数据
        mqtt_publish_data(motorState, deviceStatus);

        // 发布 PC 系统状态 (如果有)
        if (systemStatus.valid) {
            mqtt_publish_system_status(systemStatus);
        }
    }

    // 空闲等待
    vTaskDelay(100 / portTICK_PERIOD_MS);
}
