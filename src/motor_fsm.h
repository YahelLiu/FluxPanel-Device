/**
 * @file motor_fsm.h
 * @brief Motor Finite State Machine
 *
 * 电机状态机管理生命周期:
 *
 *   IDLE ──start──→ STARTING ──armed──→ RUNNING
 *     ↑                                   │
 *     │         stop/error                │
 *     └───────────────────────────────────┘
 *                      │
 *                      ↓ error
 *                    ERROR ──reset──→ IDLE
 *
 * 每个状态有明确的进入/退出动作
 * 状态转换由事件驱动
 */

#pragma once

#include <Arduino.h>
#include "motor_state.h"

// ============================================================================
// 事件定义
// ============================================================================
#define FSM_EVENT_START    1     // 启动命令
#define FSM_EVENT_STOP     2     // 停止命令
#define FSM_EVENT_ERROR    3     // 错误事件
#define FSM_EVENT_RESET    4     // 复位命令
#define FSM_EVENT_ARMED    5     // ESC 校准完成

// ============================================================================
// 函数接口
// ============================================================================

/**
 * @brief 初始化状态机
 *
 * 设置初始状态为 IDLE
 */
void fsm_init();

/**
 * @brief 处理状态机事件
 *
 * 根据当前状态和事件执行状态转换
 *
 * @param event 事件类型 (FSM_EVENT_xxx)
 */
void fsm_process_event(uint8_t event);

/**
 * @brief 状态机周期更新
 *
 * 处理状态相关的周期性任务
 * 应在定时任务中调用
 */
void fsm_update();

/**
 * @brief 获取当前状态名称
 * @return 状态名称字符串
 */
const char* fsm_get_state_name();

/**
 * @brief 获取当前状态
 * @return MotorStateEnum 枚举值
 */
MotorStateEnum fsm_get_state();
