# 阶段一：基础入门 - 项目架构与配置

## 学习目标

- 理解 ESP32-C3 BLDC 电机控制项目的整体架构
- 掌握嵌入式 C++ 的配置管理模式
- 理解数据结构设计原则
- 掌握初始化流程设计

## 前置知识

- C/C++ 基础语法
- Arduino 框架基本概念（setup/loop）
- 嵌入式系统基本概念

---

## 1. 项目架构总览

### 1.1 模块划分

```
src/
├── config.h          ← 全局配置（编译时常量）
├── motor_state.h     ← 数据结构定义（运行时状态）
├── main.cpp          ← 程序入口（初始化 + 主循环）
│
├── 【通信层】
│   ├── wifi_manager.*   # WiFi 连接管理
│   └── websocket.*      # WebSocket 客户端
│
├── 【控制层】
│   ├── motor_fsm.*      # 状态机
│   ├── esc_pwm.*        # ESC PWM 控制
│   ├── pid_controller.* # PID 闭环控制
│   └── relay.*          # 继电器控制
│
├── 【感知层】
│   ├── hall_sensor.*    # 霍尔传感器测速
│   ├── ina219_sensor.*  # 电流/电压监测
│   └── alarm.*          # 报警保护系统
│
└── 【显示层】
    └── display.*        # ST7735 LCD 显示
```

### 为什么这样分层？

| 层次 | 职责 | 设计原则 |
|------|------|----------|
| 通信层 | 网络连接、协议处理 | 单向依赖，不感知业务逻辑 |
| 控制层 | 电机控制决策 | 核心逻辑，高优先级 |
| 感知层 | 数据采集、安全检测 | 被动响应，提供数据 |
| 显示层 | 人机交互 | 低优先级，不影响控制 |

**关键设计思想**：高层依赖低层，低层不依赖高层。这保证了模块的独立性和可测试性。

---

## 2. config.h 深度解读

### 2.1 宏定义配置模式

```cpp
// 为什么用宏定义而不是变量？
#define ESC_PWM_FREQ 50        // PWM 频率
#define ESC_MIN_PULSE 1000     // 最小脉宽
```

**对比分析**：

| 方案 | 存储位置 | 访问速度 | 可修改性 |
|------|----------|----------|----------|
| `#define` | Flash（编译时常量） | 最快（直接嵌入指令） | 需重新编译 |
| `const` | RAM（运行时常量） | 较快（内存访问） | 可运行时修改 |
| `变量` | RAM | 慢（需地址解析） | 随时修改 |

**嵌入式选择原则**：
- **不变配置** → 用 `#define`（节省 RAM）
- **可调参数** → 用变量（运行时可改）

### 2.2 WiFi 双模式设计

```cpp
// 选择模式
#define WIFI_USE_CONFIG true   // true=硬编码, false=WiFiManager配网

// 硬编码模式配置
#define WIFI_SSID "Xiaomi_4237"
#define WIFI_PASSWORD "1593572486"

// WiFiManager 模式配置
#define WIFI_AP_NAME "ESP32_MotorSetup"
#define WIFI_AP_PASSWORD ""
```

**两种模式的适用场景**：

| 模式 | 适用场景 | 优点 | 缺点 |
|------|----------|------|------|
| 硬编码 | 开发调试、固定环境 | 简单快速 | 密码暴露、不灵活 |
| WiFiManager | 产品化部署 | 用户友好、安全 | 首次需配网 |

### 2.3 安全阈值设计

```cpp
#define SAFETY_TIMEOUT 10000              // 通信超时停机 (ms)
#define ALARM_OVERCURRENT_THRESHOLD 5.0f  // 过流阈值 (A)
#define ALARM_OVERTEMP_THRESHOLD 80.0f    // 过温阈值 (°C)
#define ALARM_STALL_TIMEOUT 3000          // 堵转超时 (ms)
```

**Fail-safe 设计原则**：
1. 任何异常都应导向安全状态（停机）
2. 阈值应留有安全裕度
3. 超时时间应短于危险发生时间

---

## 3. motor_state.h 数据结构设计

### 3.1 状态机枚举

```cpp
enum MotorStateEnum {
    STATE_IDLE = 0,       // 空闲
    STATE_STARTING = 1,   // 启动中
    STATE_RUNNING = 2,    // 运行中
    STATE_STOPPING = 3,   // 停止中
    STATE_ERROR = 4       // 错误
};
```

**为什么需要 STARTING 和 STOPPING 中间状态？**

1. ESC 解锁需要时间（约 2 秒）
2. 电机有惯性，停止不是瞬间的
3. 防止命令冲突（启动中不能再启动）
4. 便于错误定位（知道在哪个阶段出错）

### 3.2 核心状态结构体

```cpp
struct MotorState {
    MotorStateEnum state = STATE_IDLE;  // 状态机状态
    bool is_running = false;            // 运行标志
    int speed_percent = 0;              // 转速百分比
    int direction = 1;                  // 方向 (1/-1)
    float temperature = 0.0;            // 温度
    float current = 0.0;                // 电流
    float voltage = 0.0;                // 电压
    
    // PID 闭环控制
    bool pid_enabled = false;
    float target_rpm = 0;
    float actual_rpm = 0;
    
    void reset() { /* 重置所有字段 */ }
};
```

**嵌入式数据结构设计原则**：

| 原则 | 应用 |
|------|------|
| 避免动态内存 | 无 `new/delete`，无 `String`（命令结构体除外） |
| 明确初始化 | 所有字段有默认值 |
| 内存对齐 | `float` 4 字节，`bool` 1 字节（编译器自动填充） |

**direction 为什么用 1/-1 而不是枚举？**

```cpp
// 可以直接用于数学计算
float effective_speed = base_speed * motorState.direction;  // 正转或反转
```

---

## 4. main.cpp 入口分析

### 4.1 初始化顺序

```cpp
void setup() {
    // 1. 串口（调试输出）
    Serial.begin(DEBUG_BAUD);
    
    // 2. 显示屏（可视反馈）
    display_init();
    
    // 3. 继电器（电源控制）
    relay_init();
    
    // 4. ESC PWM（必须在继电器后！）
    esc_init();
    
    // 5. 传感器
    hall_init();
    ina219_init();
    
    // 6. 报警系统
    alarm_init();
    
    // 7. 状态机
    fsm_init();
    
    // 8. WiFi
    wifi_manager_init();
    
    // 9. WebSocket
    ws_init();
    
    // 10. PID
    pid_init(PID_KP, PID_KI, PID_KD);
    
    // 11. FreeRTOS（最后启动）
    rtos_init();
    rtos_start();
}
```

**为什么是这个顺序？**

```
硬件基础层 → 外设驱动层 → 逻辑控制层 → 网络通信层 → 实时系统层
```

关键依赖：
- ESC 必须在继电器后初始化（继电器控制电机电源）
- 状态机必须在硬件驱动后初始化
- FreeRTOS 必须最后启动（启动后进入任务调度）

### 4.2 安全机制

```cpp
void emergency_stop(const char* reason) {
    DEBUG_PRINTF("[SAFETY] Emergency stop: %s\n", reason);
    fsm_process_event(FSM_EVENT_ERROR);  // 触发状态机停机
    alarm_trigger(ALARM_COMM_LOST);       // 触发报警
}

void safety_check() {
    if (!motorState.is_running) return;
    
    // 检查 1: WebSocket 断开
    if (!deviceStatus.ws_connected) {
        emergency_stop("WebSocket disconnected");
        return;
    }
    
    // 检查 2: 通信超时
    if (millis() - lastCommandTime > SAFETY_TIMEOUT) {
        emergency_stop("Command timeout");
        return;
    }
}
```

**Fail-safe 设计**：通信断开 = 自动停机，确保失控时系统安全。

---

## 为什么这么写？（关键问答）

**Q1: 为什么用宏定义而不是 const 变量？**
- A: 宏定义在编译期替换，不占用 RAM，适合嵌入式资源受限场景

**Q2: 为什么初始化顺序很重要？**
- A: 模块间有依赖关系，错误的顺序会导致硬件操作失败或状态不一致

**Q3: 为什么用状态机管理电机状态？**
- A: 状态明确、行为可预测、易于维护和扩展，避免非法状态转换

**Q4: 为什么需要 emergency_stop 函数？**
- A: 统一的紧急停机入口，确保任何异常都能安全停机，避免代码重复

---

## 动手实践

1. **修改配置**：在 `config.h` 中修改 `DEVICE_ID`，观察串口输出变化
2. **调整安全阈值**：将 `SAFETY_TIMEOUT` 改为 5000ms，观察超时停机行为
3. **追踪初始化**：在 `setup()` 各函数间添加 `DEBUG_PRINTLN`，观察初始化顺序

---

## 面试要点

**Q: 嵌入式系统中如何管理配置参数？**
- A: 不变配置用宏定义（编译期），可调参数用变量（运行时），敏感信息用 NVS 存储

**Q: 如何设计初始化顺序？**
- A: 按依赖关系分层：硬件基础 → 外设驱动 → 业务逻辑 → 网络通信 → RTOS

---

**下一阶段**：[阶段二：通信与控制](learning-phase2-communication.md)
