/**
 * @file ina219_sensor.cpp
 * @brief INA219 Current/Voltage/Power Sensor Implementation
 */

#include "ina219_sensor.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_INA219.h>

// ============================================================================
// 内部状态
// ============================================================================
static Adafruit_INA219 ina219;
static bool sensorReady = false;

// 传感器数据
static float sensorCurrent = 0;     // 电流 (A)
static float sensorVoltage = 0;     // 电压 (V)
static float sensorPower = 0;       // 功率 (W)

// ============================================================================
// 公共函数
// ============================================================================

bool ina219_init() {
    // 初始化 I2C
    Wire.begin(INA219_SDA, INA219_SCL);

    // 尝试初始化 INA219
    if (!ina219.begin()) {
        DEBUG_PRINTLN("[INA219] Init failed! Check wiring.");
        sensorReady = false;
        return false;
    }

    // 设置校准: 32V, 2A 量程
    // 如果需要更大量程可改为 setCalibration_32V_2A()
    ina219.setCalibration_32V_2A();

    sensorReady = true;
    DEBUG_PRINTLN("[INA219] Initialized successfully");

    // 首次读取
    ina219_read();

    return true;
}

void ina219_read() {
    if (!sensorReady) return;

    // 读取电流 (mA → A)
    float ma = ina219.getCurrent_mA();
    sensorCurrent = ma / 1000.0;

    // 读取电压
    sensorVoltage = ina219.getBusVoltage_V();

    // 读取功率 (mW → W)
    float mw = ina219.getPower_mW();
    sensorPower = mw / 1000.0;

    DEBUG_PRINTF("[INA219] V=%.2fV, I=%.3fA, P=%.2fW\n",
        sensorVoltage, sensorCurrent, sensorPower);
}

float ina219_get_current() {
    return sensorCurrent;
}

float ina219_get_voltage() {
    return sensorVoltage;
}

float ina219_get_power() {
    return sensorPower;
}

bool ina219_is_ready() {
    return sensorReady;
}
