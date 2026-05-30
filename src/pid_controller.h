/**
 * @file pid_controller.h
 * @brief PID Controller for Motor Speed Control
 *
 * PID 闭环控制实现:
 *   - 目标 RPM → PID 计算 → PWM 输出 → 实际 RPM
 *   - 支持参数在线调整
 *   - 积分抗饱和
 */

#pragma once

#include <Arduino.h>

// ============================================================================
// PID 结构体
// ============================================================================
typedef struct {
    float Kp;               // 比例系数
    float Ki;               // 积分系数
    float Kd;               // 微分系数
    float setpoint;         // 目标值 (RPM)
    float input;            // 输入值 (实际 RPM)
    float output;           // 输出值 (PWM %)
    float integral;         // 积分累积
    float lastError;        // 上次误差
    float outputMin;        // 输出下限
    float outputMax;        // 输出上限
    unsigned long lastTime; // 上次计算时间
} PID_TypeDef;

// ============================================================================
// 初始化与配置
// ============================================================================

/**
 * @brief 初始化 PID 控制器
 *
 * @param Kp 比例系数
 * @param Ki 积分系数
 * @param Kd 微分系数
 */
void pid_init(float Kp, float Ki, float Kd);

/**
 * @brief 设置 PID 参数
 *
 * @param Kp 比例系数
 * @param Ki 积分系数
 * @param Kd 微分系数
 */
void pid_set_tunings(float Kp, float Ki, float Kd);

/**
 * @brief 设置目标值
 *
 * @param setpoint 目标 RPM
 */
void pid_set_setpoint(float setpoint);

/**
 * @brief 设置输出范围
 *
 * @param min 最小输出 (默认 0)
 * @param max 最大输出 (默认 100)
 */
void pid_set_output_limits(float min, float max);

// ============================================================================
// 计算与状态
// ============================================================================

/**
 * @brief 执行 PID 计算
 *
 * @param input 当前测量值 (实际 RPM)
 * @return PID 输出值 (PWM 百分比 0-100)
 */
float pid_compute(float input);

/**
 * @brief 重置 PID 状态
 *
 * 清除积分累积和上次误差
 */
void pid_reset();

/**
 * @brief 获取当前输出值
 *
 * @return 输出值 (PWM %)
 */
float pid_get_output();

/**
 * @brief 获取当前 PID 参数
 *
 * @param Kp 输出比例系数
 * @param Ki 输出积分系数
 * @param Kd 输出微分系数
 */
void pid_get_tunings(float* Kp, float* Ki, float* Kd);
