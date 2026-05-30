/**
 * @file esc_pwm.cpp
 * @brief ESC PWM Control Module Implementation
 */

#include "esc_pwm.h"
#include "config.h"

// ============================================================================
// 内部状态
// ============================================================================

static uint8_t currentSpeed = 0;    // 当前转速百分比
static bool escArmed = false;       // ESC 是否已校准

// ============================================================================
// 内部函数
// ============================================================================

/**
 * @brief 将转速百分比转换为 LEDC 占空比
 *
 * @param percent 转速百分比 (0-100)
 * @return LEDC 占空比值 (0-65535)
 *
 * 计算公式:
 *   pulse_us = 1000 + (percent * 10)  // 1000us ~ 2000us
 *   duty = (pulse_us * freq * resolution) / 1000000
 */
static uint32_t speed_to_duty(uint8_t percent) {
    // 限制范围
    if (percent > 100) percent = 100;

    // 计算脉宽 (us): 1000us ~ 2000us
    uint32_t pulse_us = ESC_MIN_PULSE + (percent * (ESC_MAX_PULSE - ESC_MIN_PULSE) / 100);

    // 计算占空比: duty = (pulse_us * freq * 2^res) / 1000000
    // 对于 50Hz, 16-bit: duty = pulse_us * 50 * 65535 / 1000000 ≈ pulse_us * 3.27675
    uint32_t duty = (pulse_us * ESC_PWM_FREQ * ((1 << ESC_PWM_RES) - 1)) / 1000000;

    return duty;
}

// ============================================================================
// 公共函数
// ============================================================================

void esc_init() {
    // 1. 配置 GPIO 为输出, 默认低电平 (安全)
    pinMode(ESC_PWM_PIN, OUTPUT);
    digitalWrite(ESC_PWM_PIN, LOW);

    // 2. 配置 LEDC 定时器
    // ESP32 Arduino 2.x API: ledcSetup() 和 ledcAttach()
    ledcSetup(ESC_PWM_CHANNEL, ESC_PWM_FREQ, ESC_PWM_RES);

    // 3. 绑定 GPIO 到 LEDC 通道
    ledcAttachPin(ESC_PWM_PIN, ESC_PWM_CHANNEL);

    // 4. 输出最小脉宽 (安全初始状态)
    uint32_t duty = speed_to_duty(0);
    ledcWrite(ESC_PWM_CHANNEL, duty);

    // 5. 初始化状态
    currentSpeed = 0;
    escArmed = false;

    DEBUG_PRINTLN("[ESC] Initialized, outputting minimum pulse (safe state)");
}

void esc_arm() {
    // 输出最小脉宽, 等待 ESC 自检
    uint32_t duty = speed_to_duty(0);
    ledcWrite(ESC_PWM_CHANNEL, duty);

    DEBUG_PRINTLN("[ESC] Arming... sending minimum pulse");

    // 等待 ESC 响应 (通常需要 1-2 秒听到蜂鸣声)
    delay(ESC_ARM_DELAY);

    escArmed = true;
    currentSpeed = 0;

    DEBUG_PRINTLN("[ESC] Armed successfully, ready for control");
}

void esc_set_speed(uint8_t percent) {
    // 安全检查: 未校准时禁止控制
    if (!escArmed) {
        DEBUG_PRINTLN("[ESC] WARNING: Not armed, speed command ignored");
        return;
    }

    // 限制范围
    if (percent > 100) percent = 100;

    // 计算并输出占空比
    uint32_t duty = speed_to_duty(percent);
    ledcWrite(ESC_PWM_CHANNEL, duty);

    currentSpeed = percent;

    DEBUG_PRINTF("[ESC] Speed set to %d%% (duty=%u, pulse=%uus)\n",
        percent, duty, ESC_MIN_PULSE + (percent * 10));
}

void esc_stop() {
    // 立即输出最小脉宽
    uint32_t duty = speed_to_duty(0);
    ledcWrite(ESC_PWM_CHANNEL, duty);

    currentSpeed = 0;

    DEBUG_PRINTLN("[ESC] Stopped, outputting minimum pulse");
}

uint8_t esc_get_speed() {
    return currentSpeed;
}

bool esc_is_armed() {
    return escArmed;
}
