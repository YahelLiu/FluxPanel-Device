# 阶段二：通信与控制 - WebSocket 与 PWM

## 学习目标

- 理解 WiFiManager 配网原理
- 掌握 WebSocket 通信与 JSON 协议
- 理解 ESC PWM 信号原理
- 掌握回调机制设计

## 前置知识

- 阶段一：基础入门
- 网络通信基础概念
- PWM 信号基本原理

---

## 1. WiFiManager 配网原理

### 1.1 双模式设计

```cpp
// config.h
#define WIFI_USE_CONFIG true   // 选择模式

#if WIFI_USE_CONFIG
    // 硬编码模式：直接连接
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#else
    // WiFiManager 模式：网页配网
    wm.autoConnect(WIFI_AP_NAME, WIFI_AP_PASSWORD);
#endif
```

### 1.2 WiFiManager 配网流程

```
ESP32 启动
    ↓
检查保存的 WiFi 配置
    ├─ 有 → 尝试连接 → 成功 → 正常模式
    │              ↓ 失败
    └─ 无 → 开启 AP 热点 (192.168.4.1)
              ↓
         手机连接热点 → 浏览器访问 192.168.4.1
              ↓
         选择 WiFi + 输入密码
              ↓
         ESP32 保存配置 → 连接 → 正常模式
```

**为什么需要配网模式？**
- 产品化部署时，用户无法修改代码
- WiFi 密码不应硬编码在固件中
- 支持更换 WiFi 环境

---

## 2. WebSocket 通信

### 2.1 为什么选择 WebSocket？

| 特性 | HTTP 轮询 | MQTT | WebSocket |
|------|----------|------|-----------|
| 实时性 | 差（秒级） | 好（毫秒级） | 好（毫秒级） |
| 双向通信 | 否 | 是 | 是 |
| 连接开销 | 高 | 低 | 低 |
| 服务器资源 | 需 Web 服务器 | 需 Broker | 可直接对接后端 |

**本项目选择原因**：
1. 实时控制需求（毫秒级响应）
2. 双向通信（命令下发 + 状态上报）
3. 无需额外 Broker
4. JSON 原生支持

### 2.2 JSON 协议设计

**命令消息（服务器 → ESP32）**：
```json
{
  "type": "command",
  "command_id": 12345,
  "command": "start",
  "params": {
    "speed_percent": 50,
    "direction": 1
  }
}
```

**状态上报（ESP32 → 服务器）**：
```json
{
  "type": "status",
  "device_id": "ESP32_Yves",
  "data": {
    "is_running": true,
    "speed_percent": 50,
    "current": 1.25,
    "voltage": 12.0
  }
}
```

**命令确认**：
```json
{
  "type": "command_ack",
  "command_id": 12345,
  "success": true,
  "message": "Motor started at 50%"
}
```

### 2.3 回调机制

```cpp
// websocket.h - 回调类型定义
typedef void (*CommandCallback)(MotorCommand& cmd);

// 注册回调
void ws_set_command_callback(CommandCallback callback) {
    commandCallback = callback;
}

// 收到消息时调用
static void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    case WStype_TEXT: {
        MotorCommand cmd;
        // 解析 JSON...
        
        if (commandCallback) {
            commandCallback(cmd);  // 调用 main.cpp 中的处理函数
        }
    }
}
```

**回调机制的优势**：
- **解耦**：通信层不感知业务逻辑
- **可扩展**：可注册多个回调
- **测试友好**：可模拟回调进行测试

### 2.4 心跳机制

```cpp
#define HEARTBEAT_INTERVAL 30000  // 30秒

void ws_loop() {
    webSocket.loop();
    
    // 发送心跳
    if (wsConnected && (millis() - lastHeartbeat >= HEARTBEAT_INTERVAL)) {
        lastHeartbeat = millis();
        ws_send_heartbeat();  // {"type":"ping","device_id":"ESP32_Yves"}
    }
}
```

**心跳作用**：
1. 保活：防止 NAT 映射超时
2. 延迟检测：监控网络质量
3. 状态同步：服务器确认设备在线

---

## 3. ESC PWM 原理

### 3.1 为什么是 50Hz？

ESC（电子调速器）起源于航模领域，采用与舵机相同的 PWM 标准：

- **频率**：50Hz（周期 20ms）
- **脉宽范围**：1000µs ~ 2000µs
- **映射关系**：1000µs = 停止，2000µs = 全速

```
         ←─────── 20ms ───────→
         ┌────┐
         │    │
    ─────┘    └─────────────────
         ←1ms→
         1000µs = 停止
         
         ┌─────────┐
         │         │
    ─────┘         └────────────
         ←──2ms──→
         2000µs = 全速
```

### 3.2 脉宽计算公式

```cpp
static uint32_t speed_to_duty(uint8_t percent) {
    // 步骤 1: 计算脉宽 (线性映射)
    // 0% → 1000µs, 100% → 2000µs
    uint32_t pulse_us = ESC_MIN_PULSE + (percent * (ESC_MAX_PULSE - ESC_MIN_PULSE) / 100);
    //           = 1000 + percent * 10
    
    // 步骤 2: 计算占空比 (LEDC 格式)
    // 公式: duty = (pulse_us × freq × 2^res) / 1,000,000
    uint32_t duty = (pulse_us * ESC_PWM_FREQ * ((1 << ESC_PWM_RES) - 1)) / 1000000;
    //           = pulse_us * 50 * 65535 / 1000000
    //           ≈ pulse_us * 3.27675
    
    return duty;
}
```

**数值示例**：

| 转速 % | 脉宽 (µs) | 占空比 (16-bit) |
|--------|----------|-----------------|
| 0% | 1000 | 3277 |
| 50% | 1500 | 4915 |
| 100% | 2000 | 6554 |

### 3.3 ESC 校准流程

```cpp
void esc_arm() {
    // 1. 输出最小脉宽
    uint32_t duty = speed_to_duty(0);  // 1000µs
    ledcWrite(ESC_PWM_CHANNEL, duty);
    
    // 2. 等待 ESC 自检 (蜂鸣声)
    delay(ESC_ARM_DELAY);  // 2000ms
    
    escArmed = true;
}
```

**校准的重要性**：
- ESC 需要学习最小/最大脉宽
- 确保油门响应正确
- 防止上电瞬间电机意外启动

**安全设计**：
```cpp
void esc_set_speed(uint8_t percent) {
    if (!escArmed) {
        DEBUG_PRINTLN("[ESC] Not armed, ignored");
        return;  // 未校准禁止控制
    }
    // ...
}
```

---

## 为什么这么写？（关键问答）

**Q1: 为什么用 WebSocket 而不是 HTTP？**
- A: 实时双向通信需求，HTTP 轮询延迟高且浪费资源

**Q2: 为什么用回调机制？**
- A: 解耦通信层与业务层，通信层只需调用回调，不关心具体处理

**Q3: 为什么 ESC 需要校准？**
- A: ESC 需要学习脉宽范围，确保油门映射正确，防止失控

**Q4: 为什么 PWM 频率是 50Hz？**
- A: 标准 ESC/舵机协议，大多数 ESC 固件依赖这个频率

---

## 动手实践

1. **测试 WebSocket**：用 wscat 或 Postman 连接服务器，发送 start/stop 命令
2. **观察 PWM**：用示波器或逻辑分析仪测量 GPIO 1 的 PWM 信号
3. **修改心跳**：将 `HEARTBEAT_INTERVAL` 改为 10 秒，观察网络行为

---

## 面试要点

**Q: WebSocket 与 HTTP 长轮询的区别？**
- A: WebSocket 是全双工长连接，服务端可主动推送；HTTP 长轮询是半双工，每次需客户端发起

**Q: PWM 占空比如何计算？**
- A: `duty = (pulse_us × freq × 2^resolution) / 1,000,000`

---

**下一阶段**：[阶段三：RTOS 多任务](learning-phase3-rtos.md)
