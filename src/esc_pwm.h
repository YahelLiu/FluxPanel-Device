/**
 * @file esc_pwm.h
 * @brief ESC PWM Control Module
 *
 * 通过 ESP32 LEDC 输出 50Hz PWM 信号控制 BLDC 电调 (ESC)
 *
 * PWM 参数:
 *   - 频率: 50Hz (标准 ESC 信号)
 *   - 脉宽范围: 1000us (停止) ~ 2000us (全速)
 *   - 分辨率: 16-bit
 *
 * 使用流程:
 *   1. esc_init()   - 初始化 PWM 输出, 默认最小脉宽 (安全)
 *   2. esc_arm()    - ESC 校准流程
 *   3. esc_set_speed() - 设置转速 0-100%
 *   4. esc_stop()   - 停止输出
 */

#pragma once

#include <Arduino.h>

/**
 * @brief 初始化 ESC PWM 输出
 *
 * 配置 LEDC 定时器和通道, 输出最小脉宽 (1000us) 作为安全初始状态
 * 上电后 ESC 处于未校准状态, 需要调用 esc_arm() 后才能控制
 */
void esc_init();

/**
 * @brief ESC 启动校准
 *
 * 输出最小脉宽信号并等待 ESC 自检完成
 * 校准完成后才能通过 esc_set_speed() 控制转速
 */
void esc_arm();

/**
 * @brief 设置电机转速
 *
 * @param percent 转速百分比 (0-100)
 *                0   = 1000us 脉宽 (停止)
 *                100 = 2000us 脉宽 (全速)
 *
 * 注意: 必须先调用 esc_arm() 完成校准, 否则不执行
 */
void esc_set_speed(uint8_t percent);

/**
 * @brief 停止 PWM 输出, 归零到最小脉宽
 */
void esc_stop();

/**
 * @brief 获取当前转速设置
 * @return 转速百分比 (0-100)
 */
uint8_t esc_get_speed();

/**
 * @brief 查询 ESC 是否已校准
 * @return true 已校准, false 未校准
 */
bool esc_is_armed();
