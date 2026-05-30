/**
 * @file wifi_manager.h
 * @brief WiFi Manager Module Header
 *
 * 支持两种 WiFi 连接模式:
 *
 * 模式1: WiFiManager 网页配网 (WIFI_USE_CONFIG=false)
 *   - 首次开机 ESP32 开启热点 (如 "ESP32_MotorSetup")
 *   - 手机连接热点，打开 192.168.4.1
 *   - 在网页中选择 WiFi 并输入密码
 *   - 保存后自动连接，下次开机自动连接
 *
 * 模式2: 硬编码 WiFi (WIFI_USE_CONFIG=true)
 *   - WiFi 名称密码写死在 config.h 中
 *   - 开机直接连接，无需配网
 *
 * 通过 config.h 中的 WIFI_USE_CONFIG 宏选择模式
 */

#pragma once

#include <Arduino.h>

/**
 * @brief 初始化 WiFi
 * @return true=连接成功, false=失败或进入配网模式
 */
bool wifi_manager_init();

/**
 * @brief 处理 WiFi 事件 (需要在 loop 中调用)
 */
void wifi_manager_loop();

/**
 * @brief 检查是否已连接
 * @return true=已连接
 */
bool wifi_manager_is_connected();

/**
 * @brief 获取 IP 地址
 * @return IP 地址字符串
 */
String wifi_manager_get_ip();

/**
 * @brief 重置 WiFi 配置
 * - WiFiManager 模式: 清除保存的 WiFi
 * - 硬编码模式: 触发重连
 */
void wifi_manager_reset();

/**
 * @brief 设置配网模式回调 (进入配网页面时调用，仅 WiFiManager 模式)
 */
void wifi_manager_set_config_callback(void (*callback)());

/**
 * @brief 设置连接成功回调
 */
void wifi_manager_set_connected_callback(void (*callback)(String ip));
