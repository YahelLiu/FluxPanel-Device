/**
 * @file hall_sensor.cpp
 * @brief Hall Effect Sensor Implementation
 *
 * 使用 GPIO 中断计数霍尔传感器脉冲
 * 通过脉冲频率计算电机转速
 */

#include "hall_sensor.h"
#include "config.h"

// ============================================================================
// 内部状态
// ============================================================================

// 脉冲计数器 (volatile 因为在中断中修改)
volatile uint32_t pulseCount = 0;

// 极对数 (用于计算 RPM)
static uint8_t polePairs = HALL_POLE_PAIRS;

// 上次读取的状态
static uint32_t lastPulseCount = 0;
static unsigned long lastReadTime = 0;

// RPM 滤波 (滑动平均)
#define RPM_FILTER_SIZE 5
static float rpmFilter[RPM_FILTER_SIZE] = {0};
static uint8_t rpmFilterIndex = 0;

// ============================================================================
// 中断服务函数
// ============================================================================

/**
 * @brief 霍尔传感器中断服务函数
 *
 * 必须使用 IRAM_ATTR 放入 IRAM，避免 Flash 访问延迟
 */
void IRAM_ATTR hallISR() {
    pulseCount++;
}

// ============================================================================
// 公共函数
// ============================================================================

void hall_init() {
    // 配置 GPIO 为输入，启用内部上拉
    pinMode(HALL_PIN, INPUT_PULLUP);

    // 绑定中断服务函数 (上升沿触发)
    attachInterrupt(digitalPinToInterrupt(HALL_PIN), hallISR, RISING);

    // 初始化状态
    pulseCount = 0;
    lastPulseCount = 0;
    lastReadTime = millis();

    DEBUG_PRINTF("[Hall] Initialized on GPIO%d, pole pairs=%d\n", HALL_PIN, polePairs);
}

void hall_reset() {
    pulseCount = 0;
    lastPulseCount = 0;
    lastReadTime = millis();

    // 清空滤波器
    for (int i = 0; i < RPM_FILTER_SIZE; i++) {
        rpmFilter[i] = 0;
    }

    DEBUG_PRINTLN("[Hall] Reset");
}

uint32_t hall_get_pulses() {
    return pulseCount;
}

float hall_get_rpm() {
    // 获取当前时间和脉冲数
    uint32_t currentPulses = pulseCount;
    unsigned long currentTime = millis();

    // 计算时间差和脉冲差
    uint32_t pulseDelta = currentPulses - lastPulseCount;
    float timeDelta = (currentTime - lastReadTime) / 1000.0;  // 秒

    // 更新上次状态
    lastPulseCount = currentPulses;
    lastReadTime = currentTime;

    // 防止除零
    if (timeDelta <= 0.001) {
        return 0;
    }

    // 计算 RPM: (脉冲数 / 极对数) * (60 / 时间)
    // 每转一圈产生 polePairs 个脉冲
    float rpm = (pulseDelta * 60.0) / (polePairs * timeDelta);

    // 滑动平均滤波
    rpmFilter[rpmFilterIndex] = rpm;
    rpmFilterIndex = (rpmFilterIndex + 1) % RPM_FILTER_SIZE;

    float filteredRpm = 0;
    for (int i = 0; i < RPM_FILTER_SIZE; i++) {
        filteredRpm += rpmFilter[i];
    }
    filteredRpm /= RPM_FILTER_SIZE;

    return filteredRpm;
}

void hall_set_pole_pairs(uint8_t pairs) {
    if (pairs > 0) {
        polePairs = pairs;
        DEBUG_PRINTF("[Hall] Pole pairs set to %d\n", pairs);
    }
}

uint8_t hall_get_pole_pairs() {
    return polePairs;
}
