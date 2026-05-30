/**
 * @file motor_fsm.cpp
 * @brief Motor Finite State Machine Implementation
 */

#include "motor_fsm.h"
#include "config.h"
#include "esc_pwm.h"
#include "relay.h"
#include "pid_controller.h"

// ============================================================================
// 外部变量
// ============================================================================
extern MotorState motorState;

// ============================================================================
// 内部状态
// ============================================================================
static MotorStateEnum currentState = STATE_IDLE;

// ============================================================================
// 状态名称
// ============================================================================
static const char* stateNames[] = {
    "IDLE",       // STATE_IDLE
    "STARTING",   // STATE_STARTING
    "RUNNING",    // STATE_RUNNING
    "STOPPING",   // STATE_STOPPING
    "ERROR"       // STATE_ERROR
};

// ============================================================================
// 状态转换函数
// ============================================================================

/**
 * @brief 进入 STARTING 状态
 */
static void enter_starting() {
    DEBUG_PRINTLN("[FSM] Entering STARTING state");

    // ESC 校准
    esc_arm();

    // 接通继电器
    relay_on();

    motorState.is_running = true;
}

/**
 * @brief 进入 RUNNING 状态
 */
static void enter_running() {
    DEBUG_PRINTLN("[FSM] Entering RUNNING state");

    // 设置转速
    if (motorState.speed_percent > 0) {
        esc_set_speed(motorState.speed_percent);
    } else {
        motorState.speed_percent = 50;  // 默认 50%
        esc_set_speed(50);
    }
}

/**
 * @brief 进入 STOPPING 状态
 */
static void enter_stopping() {
    DEBUG_PRINTLN("[FSM] Entering STOPPING state");

    // 停止 PWM
    esc_stop();

    // 断开继电器
    relay_off();

    // 重置状态
    motorState.is_running = false;
    motorState.speed_percent = 0;
    motorState.pid_enabled = false;

    // 重置 PID
    pid_reset();
}

/**
 * @brief 进入 ERROR 状态
 */
static void enter_error() {
    DEBUG_PRINTLN("[FSM] Entering ERROR state");

    // 紧急停机
    esc_stop();
    relay_off();

    // 更新状态
    motorState.is_running = false;
    motorState.speed_percent = 0;
    motorState.pid_enabled = false;

    // 重置 PID
    pid_reset();
}

// ============================================================================
// 公共函数
// ============================================================================

void fsm_init() {
    currentState = STATE_IDLE;
    motorState.state = STATE_IDLE;

    DEBUG_PRINTLN("[FSM] Initialized, state: IDLE");
}

void fsm_process_event(uint8_t event) {
    MotorStateEnum prevState = currentState;

    switch (currentState) {
        // ---- IDLE 状态 ----
        case STATE_IDLE:
            if (event == FSM_EVENT_START) {
                currentState = STATE_STARTING;
                enter_starting();
            }
            break;

        // ---- STARTING 状态 ----
        case STATE_STARTING:
            if (event == FSM_EVENT_ARMED) {
                currentState = STATE_RUNNING;
                enter_running();
            }
            else if (event == FSM_EVENT_ERROR) {
                currentState = STATE_ERROR;
                enter_error();
            }
            else if (event == FSM_EVENT_STOP) {
                currentState = STATE_STOPPING;
                enter_stopping();
            }
            break;

        // ---- RUNNING 状态 ----
        case STATE_RUNNING:
            if (event == FSM_EVENT_STOP) {
                currentState = STATE_STOPPING;
                enter_stopping();
            }
            else if (event == FSM_EVENT_ERROR) {
                currentState = STATE_ERROR;
                enter_error();
            }
            break;

        // ---- STOPPING 状态 ----
        case STATE_STOPPING:
            // 自动转换到 IDLE
            currentState = STATE_IDLE;
            motorState.is_running = false;
            break;

        // ---- ERROR 状态 ----
        case STATE_ERROR:
            if (event == FSM_EVENT_RESET) {
                currentState = STATE_IDLE;
                motorState.is_running = false;
                DEBUG_PRINTLN("[FSM] Reset to IDLE");
            }
            break;
    }

    // 更新 MotorState 中的状态
    motorState.state = currentState;

    // 状态变化日志
    if (prevState != currentState) {
        DEBUG_PRINTF("[FSM] State: %s → %s (event=%d)\n",
            stateNames[prevState], stateNames[currentState], event);
    }
}

void fsm_update() {
    // 处理 STARTING → RUNNING 自动转换
    if (currentState == STATE_STARTING && esc_is_armed()) {
        fsm_process_event(FSM_EVENT_ARMED);
    }

    // 处理 STOPPING → IDLE 自动转换
    if (currentState == STATE_STOPPING) {
        fsm_process_event(FSM_EVENT_STOP);  // 触发 IDLE 转换
    }
}

const char* fsm_get_state_name() {
    if (currentState >= 0 && currentState <= 4) {
        return stateNames[currentState];
    }
    return "UNKNOWN";
}

MotorStateEnum fsm_get_state() {
    return currentState;
}
