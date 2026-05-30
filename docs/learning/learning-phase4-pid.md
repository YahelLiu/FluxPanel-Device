# 阶段四：PID 闭环控制 - 算法与实现

## 学习目标

- 理解 PID 算法原理与各参数物理意义
- 掌握位置式 PID 的代码实现
- 理解积分抗饱和的原因与实现
- 掌握霍尔传感器测速原理

## 前置知识

- 阶段一、二、三
- 控制系统基础概念
- 中断处理基本原理

---

## 1. PID 算法原理

### 1.1 标准公式

$$u(t) = K_p e(t) + K_i \int_0^t e(\tau) d\tau + K_d \frac{de(t)}{dt}$$

其中：
- $e(t)$ = 设定值 - 实际值（误差）
- $K_p$ = 比例系数
- $K_i$ = 积分系数
- $K_d$ = 微分系数

### 1.2 各项物理意义

#### 比例项（P）- "响应强度"

```cpp
float P = Kp * error;
```

| Kp 值 | 效果 | 问题 |
|-------|------|------|
| 太小 | 响应迟缓 | 无法快速到达目标 |
| 适中 | 快速响应 | 平衡稳定性 |
| 太大 | 快速震荡 | 超调严重，可能失控 |

**物理意义**：类似弹簧刚度，误差越大，修正力越大。

#### 积分项（I）- "历史补偿"

```cpp
integral += error * dt;
float I = Ki * integral;
```

| Ki 值 | 效果 | 问题 |
|-------|------|------|
| 太小 | 稳态误差消除慢 | 无法精确到达目标 |
| 适中 | 消除稳态误差 | 平衡响应速度 |
| 太大 | 快速消除误差 | 超调、震荡、积分饱和 |

**物理意义**：类似"记忆"，记住历史累积误差，补偿系统偏差。

#### 微分项（D）- "趋势预测"

```cpp
float D = Kd * (error - lastError) / dt;
```

| Kd 值 | 效果 | 问题 |
|-------|------|------|
| 太小 | 抑制震荡弱 | 容易超调 |
| 适中 | 平滑响应 | 抑制超调 |
| 太大 | 过度阻尼 | 对噪声敏感，响应迟缓 |

**物理意义**：类似"阻尼器"，预测误差变化趋势，提前刹车。

---

## 2. 位置式 PID 实现

### 2.1 核心代码

```cpp
float pid_compute(float input) {
    // 时间增量
    unsigned long now = millis();
    float dt = (now - pid.lastTime) / 1000.0;
    pid.lastTime = now;
    
    // 误差计算
    float error = pid.setpoint - input;
    
    // P 项
    float P = pid.Kp * error;
    
    // I 项（带抗饱和）
    pid.integral += error * dt;
    
    // 积分限幅（抗饱和）
    float integralMax = (pid.outputMax - P) / pid.Ki;
    float integralMin = (pid.outputMin - P) / pid.Ki;
    pid.integral = constrain(pid.integral, integralMin, integralMax);
    
    float I = pid.Ki * pid.integral;
    
    // D 项
    float D = pid.Kd * (error - pid.lastError) / dt;
    pid.lastError = error;
    
    // 输出
    pid.output = constrain(P + I + D, pid.outputMin, pid.outputMax);
    
    return pid.output;
}
```

### 2.2 代码与公式对应

| 公式项 | 代码 | 说明 |
|--------|------|------|
| $K_p e(t)$ | `Kp * error` | 比例响应 |
| $K_i \int e dt$ | `Ki * integral` | 积分累积 |
| $K_d \frac{de}{dt}$ | `Kd * (error - lastError) / dt` | 微分差分 |

---

## 3. 积分抗饱和

### 3.1 为什么需要抗饱和？

**问题场景**：电机堵转

```
目标 RPM = 3000，实际 RPM = 0
误差 = 3000，持续累积...

积分项: I = Ki * Σ(error * dt)
        → 积分值不断增大 → 输出达到 100% 饱和
        → 即使堵转解除，积分值仍然很大
        → 电机突然狂飙，失控！
```

### 3.2 抗饱和实现

```cpp
// 计算积分限幅边界
// 当 P + I 可能超过输出上限时，限制积分值
float integralMax = (pid.outputMax - P) / pid.Ki;
float integralMin = (pid.outputMin - P) / pid.Ki;

pid.integral = constrain(pid.integral, integralMin, integralMax);
```

**数学推导**：
```
输出: output = P + I + D

当 output = outputMax 时:
    P + Ki * integral + D = outputMax
    integral = (outputMax - P - D) / Ki

简化版（忽略 D）:
    integralMax = (outputMax - P) / Ki
```

### 3.3 效果对比

```
无抗饱和:
RPM  │    目标 3000
     │   ╱╲╱╲╱╲╱╲╱╲────
     │  ╱                  ╲
     │ ╱                    ╲超调失控
     └─────────────────────────── 时间

有抗饱和:
RPM  │         目标 3000
     │        ────────────
     │       ╱
     │      ╱
     │────╱
     └─────────────────────────── 时间
```

---

## 4. 霍尔传感器测速

### 4.1 中断服务函数

```cpp
// volatile 关键字：每次必须从内存读取
volatile uint32_t pulseCount = 0;

// IRAM_ATTR：将函数放入 RAM，避免 Flash 延迟
void IRAM_ATTR hallISR() {
    pulseCount++;
}
```

**关键技术点**：

| 技术 | 作用 |
|------|------|
| `volatile` | 防止编译器优化，确保每次从内存读取 |
| `IRAM_ATTR` | 将中断函数放入 RAM，避免 Flash 访问延迟 |

### 4.2 RPM 计算公式

```cpp
float hall_get_rpm() {
    uint32_t pulseDelta = currentPulses - lastPulseCount;
    float timeDelta = (currentTime - lastReadTime) / 1000.0;
    
    // RPM = (脉冲数 / 极对数) * (60 / 时间)
    float rpm = (pulseDelta * 60.0) / (polePairs * timeDelta);
    
    return rpm;
}
```

**公式推导**：
```
BLDC 电机特性：
- 极对数 = N（例如 7 对极）
- 每转一圈产生 N 个霍尔脉冲

计算转速：
- 时间 Δt 内检测到 Δp 个脉冲
- 转过的圈数 = Δp / N
- 转速 (圈/秒) = (Δp / N) / Δt
- 转速 (圈/分钟) = (Δp / N) * (60 / Δt)
- RPM = (Δp * 60) / (N * Δt)
```

### 4.3 滑动平均滤波

```cpp
#define RPM_FILTER_SIZE 5
static float rpmFilter[RPM_FILTER_SIZE] = {0};

float hall_get_rpm() {
    // ... 计算 rpm ...
    
    // 滑动平均滤波
    rpmFilter[rpmFilterIndex] = rpm;
    rpmFilterIndex = (rpmFilterIndex + 1) % RPM_FILTER_SIZE;
    
    float filteredRpm = 0;
    for (int i = 0; i < RPM_FILTER_SIZE; i++) {
        filteredRpm += rpmFilter[i];
    }
    filteredRpm /= RPM_FILTER_SIZE;
    
    return filteredRpm;
}
```

**滤波效果**：窗口越大 → 越平滑 → 响应越慢

---

## 5. 调参实践

### 5.1 Ziegler-Nichols 方法（简化版）

```
Step 1: 关闭积分和微分 (Ki=0, Kd=0)
        逐步增大 Kp 直到系统开始持续震荡
        记录临界增益 Kc 和震荡周期 Tc

Step 2: 根据规则设置参数
        Kp = 0.6 * Kc
        Ki = 2 * Kp / Tc
        Kd = Kp * Tc / 8

Step 3: 微调优化
        如果超调大 → 增大 Kd，减小 Kp
        如果响应慢 → 增大 Kp
        如果稳态误差大 → 增大 Ki
```

### 5.2 调参顺序

1. **先调 Kp**：获得基本响应速度
2. **再调 Kd**：抑制超调和震荡
3. **最后调 Ki**：消除稳态误差

---

## 为什么这么写？（关键问答）

**Q1: 为什么需要积分项？**
- A: 比例控制存在稳态误差，积分项通过累积历史误差消除

**Q2: 为什么需要抗饱和？**
- A: 误差持续存在时积分项会过大，导致超调和响应迟滞

**Q3: 为什么用 volatile 修饰脉冲计数器？**
- A: 变量在中断中修改，主循环读取，volatile 确保每次从内存读取最新值

**Q4: 为什么用 IRAM_ATTR？**
- A: 中断函数需要快速执行，放入 RAM 避免 Flash 访问延迟

---

## 动手实践

1. **调整 Kp**：将 Kp 从 1.0 改为 2.0，观察响应速度和超调变化
2. **调整 Kd**：增加 Kd，观察超调抑制效果
3. **模拟堵转**：用手阻止电机，观察积分饱和现象和抗饱和效果

---

## 面试要点

**Q: 位置式 PID 与增量式 PID 的区别？**
- A: 位置式直接输出控制量，增量式输出控制增量；增量式实现简单，不易积分饱和

**Q: PID 调参的一般顺序？**
- A: 先 Kp（响应）→ 再 Kd（阻尼）→ 最后 Ki（稳态精度）

**Q: 为什么霍尔测速需要滤波？**
- A: 机械振动和电气噪声会导致 RPM 波动，滤波平滑输出

---

**下一阶段**：[阶段五：状态机与安全保护](learning-phase5-safety.md)
