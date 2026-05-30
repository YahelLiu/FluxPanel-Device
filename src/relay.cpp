/**
 * @file relay.cpp
 * @brief Relay Control Module Implementation
 */

#include "relay.h"
#include "config.h"

// 继电器当前状态
static bool relayState = false;

// ============================================================================
// 初始化
// ============================================================================

void relay_init() {
    pinMode(RELAY_PIN, OUTPUT);

    // 初始化为关闭状态
    if (RELAY_ACTIVE_LOW) {
        digitalWrite(RELAY_PIN, HIGH);  // 低电平触发, 初始高电平=关闭
    } else {
        digitalWrite(RELAY_PIN, LOW);   // 高电平触发, 初始低电平=关闭
    }

    relayState = false;

    DEBUG_PRINTLN("[Relay] Initialized, state: OFF");
}

// ============================================================================
// 控制函数
// ============================================================================

void relay_on() {
    if (RELAY_ACTIVE_LOW) {
        digitalWrite(RELAY_PIN, LOW);   // 低电平触发=开启
    } else {
        digitalWrite(RELAY_PIN, HIGH);  // 高电平触发=开启
    }

    relayState = true;
    DEBUG_PRINTLN("[Relay] ON");
}

void relay_off() {
    if (RELAY_ACTIVE_LOW) {
        digitalWrite(RELAY_PIN, HIGH);  // 低电平触发, 高电平=关闭
    } else {
        digitalWrite(RELAY_PIN, LOW);   // 高电平触发, 低电平=关闭
    }

    relayState = false;
    DEBUG_PRINTLN("[Relay] OFF");
}

void relay_set(bool state) {
    if (state) {
        relay_on();
    } else {
        relay_off();
    }
}

void relay_toggle() {
    if (relayState) {
        relay_off();
    } else {
        relay_on();
    }
}

// ============================================================================
// 状态查询
// ============================================================================

bool relay_get_state() {
    return relayState;
}

String relay_get_state_string() {
    return relayState ? "on" : "off";
}
