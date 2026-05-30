/**
 * @file rtos_tasks.cpp
 * @brief FreeRTOS Task Implementation
 *
 * RTOS 同步机制改进:
 *   - 命令队列: 防止 WebSocket 命令覆盖
 *   - 任务通知: 紧急停机微秒级响应
 *   - 栈监控: 防止栈溢出
 */

#include "rtos_tasks.h"
#include "config.h"
#include "motor_state.h"
#include "esc_pwm.h"
#include "pid_controller.h"
#include "hall_sensor.h"
#include "ina219_sensor.h"
#include "alarm.h"
#include "motor_fsm.h"
#include "relay.h"
#include "display.h"
#include "websocket.h"
#include "wifi_manager.h"

// ============================================================================
// 任务句柄
// ============================================================================
TaskHandle_t wifiTaskHandle = nullptr;
TaskHandle_t motorTaskHandle = nullptr;
TaskHandle_t sensorTaskHandle = nullptr;
TaskHandle_t displayTaskHandle = nullptr;

// ============================================================================
// 互斥锁
// ============================================================================
SemaphoreHandle_t motorStateMutex = nullptr;
SemaphoreHandle_t displayMutex = nullptr;

// ============================================================================
// 命令队列 (改进: 防止命令覆盖)
// ============================================================================
QueueHandle_t commandQueue = nullptr;

// ============================================================================
// 外部变量 (来自 main.cpp)
// ============================================================================
extern MotorState motorState;
extern DeviceStatus deviceStatus;
extern SystemStatus systemStatus;
extern unsigned long lastCommandTime;

// ============================================================================
// 内部函数声明
// ============================================================================
void safety_check_rtos();
void process_command_from_queue(MotorCommand& cmd);

// ============================================================================
// WiFi 任务 (事件驱动)
// ============================================================================
void wifiTask(void* arg) {
    DEBUG_PRINTLN("[RTOS] WiFi task started");

    while (1) {
        // WiFi 状态检查
        wifi_manager_loop();

        // 更新 WiFi 连接状态
        bool wifiNowConnected = wifi_manager_is_connected();
        if (wifiNowConnected != deviceStatus.wifi_connected) {
            if (xSemaphoreTake(motorStateMutex, 10)) {
                deviceStatus.wifi_connected = wifiNowConnected;
                if (wifiNowConnected) {
                    deviceStatus.ip_address = wifi_manager_get_ip();
                }
                xSemaphoreGive(motorStateMutex);
            }
        }

        // WebSocket 主循环
        ws_loop();

        // 更新 WebSocket 连接状态
        bool wsNowConnected = ws_is_connected();
        if (wsNowConnected != deviceStatus.ws_connected) {
            if (xSemaphoreTake(motorStateMutex, 10)) {
                deviceStatus.ws_connected = wsNowConnected;

                if (!wsNowConnected) {
                    // WS 断开
                    systemStatus.valid = false;
                    systemStatus.reset();
                }
                xSemaphoreGive(motorStateMutex);
            }
        }

        // 10ms 周期
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// ============================================================================
// 电机控制任务 (10ms 周期, 最高优先级)
// ============================================================================
void motorTask(void* arg) {
    DEBUG_PRINTLN("[RTOS] Motor task started");

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t period = 10 / portTICK_PERIOD_MS;  // 10ms

    // 命令接收缓冲
    MotorCommand cmd;
    uint32_t notificationValue = 0;

    while (1) {
        // ---- 改进1: 检查任务通知 (紧急停机) ----
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &notificationValue, 0)) {
            // 有通知到达
            if (notificationValue & NOTIFY_EMERGENCY_STOP) {
                DEBUG_PRINTLN("[RTOS] Emergency stop notification received!");

                // 立即停机 (微秒级响应)
                esc_stop();
                relay_off();
                motorState.is_running = false;
                motorState.speed_percent = 0;
                pid_reset();

                fsm_process_event(FSM_EVENT_ERROR);
            }
        }

        // ---- 改进2: 从队列获取命令 (非阻塞) ----
        while (xQueueReceive(commandQueue, &cmd, 0)) {
            // 处理队列中的所有命令
            process_command_from_queue(cmd);
        }

        // 状态机更新
        fsm_update();

        // 安全检查 (Fail-safe)
        safety_check_rtos();

        // 报警检查
        alarm_check();

        // PID 控制
        if (xSemaphoreTake(motorStateMutex, 5)) {
            if (motorState.pid_enabled && motorState.is_running) {
                // PID 闭环控制
                pid_set_setpoint(motorState.target_rpm);
                float output = pid_compute(motorState.actual_rpm);

                // 限制输出范围
                if (output < 0) output = 0;
                if (output > 100) output = 100;

                esc_set_speed((uint8_t)output);
            }
            xSemaphoreGive(motorStateMutex);
        }

        // 精确周期控制
        vTaskDelayUntil(&lastWakeTime, period);
    }
}

// ============================================================================
// 传感器读取任务 (100ms 周期)
// ============================================================================
void sensorTask(void* arg) {
    DEBUG_PRINTLN("[RTOS] Sensor task started");

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t period = 100 / portTICK_PERIOD_MS;  // 100ms

    // 栈监控计数器
    static uint32_t stackCheckCounter = 0;

    while (1) {
        if (xSemaphoreTake(motorStateMutex, 10)) {
            // 读取 INA219 电流/电压/功率
            if (ina219_is_ready()) {
                ina219_read();
                motorState.current = ina219_get_current();
                motorState.voltage = ina219_get_voltage();
            } else {
                // INA219 未就绪，使用模拟数据
                if (motorState.is_running) {
                    motorState.current = 0.5 + (motorState.speed_percent / 100.0);
                    motorState.voltage = 12.0;
                } else {
                    motorState.current = 0.0;
                    motorState.voltage = 12.0;
                }
            }

            // 读取霍尔传感器转速
            motorState.actual_rpm = hall_get_rpm();

            // 温度 (目前使用模拟数据)
            if (motorState.is_running) {
                motorState.temperature = 25.0 + (motorState.speed_percent / 10.0);
            } else {
                motorState.temperature = 25.0;
            }

            motorState.last_update = millis();
            xSemaphoreGive(motorStateMutex);
        }

        // ---- 改进3: 定期栈监控 (每5秒) ----
        stackCheckCounter++;
        if (stackCheckCounter >= 50) {  // 100ms * 50 = 5s
            stackCheckCounter = 0;
            rtos_check_stack();
        }

        vTaskDelayUntil(&lastWakeTime, period);
    }
}

// ============================================================================
// 显示刷新任务 (100ms 周期)
// ============================================================================
void displayTask(void* arg) {
    DEBUG_PRINTLN("[RTOS] Display task started");

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t period = 100 / portTICK_PERIOD_MS;  // 100ms

    while (1) {
        if (xSemaphoreTake(displayMutex, 10)) {
            if (deviceStatus.ws_connected && systemStatus.valid) {
                display_system_status(systemStatus);
            } else {
                display_setup_mode(deviceStatus);
            }
            xSemaphoreGive(displayMutex);
        }

        vTaskDelayUntil(&lastWakeTime, period);
    }
}

// ============================================================================
// 命令处理 (从队列)
// ============================================================================
void process_command_from_queue(MotorCommand& cmd) {
    DEBUG_PRINTF("[RTOS] Processing command from queue: %s (id: %u)\n",
        cmd.command.c_str(), cmd.command_id);

    // 更新最后命令时间
    lastCommandTime = millis();

    // 执行命令 (简化版，实际可以复用 main.cpp 的 onCommandReceived)
    if (cmd.command == "start") {
        motorState.speed_percent = cmd.speed_percent > 0 ? cmd.speed_percent : 50;
        motorState.direction = cmd.direction;
        fsm_process_event(FSM_EVENT_START);
    }
    else if (cmd.command == "stop") {
        fsm_process_event(FSM_EVENT_STOP);
    }
    else if (cmd.command == "speed") {
        if (cmd.speed_percent >= 0 && cmd.speed_percent <= 100) {
            motorState.speed_percent = cmd.speed_percent;
            if (motorState.is_running) {
                esc_set_speed(cmd.speed_percent);
            }
        }
    }
    else if (cmd.command == "direction") {
        if (cmd.direction == 1 || cmd.direction == -1) {
            motorState.direction = cmd.direction;
        }
    }
}

// ============================================================================
// 安全检查 (Fail-safe) - RTOS 版本
// ============================================================================
void safety_check_rtos() {
    if (xSemaphoreTake(motorStateMutex, 5)) {
        if (!motorState.is_running) {
            xSemaphoreGive(motorStateMutex);
            return;
        }

        bool shouldStop = false;
        const char* reason = "";

        // 检查 1: WebSocket 断开
        if (!deviceStatus.ws_connected) {
            shouldStop = true;
            reason = "WS disconnected";
            alarm_trigger(ALARM_COMM_LOST);
        }
        // 检查 2: 通信超时
        else if (millis() - lastCommandTime > SAFETY_TIMEOUT) {
            shouldStop = true;
            reason = "Command timeout";
            alarm_trigger(ALARM_COMM_LOST);
        }

        if (shouldStop) {
            DEBUG_PRINTF("[SAFETY] Emergency stop: %s\n", reason);
            fsm_process_event(FSM_EVENT_ERROR);
        }

        xSemaphoreGive(motorStateMutex);
    }
}

// ============================================================================
// 紧急通知函数
// ============================================================================

void rtos_notify_emergency_from_isr() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // 从 ISR 发送任务通知
    if (motorTaskHandle != nullptr) {
        vTaskNotifyGiveFromISR(motorTaskHandle, &xHigherPriorityTaskWoken);
    }

    // 如果唤醒了更高优先级任务，触发上下文切换
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void rtos_notify_emergency() {
    // 从任务发送任务通知
    if (motorTaskHandle != nullptr) {
        xTaskNotify(motorTaskHandle, NOTIFY_EMERGENCY_STOP, eSetBits);
    }
}

// ============================================================================
// 栈监控
// ============================================================================

void rtos_check_stack() {
    // 检查各任务栈使用情况
    UBaseType_t stackHighWaterMark;

    if (motorTaskHandle) {
        stackHighWaterMark = uxTaskGetStackHighWaterMark(motorTaskHandle);
        if (stackHighWaterMark < 200) {  // 剩余小于 200 字节时警告
            DEBUG_PRINTF("[RTOS] WARNING: MotorTask stack low: %u bytes\n", stackHighWaterMark);
        }
    }

    if (wifiTaskHandle) {
        stackHighWaterMark = uxTaskGetStackHighWaterMark(wifiTaskHandle);
        if (stackHighWaterMark < 200) {
            DEBUG_PRINTF("[RTOS] WARNING: WiFiTask stack low: %u bytes\n", stackHighWaterMark);
        }
    }

    if (sensorTaskHandle) {
        stackHighWaterMark = uxTaskGetStackHighWaterMark(sensorTaskHandle);
        if (stackHighWaterMark < 200) {
            DEBUG_PRINTF("[RTOS] WARNING: SensorTask stack low: %u bytes\n", stackHighWaterMark);
        }
    }

    if (displayTaskHandle) {
        stackHighWaterMark = uxTaskGetStackHighWaterMark(displayTaskHandle);
        if (stackHighWaterMark < 200) {
            DEBUG_PRINTF("[RTOS] WARNING: DisplayTask stack low: %u bytes\n", stackHighWaterMark);
        }
    }
}

void rtos_print_stack_info() {
    DEBUG_PRINTLN("\n[RTOS] === Stack Usage ===");

    if (motorTaskHandle) {
        DEBUG_PRINTF("MotorTask:  %u bytes free\n", uxTaskGetStackHighWaterMark(motorTaskHandle));
    }
    if (wifiTaskHandle) {
        DEBUG_PRINTF("WiFiTask:   %u bytes free\n", uxTaskGetStackHighWaterMark(wifiTaskHandle));
    }
    if (sensorTaskHandle) {
        DEBUG_PRINTF("SensorTask: %u bytes free\n", uxTaskGetStackHighWaterMark(sensorTaskHandle));
    }
    if (displayTaskHandle) {
        DEBUG_PRINTF("DisplayTask:%u bytes free\n", uxTaskGetStackHighWaterMark(displayTaskHandle));
    }

    DEBUG_PRINTLN("[RTOS] ======================\n");
}

// ============================================================================
// 初始化函数
// ============================================================================

void rtos_init() {
    // 创建互斥锁
    motorStateMutex = xSemaphoreCreateMutex();
    displayMutex = xSemaphoreCreateMutex();

    if (motorStateMutex == nullptr || displayMutex == nullptr) {
        DEBUG_PRINTLN("[RTOS] ERROR: Failed to create mutexes");
    } else {
        DEBUG_PRINTLN("[RTOS] Mutexes created");
    }

    // 创建命令队列 (改进: 防止命令覆盖)
    commandQueue = xQueueCreate(CMD_QUEUE_LENGTH, sizeof(MotorCommand));

    if (commandQueue == nullptr) {
        DEBUG_PRINTLN("[RTOS] ERROR: Failed to create command queue");
    } else {
        DEBUG_PRINTF("[RTOS] Command queue created (size: %d)\n", CMD_QUEUE_LENGTH);
    }
}

void rtos_start() {
    DEBUG_PRINTLN("[RTOS] Starting tasks...");

    // 创建任务
    // WiFi 任务: 优先级 2, 4KB 栈
    xTaskCreate(
        wifiTask,
        "WiFi",
        4096,
        nullptr,
        2,
        &wifiTaskHandle
    );

    // 电机控制任务: 优先级 3 (最高), 2KB 栈
    xTaskCreate(
        motorTask,
        "Motor",
        2048,
        nullptr,
        3,
        &motorTaskHandle
    );

    // 传感器读取任务: 优先级 1, 2KB 栈
    xTaskCreate(
        sensorTask,
        "Sensor",
        2048,
        nullptr,
        1,
        &sensorTaskHandle
    );

    // 显示刷新任务: 优先级 1, 2KB 栈
    xTaskCreate(
        displayTask,
        "Display",
        2048,
        nullptr,
        1,
        &displayTaskHandle
    );

    DEBUG_PRINTLN("[RTOS] All tasks started");
}
