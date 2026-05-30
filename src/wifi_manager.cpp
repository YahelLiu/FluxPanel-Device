/**
 * @file wifi_manager.cpp
 * @brief WiFi Manager Module Implementation
 *
 * 支持两种模式:
 * 1. WiFiManager 网页配网 (WIFI_USE_CONFIG=false)
 * 2. 硬编码 WiFi (WIFI_USE_CONFIG=true)
 */

#include "wifi_manager.h"
#include "config.h"

#include <WiFi.h>

#if WIFI_USE_CONFIG
// 硬编码模式 - 不需要 WiFiManager
#else
#include <WiFiManager.h>
static WiFiManager wm;
#endif

// 连接状态
static bool isConnected = false;

// 回调函数
static void (*configCallback)() = nullptr;
static void (*connectedCallback)(String) = nullptr;

// ============================================================================
// WiFiManager 模式 (WIFI_USE_CONFIG=false)
// ============================================================================
#if !WIFI_USE_CONFIG

// 进入配网模式回调
static void enterConfigModeCallback(WiFiManager *myWiFiManager) {
    DEBUG_PRINTLN("[WiFiManager] Entered config mode");
    DEBUG_PRINTF("[WiFiManager] AP: %s\n", myWiFiManager->getConfigPortalSSID().c_str());
    DEBUG_PRINTF("[WiFiManager] IP: %s\n", WiFi.softAPIP().toString().c_str());

    if (configCallback) {
        configCallback();
    }
}

// 保存配置回调
static void saveConfigCallback() {
    DEBUG_PRINTLN("[WiFiManager] Config saved!");
}

bool wifi_manager_init() {
    DEBUG_PRINTLN("[WiFi] WiFiManager mode");

    // 设置回调
    wm.setAPCallback(enterConfigModeCallback);
    wm.setSaveConfigCallback(saveConfigCallback);

    // 设置配网超时 (秒)
    wm.setConfigPortalTimeout(WIFI_CONFIG_TIMEOUT);

    // 设置最小信号质量
    wm.setMinimumSignalQuality(20);

    // 设置标题 (网页顶部显示)
    wm.setTitle("FluxPanel Device");

    // 尝试自动连接
    bool res = wm.autoConnect(WIFI_AP_NAME, WIFI_AP_PASSWORD);

    if (res) {
        isConnected = true;
        String ip = WiFi.localIP().toString();
        DEBUG_PRINTF("[WiFi] Connected! IP: %s\n", ip.c_str());

        if (connectedCallback) {
            connectedCallback(ip);
        }
        return true;
    } else {
        isConnected = false;
        DEBUG_PRINTLN("[WiFi] Failed to connect, in AP mode");
        return false;
    }
}

void wifi_manager_reset() {
    DEBUG_PRINTLN("[WiFi] Resetting WiFi config...");
    wm.resetSettings();
    isConnected = false;
}

// ============================================================================
// 硬编码模式 (WIFI_USE_CONFIG=true)
// ============================================================================
#else

bool wifi_manager_init() {
    DEBUG_PRINTF("[WiFi] Hardcoded mode, connecting to %s...\n", WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startAttemptTime = millis();

    // 等待连接
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startAttemptTime < WIFI_CONNECT_TIMEOUT) {
        delay(500);
        DEBUG_PRINT(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        isConnected = true;
        String ip = WiFi.localIP().toString();
        DEBUG_PRINTF("\n[WiFi] Connected! IP: %s\n", ip.c_str());

        if (connectedCallback) {
            connectedCallback(ip);
        }
        return true;
    } else {
        isConnected = false;
        DEBUG_PRINTLN("\n[WiFi] Connection failed!");
        return false;
    }
}

void wifi_manager_reset() {
    DEBUG_PRINTLN("[WiFi] Reconnecting...");
    WiFi.disconnect();
    WiFi.reconnect();
}

#endif

// ============================================================================
// 公共函数 (两种模式共用)
// ============================================================================

void wifi_manager_loop() {
    // 检查连接状态变化
    static bool lastConnected = false;
    bool nowConnected = (WiFi.status() == WL_CONNECTED);

    if (nowConnected != lastConnected) {
        lastConnected = nowConnected;
        isConnected = nowConnected;

        if (nowConnected) {
            String ip = WiFi.localIP().toString();
            DEBUG_PRINTF("[WiFi] Reconnected! IP: %s\n", ip.c_str());
            if (connectedCallback) {
                connectedCallback(ip);
            }
        } else {
            DEBUG_PRINTLN("[WiFi] Connection lost!");
        }
    }

#if WIFI_USE_CONFIG
    // 硬编码模式: 尝试自动重连
    if (!isConnected) {
        static unsigned long lastReconnectTime = 0;
        if (millis() - lastReconnectTime > 10000) {  // 每 10 秒重试
            lastReconnectTime = millis();
            DEBUG_PRINTLN("[WiFi] Reconnecting...");
            WiFi.reconnect();
        }
    }
#endif
}

bool wifi_manager_is_connected() {
    return isConnected && (WiFi.status() == WL_CONNECTED);
}

String wifi_manager_get_ip() {
    if (isConnected) {
        return WiFi.localIP().toString();
    }
    return "";
}

void wifi_manager_set_config_callback(void (*callback)()) {
    configCallback = callback;
}

void wifi_manager_set_connected_callback(void (*callback)(String)) {
    connectedCallback = callback;
}
