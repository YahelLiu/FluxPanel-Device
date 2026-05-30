/**
 * @file motor_state.h
 * @brief Motor state data structures
 */

#pragma once

#include <Arduino.h>

// ============================================================================
// 电机状态枚举 (状态机)
// ============================================================================
enum MotorStateEnum {
    STATE_IDLE = 0,       // 空闲
    STATE_STARTING = 1,   // 启动中
    STATE_RUNNING = 2,    // 运行中
    STATE_STOPPING = 3,   // 停止中
    STATE_ERROR = 4       // 错误
};

/**
 * @brief 系统状态结构体 (来自服务器的 Yves-PC 数据)
 */
struct SystemStatus {
    float cpu_load = 0;           // CPU 负载 %
    float cpu_temp = 0;           // CPU 温度
    float cpu_power = 0;          // CPU 功率 W
    float gpu_load = 0;           // GPU 负载 %
    float gpu_temp = 0;           // GPU 温度
    float gpu_power = 0;          // GPU 功率 W
    int mem_load = 0;             // 内存占用 %
    bool valid = false;           // 数据是否有效
    unsigned long last_update = 0;

    void reset() {
        cpu_load = 0;
        cpu_temp = 0;
        cpu_power = 0;
        gpu_load = 0;
        gpu_temp = 0;
        gpu_power = 0;
        mem_load = 0;
        valid = false;
        last_update = millis();
    }
};

/**
 * @brief 电机状态结构体
 */
struct MotorState {
    // 状态机状态
    MotorStateEnum state = STATE_IDLE;  // 当前状态

    bool is_running = false;          // 是否运行中
    int speed_percent = 0;            // 转速百分比 (0-100)
    int direction = 1;                // 方向 (1=正转, -1=反转)
    float temperature = 0.0;          // 温度 (°C)
    float current = 0.0;              // 电流 (A)
    float voltage = 0.0;              // 电压 (V)
    unsigned long last_update = 0;    // 最后更新时间戳

    // PID 闭环控制相关
    bool pid_enabled = false;         // PID 模式开关
    float target_rpm = 0;             // 目标转速 (RPM)
    float actual_rpm = 0;             // 实际转速 (RPM, 来自霍尔传感器)

    /**
     * @brief 重置状态
     */
    void reset() {
        state = STATE_IDLE;
        is_running = false;
        speed_percent = 0;
        direction = 1;
        temperature = 0.0;
        current = 0.0;
        voltage = 0.0;
        last_update = millis();
        pid_enabled = false;
        target_rpm = 0;
        actual_rpm = 0;
    }
};

/**
 * @brief 电机命令结构体
 */
struct MotorCommand {
    uint32_t command_id = 0;          // 命令 ID
    String command = "";              // 命令类型 (start, stop, speed, direction, lcd, pid_mode, pid_tune)
    int speed_percent = 0;            // 转速参数
    int direction = 1;                // 方向参数
    // LCD 显示参数
    String content = "";              // LCD 显示内容
    int line = 0;                     // LCD 行号 (0-3)
    bool clear_screen = false;        // 是否清屏
    // PID 控制参数
    bool pid_enabled = false;         // PID 模式开关
    float target_rpm = 0;             // 目标转速
    float pid_kp = 0;                 // PID 比例系数
    float pid_ki = 0;                 // PID 积分系数
    float pid_kd = 0;                 // PID 微分系数

    /**
     * @brief 清空命令
     */
    void clear() {
        command_id = 0;
        command = "";
        speed_percent = 0;
        direction = 1;
        content = "";
        line = 0;
        clear_screen = false;
        pid_enabled = false;
        target_rpm = 0;
        pid_kp = 0;
        pid_ki = 0;
        pid_kd = 0;
    }
};

/**
 * @brief 设备连接状态
 */
struct DeviceStatus {
    bool wifi_connected = false;      // WiFi 连接状态
    bool ws_connected = false;        // WebSocket 连接状态
    String ip_address = "";           // IP 地址
    unsigned long uptime = 0;         // 运行时间 (秒)
};
