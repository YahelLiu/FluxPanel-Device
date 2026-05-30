/**
 * @file rtos_tasks.h
 * @brief FreeRTOS Task Definitions
 *
 * ESP32-C3 多任务架构:
 *   - WiFiTask:   WiFi/WebSocket 通信 (优先级 2)
 *   - MotorTask:  电机控制 + PID  (优先级 3, 最高)
 *   - SensorTask: 传感器读取      (优先级 1)
 *   - DisplayTask: LCD 刷新       (优先级 1)
 */

#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// ============================================================================
// 任务句柄
// ============================================================================
extern TaskHandle_t wifiTaskHandle;
extern TaskHandle_t motorTaskHandle;
extern TaskHandle_t sensorTaskHandle;
extern TaskHandle_t displayTaskHandle;

// ============================================================================
// 互斥锁
// ============================================================================
extern SemaphoreHandle_t motorStateMutex;
extern SemaphoreHandle_t displayMutex;

// ============================================================================
// 任务函数
// ============================================================================
void wifiTask(void* arg);
void motorTask(void* arg);
void sensorTask(void* arg);
void displayTask(void* arg);

// ============================================================================
// 初始化函数
// ============================================================================

/**
 * @brief 创建互斥锁
 */
void rtos_init();

/**
 * @brief 启动所有任务
 */
void rtos_start();
