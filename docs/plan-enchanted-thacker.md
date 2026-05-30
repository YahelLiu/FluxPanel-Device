# ESP32-C3 BLDC 电机控制系统 - 项目介绍与学习指南

## 一、项目概述

这是一个基于 **ESP32-C3** 的无刷直流电机 (BLDC) 控制系统，实现了远程控制、闭环调速、多传感器监测和工业级安全保护。

### 1.1 核心功能

| 功能模块 | 描述 |
|---------|------|
| **远程控制** | WebSocket 连接后端服务器，接收控制命令 |
| **闭环调速** | PID 算法 + 霍尔传感器实现转速自动调节 |
| **多传感器** | INA219 电流电压监测 + 霍尔测速 |
| **安全保护** | 过流/过温/堵转/通信丢失四重保护 |
| **可视化** | ST7735 LCD 实时显示系统状态 |
| **配网** | WiFiManager 网页配网，无需硬编码 |

### 1.2 技术栈

```
┌─────────────────────────────────────────────┐
│  硬件: ESP32-C3-DevKitM-1 + ST7735 LCD      │
│  框架: Arduino (PlatformIO)                  │
│  RTOS: FreeRTOS                             │
│  通信: WebSocket + WiFi                     │
│  协议: JSON                                 │
└─────────────────────────────────────────────┘
```

---

## 二、项目架构

### 2.1 模块划分

```
d:\code\apps\BLDC_Monitor\esp32_motor_control\src\
├── main.cpp              # 入口：初始化、命令处理
├── config.h              # 全局配置：引脚、参数、阈值
├── motor_state.h         # 数据结构：状态、命令定义
│
├── 【通信层】
│   ├── wifi_manager.*    # WiFi 连接管理
│   └── websocket.*       # WebSocket 客户端
│
├── 【控制层】
│   ├── motor_fsm.*       # 状态机 (IDLE→STARTING→RUNNING→STOPPING→ERROR)
│   ├── esc_pwm.*         # ESC PWM 信号输出
│   ├── pid_controller.*  # PID 闭环控制
│   └── relay.*           # 继电器控制
│
├── 【感知层】
│   ├── hall_sensor.*     # 霍尔传感器测速
│   ├── ina219_sensor.*   # INA219 电流/电压监测
│   └── alarm.*           # 报警保护系统
│
├── 【显示层】
│   └── display.*         # ST7735 LCD 显示
│
└── 【RTOS 层】
    └── rtos_tasks.*      # FreeRTOS 多任务调度
```

### 2.2 RTOS 任务架构

| 任务 | 优先级 | 周期 | 职责 |
|------|--------|------|------|
| **MotorTask** | 3 (最高) | 10ms | PID控制、状态机更新、安全检查 |
| **WiFiTask** | 2 | 事件驱动 | WebSocket 通信、命令接收 |
| **SensorTask** | 1 | 100ms | 电流/电压/转速读取 |
| **DisplayTask** | 1 | 100ms | LCD 界面刷新 |

### 2.3 状态机流程

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

---

## 三、学习路径 (由浅入深)

### 第一阶段：基础入门 (1-2天)

**目标**：理解项目整体结构和基础配置

#### 学习内容

| 文件 | 学习重点 |
|------|----------|
| `config.h` | 宏定义配置方式、引脚映射、安全阈值 |
| `motor_state.h` | 结构体设计、枚举类型、状态定义 |
| `main.cpp` | 初始化流程、setup/loop 结构 |

#### 动手实践

```bash
# 1. 编译项目
cd d:\code\apps\BLDC_Monitor\esp32_motor_control
pio run

# 2. 修改 WiFi 配置 (config.h)
#define WIFI_SSID "你的WiFi名称"
#define WIFI_PASSWORD "你的WiFi密码"

# 3. 观察串口输出
pio device monitor
```

#### 面试要点

- **Q**: 为什么用宏定义而非变量存储配置？
- **A**: 宏定义在编译期替换，不占用 RAM，适合嵌入式资源受限场景

---

### 第二阶段：通信与控制 (2-3天)

**目标**：掌握 WebSocket 通信和 PWM 控制

#### 学习内容

| 文件 | 学习重点 |
|------|----------|
| `wifi_manager.*` | WiFiManager 配网流程、回调机制 |
| `websocket.*` | JSON 协议解析、命令处理回调 |
| `esc_pwm.*` | LEDC PWM 配置、ESC 校准流程 |

#### 关键代码解读

**ESC PWM 信号生成**：
```cpp
// 50Hz PWM，脉宽 1000-2000µs
// 占空比计算: duty = pulse_us × freq × (2^res - 1) / 1,000,000
// 例: 50% 转速 → 1500µs → duty ≈ 4915 (16-bit)
uint32_t duty = (pulse_us * ESC_PWM_FREQ * 65535) / 1000000;
```

**WebSocket 命令格式**：
```json
{
  "type": "command",
  "command_id": 123,
  "command": "start",
  "params": { "speed_percent": 50 }
}
```

#### 动手实践

1. 用 Postman 或 wscat 测试 WebSocket 连接
2. 发送 start/stop/speed 命令观察响应
3. 修改 PWM 参数观察电机行为变化

#### 面试要点

- **Q**: ESC 为什么要校准？
- **A**: ESC 需要学习最小/最大脉宽，确保油门响应正确

- **Q**: WebSocket 心跳机制的作用？
- **A**: 检测连接存活，触发重连，实现 Fail-safe

---

### 第三阶段：RTOS 多任务 (2-3天)

**目标**：理解 FreeRTOS 任务调度和同步机制

#### 学习内容

| 文件 | 学习重点 |
|------|----------|
| `rtos_tasks.*` | 任务创建、优先级设计、互斥锁 |

#### 关键代码解读

**精确周期控制**：
```cpp
void motorTask(void* arg) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t period = 10 / portTICK_PERIOD_MS;

    while (1) {
        // 任务处理...

        // 绝对周期延迟 (不受执行时间影响)
        vTaskDelayUntil(&lastWakeTime, period);
    }
}
```

**互斥锁保护共享数据**：
```cpp
if (xSemaphoreTake(motorStateMutex, 10)) {  // 10 ticks 超时
    motorState.actual_rpm = hall_get_rpm();  // 临界区
    xSemaphoreGive(motorStateMutex);
}
```

#### 动手实践

1. 修改任务优先级观察行为变化
2. 注释互斥锁，观察数据竞争问题
3. 调整任务周期，观察控制响应变化

#### 面试要点

- **Q**: vTaskDelay vs vTaskDelayUntil？
- **A**: 前者是相对延迟，后者是绝对周期；后者保证固定频率执行

- **Q**: 为什么 MotorTask 优先级最高？
- **A**: PID 控制需要严格时序，延迟会导致控制不稳定

---

### 第四阶段：PID 闭环控制 (2-3天)

**目标**：掌握 PID 算法实现和调参方法

#### 学习内容

| 文件 | 学习重点 |
|------|----------|
| `pid_controller.*` | 位置式 PID、积分抗饱和、在线调参 |
| `hall_sensor.*` | GPIO 中断、RPM 计算、滑动平均滤波 |

#### 关键代码解读

**PID 计算核心**：
```cpp
float pid_compute(float input) {
    float error = setpoint - input;

    // P 项
    float P = Kp * error;

    // I 项 (带抗饱和限幅)
    integral += error * dt;
    integral = constrain(integral, integralMin, integralMax);
    float I = Ki * integral;

    // D 项
    float D = Kd * (error - lastError) / dt;

    return constrain(P + I + D, outputMin, outputMax);
}
```

**RPM 计算**：
```cpp
// RPM = (脉冲数 / 极对数) × (60 / 时间秒)
float rpm = (delta_pulses * 60.0) / (polePairs * dt);
```

#### 动手实践

1. 启用 PID 模式，设置目标 RPM
2. 调整 Kp 观察响应速度变化
3. 调整 Kd 观察超调抑制效果
4. 调整 Ki 消除稳态误差

#### 面试要点

- **Q**: 什么是积分饱和？
- **A**: 误差持续存在时积分项过大，导致超调和响应迟滞

- **Q**: 调参顺序？
- **A**: 先 Kp (响应) → 再 Kd (阻尼) → 最后 Ki (稳态精度)

---

### 第五阶段：状态机与安全保护 (2-3天)

**目标**：理解状态机设计和 Fail-safe 机制

#### 学习内容

| 文件 | 学习重点 |
|------|----------|
| `motor_fsm.*` | 状态定义、事件驱动、状态转换 |
| `alarm.*` | 过流/过温/堵转检测、紧急停机 |

#### 关键代码解读

**状态机事件处理**：
```cpp
void fsm_process_event(uint8_t event) {
    switch (currentState) {
        case STATE_IDLE:
            if (event == FSM_EVENT_START) {
                currentState = STATE_STARTING;
                enter_starting();  // 执行进入动作
            }
            break;
        case STATE_RUNNING:
            if (event == FSM_EVENT_ERROR) {
                currentState = STATE_ERROR;
                enter_error();  // 紧急停机
            }
            break;
        // ...
    }
}
```

**堵转检测**：
```cpp
// 条件: PWM > 10% 但 RPM < 100，持续超过 3 秒
if (speed_percent > 10 && actual_rpm < 100) {
    if (millis() - stall_start_time > 3000) {
        alarm_trigger(ALARM_STALL);
    }
}
```

#### 动手实践

1. 断开 WebSocket 观察自动停机
2. 模拟堵转观察保护触发
3. 分析状态转换日志

#### 面试要点

- **Q**: 为什么用状态机？
- **A**: 状态明确、行为可预测、易于维护和扩展

- **Q**: Fail-safe 设计原则？
- **A**: 通信断开自动停机、默认安全状态、多层保护

---

## 四、核心技术总结

### 4.1 面试高频问题速查表

| 主题 | 问题 | 答案要点 |
|------|------|----------|
| **RTOS** | 任务间通信方式？ | 互斥锁、队列、信号量 |
| **RTOS** | 如何避免优先级反转？ | 优先级继承互斥锁 |
| **PWM** | 为什么选择 50Hz？ | 标准 ESC/舵机协议 |
| **PID** | 位置式 vs 增量式？ | 位置式直接输出值，增量式输出增量 |
| **状态机** | 三要素？ | 状态、事件、转换 |
| **安全** | 如何实现 Fail-safe？ | 心跳检测、超时停机、默认安全状态 |
| **中断** | IRAM_ATTR 作用？ | 将中断函数放入 RAM，避免 Flash 延迟 |

### 4.2 项目亮点

1. **模块化设计** - 清晰的层次划分，职责单一
2. **实时控制** - FreeRTOS 多任务 + PID 闭环
3. **安全可靠** - 四重保护机制，Fail-safe 设计
4. **可扩展** - 状态机架构易于添加新功能
5. **工业级思维** - 报警阈值、紧急停机、日志追溯

---

## 五、硬件清单

| 组件 | 型号 | 接口 |
|------|------|------|
| 主控 | ESP32-C3-DevKitM-1 | - |
| 显示 | ST7735 TFT LCD (128x160) | SPI |
| 电流传感器 | INA219 | I2C (GPIO 4/5) |
| 霍尔传感器 | 霍尔元件 | GPIO 9 (中断) |
| 电机驱动 | ESC + BLDC 电机 | GPIO 1 (PWM) |
| 继电器 | 5V 继电器模块 | GPIO 8 |

---

## 六、开发环境搭建

```bash
# 1. 安装 VS Code + PlatformIO 插件

# 2. 克隆项目
git clone <repo_url>
cd esp32_motor_control

# 3. 编译
pio run

# 4. 烧录
pio run --target upload

# 5. 串口监视
pio device monitor -b 115200
```

---

## 七、推荐学习资源

### 书籍
- 《FreeRTOS 内核实现与应用开发实战指南》
- 《嵌入式实时操作系统 μC/OS-III》
- 《PID 控制算法的 C 语言实现》

### 在线资源
- ESP32-C3 技术参考手册
- FreeRTOS 官方文档
- Arduino ESP32 API 文档

### 实践建议
1. 先读懂代码再动手修改
2. 每学完一个模块做笔记总结
3. 尝试添加新功能巩固理解
4. 模拟面试场景自问自答