/**
 * @file rtos_tasks.h
 * @brief FreeRTOS Task Definitions
 *
 * ESP32-C3 多任务架构:
 *   - WiFiTask:   WiFi/WebSocket 通信 (优先级 2)
 *   - MotorTask:  电机控制 + PID  (优先级 3, 最高)
 *   - SensorTask: 传感器读取      (优先级 1)
 *   - DisplayTask: LCD 刷新       (优先级 1)
 *
 * RTOS 同步机制:
 *   - 命令队列: WebSocket → MotorTask (防止命令覆盖)
 *   - 任务通知: 紧急停机 (微秒级响应)
 *   - 信号量: 共享数据保护
 *   - 栈监控: 防止栈溢出
 */

#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

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
// 命令队列 (改进: 防止命令覆盖)
// ============================================================================
#define CMD_QUEUE_LENGTH 8    // 队列容量
#define CMD_QUEUE_TIMEOUT 100 // 发送超时 (ms)

extern QueueHandle_t commandQueue;

// ============================================================================
// 任务通知值 (改进: 紧急事件)
// ============================================================================
#define NOTIFY_EMERGENCY_STOP  (1 << 0)  // 紧急停机
#define_NOTIFY_ALARM           (1 << 1)  // 报警触发
#define NOTIFY_COMMAND_PENDING (1 << 2)  // 有新命令

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
 * @brief 创建互斥锁和队列
 */
void rtos_init();

/**
 * @brief 启动所有任务
 */
void rtos_start();

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 发送紧急停机通知 (从中断调用)
 */
void rtos_notify_emergency_from_isr();

/**
 * @brief 发送紧急停机通知 (从任务调用)
 */
void rtos_notify_emergency();

/**
 * @brief 检查任务栈使用情况
 */
void rtos_check_stack();

/**
 * @brief 打印所有任务栈使用情况
 */
void rtos_print_stack_info();
