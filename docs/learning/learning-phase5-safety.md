# 阶段五：状态机与安全保护 - Fail-safe 设计

## 学习目标

- 理解状态机三要素与设计方法
- 掌握 Fail-safe 设计原则
- 理解多维度保护机制
- 掌握报警系统实现

## 前置知识

- 阶段一、二、三、四
- 状态机基本概念
- 嵌入式安全设计思想

---

## 1. 状态机设计

### 1.1 状态机三要素

```
┌─────────────────────────────────────────┐
│          状态机三要素                     │
├─────────────────────────────────────────┤
│  1. 状态（State）     - 系统所处的模式    │
│  2. 事件（Event）     - 触发状态变化的信号│
│  3. 转换（Transition）- 状态变化的规则   │
└─────────────────────────────────────────┘
```

### 1.2 状态定义

```cpp
enum MotorStateEnum {
    STATE_IDLE = 0,       // 空闲
    STATE_STARTING = 1,   // 启动中
    STATE_RUNNING = 2,    // 运行中
    STATE_STOPPING = 3,   // 停止中
    STATE_ERROR = 4       // 错误
};
```

### 1.3 状态转换图

```
                    start
    ┌─────────┐ ──────────→ ┌────────────┐
    │  IDLE   │             │  STARTING  │
    └─────────┘             └────────────┘
         ▲                        │ armed
         │                        ▼
         │                  ┌────────────┐
         │                  │  RUNNING   │ ──stop──┐
         │                  └────────────┘         │
         │                        │ error          │
         │                        ▼                │
         │                  ┌────────────┐        │
         └──────────────────│   ERROR    │        │
                   reset    └────────────┘        │
                                                   │
         ┌─────────────────────────────────────────┘
         │  stop
         ▼
    ┌────────────┐
    │  STOPPING  │ ──auto──→ IDLE
    └────────────┘
```

### 1.4 状态机核心实现

```cpp
void fsm_process_event(uint8_t event) {
    MotorStateEnum prevState = currentState;
    
    switch (currentState) {
        case STATE_IDLE:
            if (event == FSM_EVENT_START) {
                currentState = STATE_STARTING;
                enter_starting();  // 进入动作
            }
            break;
            
        case STATE_STARTING:
            if (event == FSM_EVENT_ARMED) {
                currentState = STATE_RUNNING;
                enter_running();
            }
            else if (event == FSM_EVENT_ERROR) {
                currentState = STATE_ERROR;
                enter_error();
            }
            break;
            
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
            
        // ... 其他状态 ...
    }
    
    motorState.state = currentState;
}
```

### 1.5 进入动作

```cpp
// 进入 STARTING 状态
static void enter_starting() {
    esc_arm();      // ESC 校准
    relay_on();     // 接通电源
    motorState.is_running = true;
}

// 进入 ERROR 状态
static void enter_error() {
    esc_stop();     // 紧急停机
    relay_off();    // 断开电源
    motorState.is_running = false;
    pid_reset();    // 重置 PID
}
```

**设计思想**：每个状态有明确的"进入动作"，确保状态转换时执行必要操作。

---

## 2. Fail-safe 设计原则

### 2.1 核心原则

```
┌──────────────────────────────────────────────────────┐
│              Fail-safe 设计原则                        │
├──────────────────────────────────────────────────────┤
│ 1. 默认安全状态: 上电/复位后系统处于安全状态            │
│ 2. 故障导向安全: 检测到故障时执行安全动作              │
│ 3. 独立检测: 安全检查不依赖其他模块                    │
│ 4. 冗余保护: 多层次检测，单一故障不会导致整体失效       │
│ 5. 快速响应: 故障检测和响应时间足够短                  │
└──────────────────────────────────────────────────────┘
```

### 2.2 多层次保护架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      安全防护层次                                │
├─────────────────────────────────────────────────────────────────┤
│  Level 1: 硬件层                                                │
│    - 继电器可控电源                                              │
│    - GPIO 默认低电平                                            │
│                                                                 │
│  Level 2: 通信层                                                 │
│    - WebSocket 断开 → 紧急停机                                  │
│    - 命令超时 10s → 紧急停机                                     │
│    - WiFi 断开事件 → 紧急停机                                    │
│                                                                 │
│  Level 3: 状态机层                                               │
│    - 未校准禁止控制                                              │
│    - 状态转换有明确条件                                          │
│                                                                 │
│  Level 4: 保护层                                                 │
│    - 过流保护 (>5A)                                              │
│    - 过温保护 (>80°C)                                            │
│    - 堵转保护 (PWM>10% 但 RPM<100)                               │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3. 通信故障保护

### 3.1 安全检查实现

```cpp
void safety_check_rtos() {
    if (!motorState.is_running) return;  // 只在运行时检查
    
    bool shouldStop = false;
    const char* reason = "";
    
    // 检查 1: WebSocket 断开
    if (!deviceStatus.ws_connected) {
        shouldStop = true;
        reason = "WS disconnected";
        alarm_trigger(ALARM_COMM_LOST);
    }
    // 检查 2: 通信超时
    else if (millis() - lastCommandTime > SAFETY_TIMEOUT) {
        shouldStop = true;
        reason = "Command timeout";
        alarm_trigger(ALARM_COMM_LOST);
    }
    
    if (shouldStop) {
        DEBUG_PRINTF("[SAFETY] Emergency stop: %s\n", reason);
        fsm_process_event(FSM_EVENT_ERROR);  // 触发紧急停机
    }
}
```

### 3.2 WiFi 事件监听

```cpp
// main.cpp - WiFi 断开事件处理
WiFi.onEvent([](WiFiEvent_t event) {
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        fsm_process_event(FSM_EVENT_ERROR);
        alarm_trigger(ALARM_COMM_LOST);
    }
});
```

**设计要点**：底层网络事件直接触发保护，不依赖上层逻辑。

---

## 4. 报警系统

### 4.1 报警类型定义

```cpp
enum AlarmType {
    ALARM_NONE = 0,       // 无报警
    ALARM_OVERCURRENT,    // 过流
    ALARM_OVERTEMP,       // 过温
    ALARM_STALL,          // 堵转
    ALARM_COMM_LOST       // 通信丢失
};
```

### 4.2 多维度检测

```cpp
void alarm_check() {
    if (!motorState.is_running) return;
    
    // 检查 1: 过流
    if (motorState.current > alarmState.overcurrent_threshold) {
        alarm_trigger(ALARM_OVERCURRENT);
        return;
    }
    
    // 检查 2: 过温
    if (motorState.temperature > alarmState.overtemp_threshold) {
        alarm_trigger(ALARM_OVERTEMP);
        return;
    }
    
    // 检查 3: 堵转 (PWM > 10% 但 RPM 极低)
    if (motorState.speed_percent > 10 && motorState.actual_rpm < 100) {
        if (!alarmState.stall_detected) {
            alarmState.stall_detected = true;
            alarmState.stall_start_time = millis();
        } else {
            if (millis() - alarmState.stall_start_time > alarmState.stall_timeout_ms) {
                alarm_trigger(ALARM_STALL);
            }
        }
    } else {
        alarmState.stall_detected = false;
    }
}
```

### 4.3 堵转检测的智能设计

```cpp
// 不是立即触发，而是持续一段时间后才判定堵转
if (motorState.speed_percent > 10 && motorState.actual_rpm < 100) {
    if (!alarmState.stall_detected) {
        // 首次检测到堵转，开始计时
        alarmState.stall_detected = true;
        alarmState.stall_start_time = millis();
    } else {
        // 持续堵转，检查是否超时
        if (millis() - alarmState.stall_start_time > 3000) {
            alarm_trigger(ALARM_STALL);
        }
    }
}
```

**设计思想**：避免瞬时波动误触发，同时确保真实故障能被检测。

### 4.4 报警触发

```cpp
void alarm_trigger(AlarmType type) {
    alarmState.active_alarm = type;
    alarmState.alarm_time = millis();
    
    // 紧急停机序列
    esc_stop();      // 1. 停止 PWM
    relay_off();     // 2. 断开电源
    
    // 状态重置
    motorState.is_running = false;
    motorState.speed_percent = 0;
    pid_reset();
}
```

---

## 为什么这么写？（关键问答）

**Q1: 为什么用状态机管理电机状态？**
- A: 状态明确、行为可预测、易于维护和扩展，避免非法状态转换

**Q2: 为什么需要 STARTING 和 STOPPING 中间状态？**
- A: ESC 解锁需要时间，电机有惯性，中间状态防止命令冲突

**Q3: 为什么通信断开要立即停机？**
- A: 远程控制系统的安全底线，失控时必须导向安全状态

**Q4: 为什么堵转检测有延时？**
- A: 避免瞬时波动误触发，同时确保真实故障能被检测

---

## 动手实践

1. **模拟通信断开**：断开 WebSocket 连接，观察自动停机
2. **模拟堵转**：用手阻止电机，观察堵转检测触发
3. **调整阈值**：修改过流阈值，观察报警触发行为

---

## 面试要点

**Q: Fail-safe 设计的核心原则？**
- A: 默认安全状态、故障导向安全、独立检测、冗余保护、快速响应

**Q: 状态机三要素是什么？**
- A: 状态（State）、事件（Event）、转换（Transition）

**Q: 如何实现多维度保护？**
- A: 分层检测（硬件层→通信层→状态机层→保护层），每层独立检测，触发统一停机流程

---

**恭喜完成全部学习！** 🎉

建议回顾 [阶段一](learning-phase1-basics.md) 巩固基础，或尝试添加新功能（如温度传感器、速度曲线控制等）。
