/**
 * @file alarm.cpp
 * @brief Alarm and Protection System Implementation
 */

#include "alarm.h"
#include "config.h"
#include "motor_state.h"
#include "esc_pwm.h"
#include "relay.h"
#include "pid_controller.h"

// ============================================================================
// 内部状态
// ============================================================================
static AlarmState alarmState;

// ============================================================================
// 外部变量
// ============================================================================
extern MotorState motorState;

// ============================================================================
// 初始化与配置
// ============================================================================

void alarm_init() {
    alarmState.active_alarm = ALARM_NONE;
    alarmState.overcurrent_threshold = ALARM_OVERCURRENT_THRESHOLD;
    alarmState.overtemp_threshold = ALARM_OVERTEMP_THRESHOLD;
    alarmState.stall_timeout_ms = ALARM_STALL_TIMEOUT;
    alarmState.stall_rpm_threshold = 100;    // RPM < 100 视为堵转
    alarmState.stall_detected = false;
    alarmState.stall_start_time = 0;
    alarmState.alarm_time = 0;

    DEBUG_PRINTF("[ALARM] Initialized: OC=%.1fA, OT=%.0f°C, Stall=%ums\n",
        alarmState.overcurrent_threshold, alarmState.overtemp_threshold, alarmState.stall_timeout_ms);
}

void alarm_set_thresholds(float current, float temp, uint32_t stall_ms) {
    if (current > 0) alarmState.overcurrent_threshold = current;
    if (temp > 0) alarmState.overtemp_threshold = temp;
    if (stall_ms > 0) alarmState.stall_timeout_ms = stall_ms;

    DEBUG_PRINTF("[ALARM] Thresholds updated: OC=%.1fA, OT=%.0f°C, Stall=%ums\n",
        alarmState.overcurrent_threshold, alarmState.overtemp_threshold, alarmState.stall_timeout_ms);
}

// ============================================================================
// 检测与触发
// ============================================================================

void alarm_check() {
    // 只在电机运行时检查
    if (!motorState.is_running) return;

    // ---- 检查 1: 过流 ----
    if (motorState.current > alarmState.overcurrent_threshold) {
        alarm_trigger(ALARM_OVERCURRENT);
        return;
    }

    // ---- 检查 2: 过温 ----
    if (motorState.temperature > alarmState.overtemp_threshold) {
        alarm_trigger(ALARM_OVERTEMP);
        return;
    }

    // ---- 检查 3: 堵转 (PWM > 10% 但 RPM 极低) ----
    if (motorState.speed_percent > 10 && motorState.actual_rpm < alarmState.stall_rpm_threshold) {
        if (!alarmState.stall_detected) {
            // 首次检测到堵转，记录时间
            alarmState.stall_detected = true;
            alarmState.stall_start_time = millis();
            DEBUG_PRINTLN("[ALARM] Stall condition detected, monitoring...");
        } else {
            // 持续堵转，检查是否超过超时
            if (millis() - alarmState.stall_start_time > alarmState.stall_timeout_ms) {
                alarm_trigger(ALARM_STALL);
                return;
            }
        }
    } else {
        // 堵转条件消失，重置
        alarmState.stall_detected = false;
    }
}

void alarm_trigger(AlarmType type) {
    alarmState.active_alarm = type;
    alarmState.alarm_time = millis();

    DEBUG_PRINTF("[ALARM] *** TRIGGERED: %s ***\n", alarm_get_name(type));

    // 紧急停机
    esc_stop();
    relay_off();

    // 更新电机状态
    motorState.is_running = false;
    motorState.speed_percent = 0;

    // 重置 PID
    pid_reset();  // 需要 pid_controller.h

    // 重置堵转检测
    alarmState.stall_detected = false;
}

void alarm_clear() {
    alarmState.active_alarm = ALARM_NONE;
    alarmState.stall_detected = false;
    alarmState.alarm_time = 0;

    DEBUG_PRINTLN("[ALARM] Cleared");
}

// ============================================================================
// 状态查询
// ============================================================================

AlarmState alarm_get_state() {
    return alarmState;
}

bool alarm_is_active() {
    return alarmState.active_alarm != ALARM_NONE;
}

const char* alarm_get_name(AlarmType type) {
    switch (type) {
        case ALARM_NONE:        return "NONE";
        case ALARM_OVERCURRENT: return "OVERCURRENT";
        case ALARM_OVERTEMP:    return "OVERTEMP";
        case ALARM_STALL:       return "STALL";
        case ALARM_COMM_LOST:   return "COMM_LOST";
        default:                return "UNKNOWN";
    }
}
