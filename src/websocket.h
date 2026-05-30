/**
 * @file websocket.h
 * @brief WebSocket Communication Module Header
 */

#pragma once

#include <Arduino.h>
#include "motor_state.h"

/**
 * @brief 命令回调函数类型
 */
typedef void (*CommandCallback)(MotorCommand& cmd);

/**
 * @brief 初始化 WebSocket 客户端
 */
void ws_init();

/**
 * @brief WebSocket 主循环 (需要在 loop() 中调用)
 */
void ws_loop();

/**
 * @brief 检查 WebSocket 是否已连接
 * @return true=已连接, false=未连接
 */
bool ws_is_connected();

/**
 * @brief 发送状态到服务器
 * @param state 电机状态
 */
void ws_send_status(MotorState& state);

/**
 * @brief 发送命令确认
 * @param command_id 命令 ID
 * @param success 是否成功
 * @param message 消息
 */
void ws_send_ack(uint32_t command_id, bool success, String message);

/**
 * @brief 设置命令回调函数
 * @param callback 回调函数
 */
void ws_set_command_callback(CommandCallback callback);

/**
 * @brief 发送心跳包
 */
void ws_send_heartbeat();

/**
 * @brief 获取最后错误消息
 * @return 错误消息
 */
String ws_get_last_error();
