/**
 * @file pid_controller.cpp
 * @brief PID Controller Implementation
 *
 * 标准位置式 PID 算法:
 *   output = Kp * e + Ki * ∫e dt + Kd * de/dt
 *
 * 特性:
 *   - 积分抗饱和 (clamping)
 *   - 输出限幅
 *   - 可在线调参
 */

#include "pid_controller.h"
#include "config.h"

// ============================================================================
// 内部状态
// ============================================================================
static PID_TypeDef pid;

// ============================================================================
// 初始化与配置
// ============================================================================

void pid_init(float Kp, float Ki, float Kd) {
    pid.Kp = Kp;
    pid.Ki = Ki;
    pid.Kd = Kd;
    pid.setpoint = 0;
    pid.input = 0;
    pid.output = 0;
    pid.integral = 0;
    pid.lastError = 0;
    pid.outputMin = 0;
    pid.outputMax = 100;  // PWM 百分比
    pid.lastTime = millis();

    DEBUG_PRINTF("[PID] Initialized: Kp=%.2f, Ki=%.2f, Kd=%.2f\n", Kp, Ki, Kd);
}

void pid_set_tunings(float Kp, float Ki, float Kd) {
    pid.Kp = Kp;
    pid.Ki = Ki;
    pid.Kd = Kd;

    DEBUG_PRINTF("[PID] Tunings set: Kp=%.2f, Ki=%.2f, Kd=%.2f\n", Kp, Ki, Kd);
}

void pid_set_setpoint(float setpoint) {
    pid.setpoint = setpoint;

    DEBUG_PRINTF("[PID] Setpoint: %.1f RPM\n", setpoint);
}

void pid_set_output_limits(float min, float max) {
    pid.outputMin = min;
    pid.outputMax = max;

    DEBUG_PRINTF("[PID] Output limits: %.1f ~ %.1f\n", min, max);
}

// ============================================================================
// PID 计算
// ============================================================================

float pid_compute(float input) {
    // 计算时间增量
    unsigned long now = millis();
    float dt = (now - pid.lastTime) / 1000.0;

    // 防止 dt 为 0 或负数
    if (dt <= 0 || dt > 1.0) {
        dt = 0.01;  // 默认 10ms
    }

    pid.lastTime = now;
    pid.input = input;

    // 计算误差
    float error = pid.setpoint - input;

    // 比例项
    float P = pid.Kp * error;

    // 积分项 (带抗饱和)
    pid.integral += error * dt;

    // 积分限幅 (抗饱和)
    float integralMax = (pid.outputMax - P) / pid.Ki;
    float integralMin = (pid.outputMin - P) / pid.Ki;

    if (pid.integral > integralMax) pid.integral = integralMax;
    if (pid.integral < integralMin) pid.integral = integralMin;

    float I = pid.Ki * pid.integral;

    // 微分项
    float D = 0;
    if (dt > 0) {
        D = pid.Kd * (error - pid.lastError) / dt;
    }
    pid.lastError = error;

    // 计算总输出
    pid.output = P + I + D;

    // 输出限幅
    if (pid.output > pid.outputMax) pid.output = pid.outputMax;
    if (pid.output < pid.outputMin) pid.output = pid.outputMin;

    return pid.output;
}

// ============================================================================
// 状态查询
// ============================================================================

void pid_reset() {
    pid.integral = 0;
    pid.lastError = 0;
    pid.output = 0;
    pid.lastTime = millis();

    DEBUG_PRINTLN("[PID] Reset");
}

float pid_get_output() {
    return pid.output;
}

void pid_get_tunings(float* Kp, float* Ki, float* Kd) {
    if (Kp) *Kp = pid.Kp;
    if (Ki) *Ki = pid.Ki;
    if (Kd) *Kd = pid.Kd;
}
