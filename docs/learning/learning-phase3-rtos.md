# 阶段三：RTOS 多任务 - FreeRTOS 实战

## 学习目标

- 理解 FreeRTOS 任务调度原理
- 掌握任务优先级设计
- 理解互斥锁与同步机制
- 掌握精确周期控制

## 前置知识

- 阶段一、二
- RTOS 基本概念（任务、调度器）
- 多线程/多任务编程基础

---

## 1. FreeRTOS 基础概念

### 1.1 RTOS vs 裸机开发

| 特性 | 裸机开发 | RTOS |
|------|----------|------|
| 任务调度 | 顺序执行 | 抢占式调度 |
| 实时性 | 难保证 | 可配置优先级 |
| 资源共享 | 需手动管理 | 提供同步机制 |
| 复杂度 | 低 | 中 |

### 1.2 本项目任务设计

```cpp
void rtos_start() {
    // WiFi 任务: 优先级 2, 4KB 栈
    xTaskCreate(wifiTask, "WiFi", 4096, nullptr, 2, &wifiTaskHandle);
    
    // 电机控制任务: 优先级 3 (最高), 2KB 栈
    xTaskCreate(motorTask, "Motor", 2048, nullptr, 3, &motorTaskHandle);
    
    // 传感器任务: 优先级 1, 2KB 栈
    xTaskCreate(sensorTask, "Sensor", 2048, nullptr, 1, &sensorTaskHandle);
    
    // 显示任务: 优先级 1, 2KB 栈
    xTaskCreate(displayTask, "Display", 2048, nullptr, 1, &displayTaskHandle);
}
```

---

## 2. 任务优先级设计

### 2.1 优先级分配原则

| 任务 | 优先级 | 周期 | 设计理由 |
|------|--------|------|----------|
| **MotorTask** | **3 (最高)** | 10ms | PID 控制需要确定性时序，延迟会导致转速波动 |
| WiFiTask | 2 | 事件驱动 | 通信需及时响应，但可容忍短暂延迟 |
| SensorTask | 1 | 100ms | 传感器采样不需要微秒级精度 |
| DisplayTask | 1 | 100ms | 人眼无法察觉 100ms 延迟 |

**设计原则**：
```
实时性要求 ↑  → 优先级 ↑
执行时间 ↓    → 优先级 ↑
容忍延迟 ↓    → 优先级 ↑
```

### 2.2 为什么 MotorTask 优先级最高？

PID 控制需要严格的时序：

```cpp
// PID 计算依赖固定的时间间隔 dt
float dt = (now - pid.lastTime) / 1000.0;  // 期望 dt = 0.01s

// 如果 dt 波动：
// - 积分项不稳定：相同误差累积效果不一致
// - 微分项噪声放大：dt 变小 → 微分项放大 → 输出抖动
```

---

## 3. 精确周期控制

### 3.1 vTaskDelay vs vTaskDelayUntil

```cpp
// ❌ vTaskDelay: 相对延迟，会累积误差
void task_v1() {
    while (1) {
        do_work();           // 执行时间 Te
        vTaskDelay(10);      // 实际周期 = Te + 10ms (漂移!)
    }
}

// ✅ vTaskDelayUntil: 绝对周期，无漂移
void task_v2() {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t period = 10 / portTICK_PERIOD_MS;
    
    while (1) {
        do_work();
        vTaskDelayUntil(&lastWakeTime, period);  // 精确 10ms
    }
}
```

### 3.2 时序对比图

```
vTaskDelay(10):
    ████████░░░░░░░░░░░░░░░░░░████████░░░░░░░░░░░░░░░░░░████████
    ├─执行─┤──休眠10ms──┤         ├─执行─┤──休眠10ms──┤
    实际周期 = 执行时间 + 10ms (累积漂移)

vTaskDelayUntil():
    ████████░░░░░░░░░░░░░░░░░░████████░░░░░░░░░░░░░░████████
    ├执行├────精确补齐────┤     ├执行├────精确补齐────┤
    实际周期 = 恒定 10ms (无漂移)
```

### 3.3 本项目实现

```cpp
void motorTask(void* arg) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t period = 10 / portTICK_PERIOD_MS;  // 10ms
    
    while (1) {
        fsm_update();          // 状态机更新
        safety_check_rtos();   // 安全检查
        alarm_check();         // 报警检查
        // PID 控制...
        
        vTaskDelayUntil(&lastWakeTime, period);  // 精确周期
    }
}
```

---

## 4. 互斥锁与同步

### 4.1 为什么需要互斥锁？

多任务并发访问共享数据会导致数据竞争：

```
问题场景：
MotorTask:  motorState.pid_enabled = true;
            ↓ (被抢占)
SensorTask: motorState.actual_rpm = 1000;
            ↓ (被抢占)
MotorTask:  float rpm = motorState.actual_rpm;  // 读到不一致数据！
```

### 4.2 互斥锁使用

```cpp
// 创建互斥锁
SemaphoreHandle_t motorStateMutex = xSemaphoreCreateMutex();

// 使用模式
if (xSemaphoreTake(motorStateMutex, 10)) {  // 最多等待 10 ticks
    // 临界区：访问共享数据
    motorState.actual_rpm = hall_get_rpm();
    
    xSemaphoreGive(motorStateMutex);  // 释放锁
}
```

### 4.3 优先级继承

FreeRTOS 互斥锁支持优先级继承，避免优先级反转：

```
低优先级任务持有锁
    ↓
高优先级任务等待锁
    ↓
低优先级任务临时提升优先级（继承高优先级）
    ↓
低优先级任务快速完成并释放锁
    ↓
高优先级任务获得锁并执行
```

---

## 5. 任务间通信机制对比

| 机制 | 适用场景 | 特点 |
|------|----------|------|
| **互斥锁** | 保护共享资源 | 二值，支持优先级继承 |
| **队列** | 数据传递 | 可传递任意数据，有缓冲 |
| **信号量** | 事件通知 | 可计数，适用于资源计数 |
| **事件组** | 多事件同步 | 可等待多个事件 |

**本项目选择**：
- 共享数据保护 → 互斥锁
- 无需任务间数据传递（使用全局状态）

---

## 为什么这么写？（关键问答）

**Q1: 为什么 MotorTask 优先级最高？**
- A: PID 控制需要严格时序，延迟会导致控制不稳定

**Q2: 为什么用 vTaskDelayUntil 而不是 vTaskDelay？**
- A: vTaskDelayUntil 保证精确周期，避免累积漂移，对 PID 控制至关重要

**Q3: 为什么需要互斥锁？**
- A: 多任务并发访问共享数据会导致数据竞争，互斥锁保护临界区

**Q4: 为什么 WiFi 任务栈更大（4KB）？**
- A: 网络通信需要更多栈空间（SSL、JSON 解析等）

---

## 动手实践

1. **修改优先级**：将 MotorTask 优先级改为 1，观察控制稳定性变化
2. **注释互斥锁**：注释掉 `xSemaphoreTake`，观察数据竞争现象
3. **调整周期**：将 MotorTask 周期改为 50ms，观察 PID 响应变化

---

## 面试要点

**Q: FreeRTOS 任务状态有哪些？**
- A: Running（运行）、Ready（就绪）、Blocked（阻塞）、Suspended（挂起）

**Q: 如何避免优先级反转？**
- A: 使用支持优先级继承的互斥锁（Mutex），或使用优先级天花板协议

**Q: vTaskDelayUntil 如何保证精确周期？**
- A: 记录上次唤醒时间，计算到下一个周期的绝对时间，不受执行时间影响

---

**下一阶段**：[阶段四：PID 闭环控制](learning-phase4-pid.md)
