/**
 * @file relay.h
 * @brief Relay Control Module Header
 */

#pragma once

#include <Arduino.h>

/**
 * @brief 初始化继电器
 */
void relay_init();

/**
 * @brief 开启继电器
 */
void relay_on();

/**
 * @brief 关闭继电器
 */
void relay_off();

/**
 * @brief 设置继电器状态
 * @param state true=开启, false=关闭
 */
void relay_set(bool state);

/**
 * @brief 切换继电器状态
 */
void relay_toggle();

/**
 * @brief 获取继电器当前状态
 * @return true=开启, false=关闭
 */
bool relay_get_state();

/**
 * @brief 获取继电器状态字符串
 * @return "on" 或 "off"
 */
String relay_get_state_string();
