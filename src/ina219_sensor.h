/**
 * @file ina219_sensor.h
 * @brief INA219 Current/Voltage/Power Sensor
 *
 * 通过 I2C 接口读取 INA219 传感器数据:
 *   - 电流 (A)
 *   - 电压 (V)
 *   - 功率 (W)
 *
 * 使用流程:
 *   1. ina219_init()     - 初始化 I2C 和传感器
 *   2. ina219_read()     - 读取所有数据
 *   3. ina219_get_xxx()  - 获取具体数值
 */

#pragma once

#include <Arduino.h>

/**
 * @brief 初始化 INA219 传感器
 *
 * 配置 I2C 接口并校准传感器
 * @return true 成功, false 失败
 */
bool ina219_init();

/**
 * @brief 读取传感器所有数据
 *
 * 更新内部的电流、电压、功率值
 */
void ina219_read();

/**
 * @brief 获取电流值
 * @return 电流 (A)
 */
float ina219_get_current();

/**
 * @brief 获取电压值
 * @return 电压 (V)
 */
float ina219_get_voltage();

/**
 * @brief 获取功率值
 * @return 功率 (W)
 */
float ina219_get_power();

/**
 * @brief 检查传感器是否就绪
 * @return true 就绪, false 未就绪
 */
bool ina219_is_ready();
