/**
 * @file mqtt.h
 * @brief MQTT Client Module Header
 *
 * MQTT 数据上云功能，支持:
 * - 连接公共 MQTT Broker (如 EMQX)
 * - 设备数据上报 (JSON 格式)
 * - 命令订阅 (可选)
 * - 断线自动重连
 */

#pragma once

#include <Arduino.h>
#include "motor_state.h"

/**
 * @brief 初始化 MQTT 客户端
 */
void mqtt_init();

/**
 * @brief MQTT 主循环 (需在 loop 中调用)
 */
void mqtt_loop();

/**
 * @brief 连接到 MQTT Broker
 * @return true 连接成功
 */
bool mqtt_connect();

/**
 * @brief 断开 MQTT 连接
 */
void mqtt_disconnect();

/**
 * @brief 检查是否已连接
 * @return true 已连接
 */
bool mqtt_is_connected();

/**
 * @brief 发布设备数据
 * @param motorState 电机状态
 * @param deviceStatus 设备状态
 */
void mqtt_publish_data(MotorState& motorState, DeviceStatus& deviceStatus);

/**
 * @brief 发布系统状态 (PC 监控数据)
 * @param systemStatus 系统状态
 */
void mqtt_publish_system_status(SystemStatus& systemStatus);

/**
 * @brief 发布告警消息
 * @param alarm_type 告警类型
 * @param message 告警消息
 */
void mqtt_publish_alarm(const char* alarm_type, const char* message);

/**
 * @brief 发布在线状态
 * @param online 是否在线
 */
void mqtt_publish_online(bool online);
