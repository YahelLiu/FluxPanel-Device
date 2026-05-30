/**
 * @file hall_sensor.h
 * @brief Hall Effect Sensor for RPM Measurement
 *
 * 通过 GPIO 中断计数霍尔传感器脉冲，计算电机转速 (RPM)
 *
 * 计算公式:
 *   RPM = (脉冲数 / 极对数) * (60 / 时间)
 *
 * 使用流程:
 *   1. hall_init()        - 初始化 GPIO 中断
 *   2. hall_get_rpm()     - 获取当前转速
 *   3. hall_set_pole_pairs() - 设置电机极对数
 */

#pragma once

#include <Arduino.h>

/**
 * @brief 初始化霍尔传感器
 *
 * 配置 GPIO 为输入模式，启用上升沿中断
 */
void hall_init();

/**
 * @brief 重置脉冲计数器
 */
void hall_reset();

/**
 * @brief 获取累计脉冲数
 * @return 脉冲计数
 */
uint32_t hall_get_pulses();

/**
 * @brief 计算并获取当前转速 (RPM)
 *
 * 基于两次调用之间的脉冲差值计算转速
 *
 * @return 转速 (RPM)
 */
float hall_get_rpm();

/**
 * @brief 设置电机极对数
 *
 * @param pairs 极对数 (常见值: 7, 14)
 */
void hall_set_pole_pairs(uint8_t pairs);

/**
 * @brief 获取当前极对数
 * @return 极对数
 */
uint8_t hall_get_pole_pairs();
