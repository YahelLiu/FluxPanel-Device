/**
 * @file display.cpp
 * @brief ST7735 TFT LCD Display Module Implementation
 */

#include "display.h"
#include "config.h"

#if USE_LCD

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// ST7735 显示屏对象
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// 颜色定义
#define COLOR_BG       ST77XX_BLACK
#define COLOR_TITLE    ST77XX_CYAN
#define COLOR_TEXT     ST77XX_WHITE
#define COLOR_SUCCESS  ST77XX_GREEN
#define COLOR_ERROR    ST77XX_RED
#define COLOR_WARNING  ST77XX_YELLOW
#define COLOR_GRAY     0x7BEF  // 灰色

// 度数符号 (Adafruit GFX 字体中的位置 0xF8)
#define DEGREE_SYMBOL  (char)0xF8

// 界面布局 (128x160 像素)
#define Y_TITLE        5
#define Y_WIFI         25
#define Y_WS           40
#define Y_MOTOR        55
#define Y_SENSOR       105
#define Y_NETWORK      135
#define Y_LOG          150

// ============================================================================
// 初始化
// ============================================================================

void display_init() {
    // 初始化 SPI
    SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

#ifdef TFT_BL
    // 初始化背光 (PWM 50% 亮度, 节省电流)
    pinMode(TFT_BL, OUTPUT);
    ledcSetup(0, 5000, 8);           // 通道0, 5kHz, 8位分辨率
    ledcAttachPin(TFT_BL, 0);
    ledcWrite(0, 128);               // 50% 亮度 (0-255)
#endif

    // 初始化 ST7735
    // INITR_BLACKTAB 用于 1.8" TFT (128x160)
    tft.initR(INITR_BLACKTAB);

    // 清屏
    tft.fillScreen(COLOR_BG);

    // 设置竖屏方向
    tft.setRotation(0);

    // 显示启动信息
    tft.setTextColor(COLOR_TITLE);
    tft.setTextSize(1);
    tft.setCursor(5, Y_TITLE);
    tft.print("ESP32 Motor Control");

    // 分隔线
    tft.drawFastHLine(0, Y_TITLE + 12, 128, COLOR_TEXT);

    DEBUG_PRINTLN("[Display] ST7735 initialized");
}

void display_clear() {
    tft.fillScreen(COLOR_BG);
}

// ============================================================================
// WiFi 状态显示
// ============================================================================

void display_wifi_status(bool connected, String ip) {
    // 清除区域
    tft.fillRect(0, Y_WIFI, 128, 12, COLOR_BG);

    tft.setCursor(5, Y_WIFI);

    if (connected) {
        tft.setTextColor(COLOR_SUCCESS);
        tft.print("WiFi: Connected");
    } else {
        tft.setTextColor(COLOR_ERROR);
        tft.print("WiFi: Disconnected");
    }

    // 显示 IP 地址
    if (connected && ip.length() > 0) {
        tft.fillRect(0, Y_WIFI + 12, 128, 10, COLOR_BG);
        tft.setTextColor(COLOR_GRAY);
        tft.setTextSize(0);  // 小字体
        tft.setCursor(5, Y_WIFI + 12);
        // 截断 IP 显示
        if (ip.length() > 16) {
            tft.print(ip.substring(0, 16));
        } else {
            tft.print(ip);
        }
        tft.setTextSize(1);
    }
}

// ============================================================================
// WebSocket 状态显示
// ============================================================================

void display_ws_status(bool connected) {
    // 清除区域
    tft.fillRect(0, Y_WS, 128, 10, COLOR_BG);

    tft.setCursor(5, Y_WS);

    if (connected) {
        tft.setTextColor(COLOR_SUCCESS);
        tft.print("WS: Connected");
    } else {
        tft.setTextColor(COLOR_ERROR);
        tft.print("WS: Disconnected");
    }
}

// ============================================================================
// 电机状态显示
// ============================================================================

void display_motor_state(MotorState& state) {
    int y = Y_MOTOR;

    // 清除区域
    tft.fillRect(0, y, 128, 45, COLOR_BG);

    // 标题
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(5, y);
    tft.print("Motor Status:");
    y += 12;

    // 运行状态
    tft.setCursor(5, y);
    tft.print("Run: ");
    tft.setTextColor(state.is_running ? COLOR_SUCCESS : COLOR_GRAY);
    tft.print(state.is_running ? "ON" : "OFF");
    y += 10;

    // 转速
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(5, y);
    tft.print("Speed: ");
    tft.print(state.speed_percent);
    tft.print("%");
    y += 10;

    // 方向
    tft.setCursor(5, y);
    tft.print("Dir: ");
    tft.print(state.direction == 1 ? "FWD" : "REV");
}

// ============================================================================
// 设备状态显示
// ============================================================================

void display_device_status(DeviceStatus& status) {
    display_wifi_status(status.wifi_connected, status.ip_address);
    display_ws_status(status.ws_connected);
}

// ============================================================================
// 命令显示
// ============================================================================

void display_command(String command, uint32_t command_id) {
    // 在传感器区域显示命令
    int y = Y_SENSOR;

    tft.fillRect(0, y, 128, 10, COLOR_BG);
    tft.setTextColor(COLOR_WARNING);
    tft.setCursor(5, y);
    tft.print("Cmd: ");
    tft.print(command);
    tft.print(" #");
    tft.print(command_id);
}

// ============================================================================
// 日志显示
// ============================================================================

void display_log(String message) {
    static int logY = Y_LOG;
    static int logCount = 0;

    // 每 3 条清空一次
    if (logCount >= 3) {
        logCount = 0;
        tft.fillRect(0, Y_LOG - 5, 128, 15, COLOR_BG);
        logY = Y_LOG;
    }

    tft.setTextColor(COLOR_GRAY);
    tft.setTextSize(0);  // 小字体
    tft.setCursor(5, logY);

    // 截断过长的消息
    if (message.length() > 20) {
        tft.print(message.substring(0, 20));
    } else {
        tft.print(message);
    }

    tft.setTextSize(1);
    logY += 5;
    logCount++;
}

// ============================================================================
// 完整界面更新
// ============================================================================

void display_update_all(DeviceStatus& status, MotorState& motorState) {
    display_device_status(status);
    display_motor_state(motorState);
}

// ============================================================================
// 错误显示
// ============================================================================

void display_error(String error) {
    // 清除下半部分
    tft.fillRect(0, Y_MOTOR, 128, 100, COLOR_BG);

    tft.setTextColor(COLOR_ERROR);
    tft.setCursor(5, Y_MOTOR + 10);
    tft.print("ERROR:");

    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(5, Y_MOTOR + 25);

    // 分行显示错误信息
    int maxLen = 16;
    for (int i = 0; i < error.length(); i += maxLen) {
        tft.setCursor(5, tft.getCursorY() + 10);
        String line = error.substring(i, min(i + maxLen, (int)error.length()));
        tft.print(line);
    }
}

// ============================================================================
// LCD 指定行内容显示
// ============================================================================

void display_lcd_line(int line, String content, bool clear) {
    // 限制行号范围
    if (line < 0 || line > 3) line = 0;

    // 在传感器区域下方显示, 每行10像素
    int y = Y_SENSOR + line * 10;

    // 清屏: 清除传感器区域以下所有内容
    if (clear) {
        tft.fillRect(0, Y_SENSOR, 128, 55, COLOR_BG);
    } else {
        // 只清除当前行
        tft.fillRect(0, y, 128, 10, COLOR_BG);
    }

    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(1);
    tft.setCursor(5, y);

    // 过滤非 ASCII 字符, 替换度数符号 (ST7735 默认字体不支持)
    String filtered = "";
    for (unsigned int i = 0; i < content.length() && i < 20; i++) {
        char c = content.charAt(i);
        if (c >= 0x20 && c <= 0x7E) {
            filtered += c;
        }
        // 跳过 UTF-8 度数符号 (0xC2 0xB0)
    }

    tft.print(filtered);
}

// ============================================================================
// 系统状态显示模式 (WS 已连接)
// ============================================================================

void display_system_status(SystemStatus& status) {
    // 清屏
    tft.fillScreen(COLOR_BG);

    int y = 5;

    // 标题
    tft.setTextColor(COLOR_TITLE);
    tft.setTextSize(1);
    tft.setCursor(5, y);
    tft.print("Yves-PC Status");
    y += 12;

    // 分隔线
    tft.drawFastHLine(0, y, 128, COLOR_TEXT);
    y += 5;

    // CPU 状态
    tft.setTextColor(COLOR_SUCCESS);
    tft.setCursor(5, y);
    tft.print("CPU:");
    tft.setTextColor(COLOR_TEXT);
    tft.print(status.cpu_load, 0);
    tft.print("% ");
    tft.print(status.cpu_temp, 0);
    tft.print(DEGREE_SYMBOL);
    tft.print("C ");
    tft.print(status.cpu_power, 0);
    tft.print("W");
    y += 12;

    // GPU 状态
    tft.setTextColor(COLOR_WARNING);
    tft.setCursor(5, y);
    tft.print("GPU:");
    tft.setTextColor(COLOR_TEXT);
    tft.print(status.gpu_load, 0);
    tft.print("% ");
    tft.print(status.gpu_temp, 0);
    tft.print(DEGREE_SYMBOL);
    tft.print("C ");
    tft.print(status.gpu_power, 0);
    tft.print("W");
    y += 12;

    // MEM 状态
    tft.setTextColor(0x07FF);  // 青色
    tft.setCursor(5, y);
    tft.print("MEM:");
    tft.setTextColor(COLOR_TEXT);
    tft.print(status.mem_load);
    tft.print("%");
    y += 15;

    // 分隔线
    tft.drawFastHLine(0, y, 128, COLOR_GRAY);
    y += 5;

    // 连接状态
    tft.setTextColor(COLOR_SUCCESS);
    tft.setTextSize(0);
    tft.setCursor(5, y);
    tft.print("WS: Connected");
}

// ============================================================================
// 设置模式显示 (WS 未连接)
// ============================================================================

void display_setup_mode(DeviceStatus& status) {
    // 清屏
    tft.fillScreen(COLOR_BG);

    int y = 5;

    // 标题
    tft.setTextColor(COLOR_TITLE);
    tft.setTextSize(1);
    tft.setCursor(5, y);
    tft.print("ESP32 Motor Control");
    y += 14;

    // 分隔线
    tft.drawFastHLine(0, y, 128, COLOR_TEXT);
    y += 5;

    // WiFi 状态
    tft.setCursor(5, y);
    if (status.wifi_connected) {
        tft.setTextColor(COLOR_SUCCESS);
        tft.print("WiFi: Connected");
    } else {
        tft.setTextColor(COLOR_ERROR);
        tft.print("WiFi: Disconnected");
    }
    y += 10;

    // IP 地址
    if (status.wifi_connected && status.ip_address.length() > 0) {
        tft.setTextColor(COLOR_GRAY);
        tft.setTextSize(0);  // 小字体
        tft.setCursor(5, y);
        if (status.ip_address.length() > 18) {
            tft.print(status.ip_address.substring(0, 18));
        } else {
            tft.print(status.ip_address);
        }
        tft.setTextSize(1);
        y += 10;
    }

    // WS 状态
    tft.setCursor(5, y);
    if (status.ws_connected) {
        tft.setTextColor(COLOR_SUCCESS);
        tft.print("WS: Connected");
    } else {
        tft.setTextColor(COLOR_ERROR);
        tft.print("WS: Disconnected");
    }
    y += 12;

    // 分隔线
    tft.drawFastHLine(0, y, 128, COLOR_GRAY);
}

#else // USE_LCD = false

// ============================================================================
// LCD 禁用时的空实现
// ============================================================================

void display_init() {}
void display_clear() {}
void display_wifi_status(bool connected, String ip) {}
void display_ws_status(bool connected) {}
void display_motor_state(MotorState& state) {}
void display_device_status(DeviceStatus& status) {}
void display_command(String command, uint32_t command_id) {}
void display_log(String message) {}
void display_update_all(DeviceStatus& status, MotorState& motorState) {}
void display_error(String error) {}
void display_lcd_line(int line, String content, bool clear) {}
void display_system_status(SystemStatus& status) {}
void display_setup_mode(DeviceStatus& status) {}

#endif // USE_LCD
