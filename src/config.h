/**
 * @file config.h
 * @brief FluxPanel Device Configuration
 *
 * 修改此文件中的配置参数以匹配你的实际环境
 */

#pragma once

// ============================================================================
// WiFi 配置 (两种方式可选)
// ============================================================================
// 方式1: WiFiManager 网页配网 (推荐)
//   - 首次开机 ESP32 开启热点，手机连上后访问 192.168.4.1 配网
//   - 配置会自动保存，下次开机自动连接
//   - WIFI_USE_CONFIG 设为 false 或留空时使用此方式
//
// 方式2: 硬编码 WiFi (备用)
//   - WiFi 名称密码写死在代码里
//   - WIFI_USE_CONFIG 设为 true 时使用此方式

// 选择使用哪种方式 (true=硬编码, false=WiFiManager配网)
#define WIFI_USE_CONFIG true

// 硬编码 WiFi 配置 (当 WIFI_USE_CONFIG=true 时使用)
#define WIFI_SSID "Xiaomi_4237"           // WiFi 名称
#define WIFI_PASSWORD "1593572486"           // WiFi 密码
#define WIFI_CONNECT_TIMEOUT 30000           // WiFi 连接超时 (毫秒)

// WiFiManager 配网热点配置 (当 WIFI_USE_CONFIG=false 时使用)
#define WIFI_AP_NAME "ESP32_MotorSetup"      // 配网热点名称
#define WIFI_AP_PASSWORD ""                  // 配网热点密码 (空=无密码)
#define WIFI_CONFIG_TIMEOUT 180              // 配网超时 (秒)

// ============================================================================
// WebSocket 服务器配置
// ============================================================================
#define WS_SERVER "139.196.84.99"            // 后端服务器 IP 地址
#define WS_PORT 8080                         // 后端服务器端口
#define WS_PATH "/ws/device"                 // WebSocket 路径
#define WS_RECONNECT_INTERVAL 5000           // 重连间隔 (毫秒)

// ============================================================================
// 设备配置
// ============================================================================
#define DEVICE_ID "ESP32_Yves"               // 设备唯一标识
#define DEVICE_TOKEN "token"                 // 设备认证 Token

// ============================================================================
// 状态上报配置
// ============================================================================
#define STATUS_INTERVAL 1000                 // 状态上报间隔 (毫秒)

// ============================================================================
// ST7735 TFT LCD 引脚配置 (SPI)
// ============================================================================
#define USE_LCD true                        // 启用/禁用 LCD 显示

#define TFT_CS    7                          // 片选
#define TFT_RST   10                         // 复位
#define TFT_DC    6                          // 数据/命令
#define TFT_MOSI  3                          // SPI MOSI (SDA)
#define TFT_SCLK  2                          // SPI 时钟 (SCK)
// #define TFT_BL    8                        // 背光 (PWM 调光, 降低功耗)

// ============================================================================
// 继电器引脚配置
// ============================================================================
#define RELAY_PIN 8                          // 继电器控制引脚
#define RELAY_ACTIVE_LOW true                // 继电器触发电平 (true=低电平触发)

// ============================================================================
// 预留: INA219 电流传感器引脚 (I2C)
// ============================================================================
#define INA219_SDA 4                         // I2C 数据
#define INA219_SCL 5                         // I2C 时钟

// ============================================================================
// 预留: 霍尔传感器引脚
// ============================================================================
#define HALL_PIN 9                           // 霍尔传感器中断引脚

// ============================================================================
// ESC PWM 配置
// ============================================================================
#define ESC_PWM_PIN 1                        // ESC 信号线 GPIO
#define ESC_PWM_CHANNEL 1                    // LEDC 通道 (0 被 LCD 背光预留)
#define ESC_PWM_FREQ 50                      // PWM 频率 (Hz) - 标准 ESC 信号
#define ESC_PWM_RES 16                       // 分辨率 (bits)
#define ESC_ARM_DELAY 2000                   // ESC 校准等待时间 (ms)
#define ESC_MIN_PULSE 1000                   // 最小脉宽 (us) - 停止
#define ESC_MAX_PULSE 2000                   // 最大脉宽 (us) - 全速

// ============================================================================
// 安全配置 (Fail-safe)
// ============================================================================
#define SAFETY_TIMEOUT 10000                 // 通信超时停机 (ms)

// ============================================================================
// PID 控制器默认参数
// ============================================================================
#define PID_KP  1.0f                         // 比例系数
#define PID_KI  0.1f                         // 积分系数
#define PID_KD  0.01f                        // 微分系数

// ============================================================================
// 霍尔传感器参数
// ============================================================================
#define HALL_POLE_PAIRS 7                    // 电机极对数

// ============================================================================
// 报警保护阈值
// ============================================================================
#define ALARM_OVERCURRENT_THRESHOLD 5.0f     // 过流阈值 (A)
#define ALARM_OVERTEMP_THRESHOLD 80.0f       // 过温阈值 (°C)
#define ALARM_STALL_TIMEOUT 3000             // 堵转超时 (ms)

// ============================================================================
// MQTT 配置 (IoT 数据上云)
// ============================================================================
#define MQTT_ENABLED true                    // 启用/禁用 MQTT 功能

// MQTT Broker 选择 (取消注释对应行)
#define MQTT_BROKER_EMQX                     // EMQX 公共测试服务器 (免费)
// #define MQTT_BROKER_ALIYUN                // 阿里云 IoT (需要三元组)
// #define MQTT_BROKER_CUSTOM               // 自定义 Broker

// EMQX 公共 Broker (测试用, 免费无需注册)
#ifdef MQTT_BROKER_EMQX
    #define MQTT_SERVER    "broker.emqx.io"
    #define MQTT_PORT      1883
    #define MQTT_USER      ""                // 无需认证
    #define MQTT_PASSWORD  ""
#endif

// 阿里云 IoT Platform (生产环境)
#ifdef MQTT_BROKER_ALIYUN
    // 替换为你的阿里云 IoT 三元组
    #define MQTT_SERVER    "*.iot-as-mqtt.cn-shanghai.aliyuncs.com"
    #define MQTT_PORT      1883
    #define MQTT_PRODUCT_KEY    "your_product_key"
    #define MQTT_DEVICE_NAME    "your_device_name"
    #define MQTT_DEVICE_SECRET  "your_device_secret"
#endif

// 自定义 Broker
#ifdef MQTT_BROKER_CUSTOM
    #define MQTT_SERVER    "your_broker_ip"
    #define MQTT_PORT      1883
    #define MQTT_USER      "username"
    #define MQTT_PASSWORD  "password"
#endif

// MQTT 主题配置
#define MQTT_TOPIC_DATA       "fluxpanel/device/data"      // 数据上报主题
#define MQTT_TOPIC_STATUS     "fluxpanel/device/status"    // 在线状态主题
#define MQTT_TOPIC_COMMAND    "fluxpanel/device/cmd"       // 命令下发主题
#define MQTT_TOPIC_ALARM      "fluxpanel/device/alarm"     // 告警主题

// MQTT 参数
#define MQTT_KEEPALIVE        60                           // 心跳间隔 (秒)
#define MQTT_QOS              1                            // QoS 级别 (0, 1, 2)
#define MQTT_RECONNECT_MS     5000                         // 重连间隔 (毫秒)
#define MQTT_PUBLISH_MS       5000                         // 发布间隔 (毫秒)

// ============================================================================
// 调试配置
// ============================================================================
#define DEBUG_SERIAL Serial
#define DEBUG_BAUD 115200

// 调试打印宏
#ifdef DEBUG
    #define DEBUG_PRINT(x) DEBUG_SERIAL.print(x)
    #define DEBUG_PRINTLN(x) DEBUG_SERIAL.println(x)
    #define DEBUG_PRINTF(...) DEBUG_SERIAL.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif
