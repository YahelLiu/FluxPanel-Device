/**
 * @file display.h
 * @brief ST7735 TFT LCD Display Module Header
 */

#pragma once

#include <Arduino.h>
#include "motor_state.h"

/**
 * @brief 初始化显示屏
 */
void display_init();

/**
 * @brief 清屏
 */
void display_clear();

/**
 * @brief 显示 WiFi 状态
 * @param connected 是否已连接
 * @param ip IP 地址
 */
void display_wifi_status(bool connected, String ip);

/**
 * @brief 显示 WebSocket 状态
 * @param connected 是否已连接
 */
void display_ws_status(bool connected);

/**
 * @brief 显示电机状态
 * @param state 电机状态
 */
void display_motor_state(MotorState& state);

/**
 * @brief 显示设备状态
 * @param status 设备状态
 */
void display_device_status(DeviceStatus& status);

/**
 * @brief 显示接收到的命令
 * @param command 命令类型
 * @param command_id 命令 ID
 */
void display_command(String command, uint32_t command_id);

/**
 * @brief 显示日志消息
 * @param message 日志内容
 */
void display_log(String message);

/**
 * @brief 显示完整界面
 * @param status 设备状态
 * @param motorState 电机状态
 */
void display_update_all(DeviceStatus& status, MotorState& motorState);

/**
 * @brief 显示错误信息
 * @param error 错误信息
 */
void display_error(String error);

/**
 * @brief 在指定行显示 LCD 内容
 * @param line 行号 (0-3)
 * @param content 显示内容
 * @param clear 是否先清屏
 */
void display_lcd_line(int line, String content, bool clear_screen);

/**
 * @brief 显示系统状态模式 (WS 已连接时)
 * @param status 系统状态
 */
void display_system_status(SystemStatus& status);

/**
 * @brief 显示设置模式 (WS 未连接时)
 * @param status 设备状态
 */
void display_setup_mode(DeviceStatus& status);
