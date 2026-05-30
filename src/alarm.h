/**
 * @file alarm.h
 * @brief Alarm and Protection System
 *
 * 多维度异常检测和自动保护:
 *   - 过流检测 (Overcurrent)
 *   - 过温检测 (Overtemperature)
 *   - 堵转检测 (Stall)
 *   - 通信丢失检测 (Communication Lost)
 *
 * 检测到异常后自动触发紧急停机
 */

#pragma once

#include <Arduino.h>

// ============================================================================
// 报警类型枚举
// ============================================================================
enum AlarmType {
    ALARM_NONE = 0,         // 无报警
    ALARM_OVERCURRENT = 1,  // 过流
    ALARM_OVERTEMP = 2,     // 过温
    ALARM_STALL = 3,        // 堵转
    ALARM_COMM_LOST = 4     // 通信丢失
};

// ============================================================================
// 报警状态结构体
// ============================================================================
struct AlarmState {
    AlarmType active_alarm;         // 当前活动报警
    float overcurrent_threshold;    // 过流阈值 (A)
    float overtemp_threshold;       // 过温阈值 (°C)
    uint32_t stall_timeout_ms;      // 堵转超时时间 (ms)
    uint32_t stall_rpm_threshold;   // 堵转转速阈值 (RPM)
    bool stall_detected;            // 堵转检测标志
    unsigned long stall_start_time; // 堵转开始时间
    unsigned long alarm_time;       // 报警触发时间
};

// ============================================================================
// 初始化与配置
// ============================================================================

/**
 * @brief 初始化报警系统
 */
void alarm_init();

/**
 * @brief 设置报警阈值
 *
 * @param current 过流阈值 (A)
 * @param temp    过温阈值 (°C)
 * @param stall_ms 堵转超时 (ms)
 */
void alarm_set_thresholds(float current, float temp, uint32_t stall_ms);

// ============================================================================
// 检测与触发
// ============================================================================

/**
 * @brief 执行报警检查
 *
 * 检查过流、过温、堵转等条件
 * 应在定时任务中调用
 */
void alarm_check();

/**
 * @brief 触发报警
 *
 * @param type 报警类型
 */
void alarm_trigger(AlarmType type);

/**
 * @brief 清除报警状态
 */
void alarm_clear();

// ============================================================================
// 状态查询
// ============================================================================

/**
 * @brief 获取当前报警状态
 * @return 报警状态结构体
 */
AlarmState alarm_get_state();

/**
 * @brief 检查是否有活动报警
 * @return true 有报警, false 无报警
 */
bool alarm_is_active();

/**
 * @brief 获取报警类型名称
 * @param type 报警类型
 * @return 名称字符串
 */
const char* alarm_get_name(AlarmType type);
