# FluxPanel Device - ESP32-C3

FluxPanel 系统的 ESP32-C3 设备端，实现 PC 状态远程监控与硬件控制。

## 🎯 功能特性

### 已实现 ✅

| 功能 | 状态 | 说明 |
|------|------|------|
| WiFi 连接 | ✅ | 自动连接、断线重连、WiFiManager 网页配网 |
| WebSocket 通信 | ✅ | 实时双向通信，与 FluxPanel 后端无缝对接 |
| LCD 显示 | ✅ | ST7735S TFT LCD 实时显示系统状态 |
| 继电器控制 | ✅ | 远程控制继电器开关（急停/安全开关） |
| 状态上报 | ✅ | 定时上报设备状态到后端 |
| PC 状态监控 | ✅ | 显示 CPU/GPU/MEM 实时状态 |
| ESC PWM 控制 | ✅ | 50Hz PWM 信号控制 BLDC 电机转速，支持校准 |
| PID 闭环控制 | ✅ | 位置式 PID 算法，积分抗饱和，精确控速 |
| 霍尔测速 | ✅ | 中断方式测量电机转速，RPM 实时计算 |
| 电机状态机 | ✅ | 五状态 FSM（IDLE/STARTING/RUNNING/STOPPING/ERROR） |
| 报警保护 | ✅ | 过流/过温/堵转检测，Fail-safe 多层保护 |
| FreeRTOS 多任务 | ✅ | 四任务调度，互斥锁同步，精确周期控制 |
| INA219 电流传感器 | ✅ | I2C 接口，实时电流/电压/功率监测 |

### 待开发 🚧

| 功能 | 状态 | 说明 |
|------|------|------|
| 温度传感器 | 📋 | DS18B20 电机温度监测（硬件待接入） |

---

## 📚 深度学习文档

本项目提供完整的分阶段学习文档，帮助理解嵌入式开发的"为什么"：

### 学习阶段

| 阶段 | 文档 | 核心内容 |
|------|------|----------|
| 阶段一 | [基础入门](docs/learning/learning-phase1-basics.md) | 项目架构、配置管理、初始化流程、安全机制 |
| 阶段二 | [通信与控制](docs/learning/learning-phase2-communication.md) | WiFi配网、WebSocket协议、PWM原理、ESC校准 |
| 阶段三 | [RTOS多任务](docs/learning/learning-phase3-rtos.md) | FreeRTOS任务、调度器、互斥锁、同步机制 |
| 阶段四 | [PID控制](docs/learning/learning-phase4-pid.md) | PID算法、积分抗饱和、霍尔测速、调参实践 |
| 阶段五 | [状态机与安全](docs/learning/learning-phase5-safety.md) | 有限状态机、Fail-safe设计、多层保护 |

### 可视化图表

每个阶段配套交互式 HTML 可视化图表，用浏览器打开即可学习：

| 阶段 | 图表文件 | 包含内容 |
|------|----------|----------|
| 总览 | [software-architecture.html](docs/learning/diagrams/software-architecture.html) | **软件架构、分层设计、RTOS任务、数据流** |
| 阶段一 | [phase1-diagram.html](docs/learning/diagrams/phase1-diagram.html) | 模块架构图、初始化流程、配置方式对比 |
| 阶段二 | [phase2-diagram.html](docs/learning/diagrams/phase2-diagram.html) | WiFi配网流程、WebSocket架构、PWM信号波形 |
| 阶段三 | [phase3-diagram.html](docs/learning/diagrams/phase3-diagram.html) | 任务优先级、调度时间轴、vTaskDelay对比 |
| 阶段四 | [phase4-diagram.html](docs/learning/diagrams/phase4-diagram.html) | PID公式、响应曲线、积分饱和、霍尔传感器 |
| 阶段五 | [phase5-diagram.html](docs/learning/diagrams/phase5-diagram.html) | 状态转换图、四层保护、报警类型、急停流程 |

> 💡 **学习建议**：按顺序从阶段一到阶段五学习，每个文档包含"为什么这么写"的深度解析、代码逐行解读、动手实践和面试要点。

---

## 🛠 硬件要求

| 组件 | 型号 | 说明 |
|------|------|------|
| MCU | ESP32-C3-DevKitM-1 | 4MB Flash, 160MHz, WiFi 内置 |
| 显示屏 | ST7735S TFT LCD | 1.8" 128x160 像素，SPI 接口 |
| 继电器 | 5V 光耦隔离继电器模块 | 急停/安全开关控制 |
| ESC | BLDC 电调 | 50Hz PWM 控制，1000-2000µs 脉宽 |
| 电流传感器 | INA219 | I2C 接口，32V/2A 量程 |
| 霍尔传感器 | 霍尔开关模块 | 转速脉冲测量，极对数 7 |

---

## 🔌 引脚连接

### ST7735S TFT LCD

```
LCD 引脚顺序: BLK | CS | DC | RST | SDA | SCL | VDD | GND
```

| LCD 引脚 | ESP32-C3 GPIO | 说明 |
|----------|---------------|------|
| BLK | 不接 / 3.3V | 背光（可选） |
| CS | GPIO 7 | 片选信号 |
| DC | GPIO 6 | 数据/命令选择 |
| RST | GPIO 10 | 复位信号 |
| SDA | GPIO 3 | SPI 数据线 (MOSI) |
| SCL | GPIO 2 | SPI 时钟线 (SCK) |
| VDD | 5V | 电源 |
| GND | GND | 地线 |

> ⚠️ **注意**: LCD 供电建议使用 5V 引脚，避免 3.3V 供电不足。详见 [硬件接线指南](docs/HARDWARE_WIRING.md)

### 继电器

| 继电器引脚 | ESP32-C3 GPIO | 说明 |
|------------|---------------|------|
| VCC | 5V | 电源 |
| GND | GND | 地线 |
| IN | GPIO 8 | 控制信号 |

### ESC PWM

| ESC 引脚 | ESP32-C3 GPIO | 说明 |
|----------|---------------|------|
| 信号线 | GPIO 1 | PWM 50Hz 控制信号 |

### INA219 电流传感器 (I2C)

| INA219 引脚 | ESP32-C3 GPIO | 说明 |
|-------------|---------------|------|
| SDA | GPIO 4 | I2C 数据 |
| SCL | GPIO 5 | I2C 时钟 |
| VCC | 3.3V | 电源 |
| GND | GND | 地线 |

### 霍尔传感器

| 霍尔引脚 | ESP32-C3 GPIO | 说明 |
|----------|---------------|------|
| 信号 | GPIO 9 | 转速脉冲输入 (中断) |

---

## 🚀 快速开始

### 1. 安装开发环境

- 安装 [VS Code](https://code.visualstudio.com/)
- 安装 [PlatformIO 插件](https://platformio.org/install/ide?install=vscode)

### 2. 配置项目

编辑 `src/config.h`，修改以下配置：

```cpp
// WiFi 配置
#define WIFI_USE_CONFIG true           // true=硬编码, false=WiFiManager配网
#define WIFI_SSID "your_wifi_ssid"     // WiFi 名称
#define WIFI_PASSWORD "your_password"  // WiFi 密码

// WebSocket 服务器配置
#define WS_SERVER "192.168.1.100"      // 后端 IP
#define WS_PORT 8080                   // 后端端口
#define DEVICE_ID "ESP32_001"          // 设备 ID
#define DEVICE_TOKEN "your_token"      // 认证 Token

// LCD 开关
#define USE_LCD true                   // 启用/禁用 LCD
```

### 3. 编译上传

```powershell
# 编译
C:\Users\Yves\.platformio\penv\Scripts\platformio.exe run

# 上传
C:\Users\Yves\.platformio\penv\Scripts\platformio.exe run -t upload

# 串口监视
C:\Users\Yves\.platformio\penv\Scripts\platformio.exe device monitor -p COM3 -b 115200
```

---

## 📡 WebSocket 通信协议

### 连接端点

```
ws://<server>:<port>/ws/device?device_id=ESP32_001&token=xxx
```

### 状态上报 (ESP32 → Server)

```json
{
  "type": "status",
  "device_id": "ESP32_001",
  "data": {
    "is_running": true,
    "speed_percent": 50,
    "direction": 1,
    "temperature": 45.5,
    "current": 0.8,
    "voltage": 12.0
  }
}
```

### 命令下发 (Server → ESP32)

```json
{
  "type": "command",
  "command_id": 123,
  "command": "start",
  "params": {
    "speed_percent": 50,
    "direction": 1
  }
}
```

### 支持的命令

| 命令 | 参数 | 说明 |
|------|------|------|
| `start` | `speed_percent`, `direction` | 启动电机 |
| `stop` | - | 停止电机 |
| `speed` | `speed_percent` | 设置转速 (0-100) |
| `direction` | `direction` | 设置方向 (1/-1) |
| `lcd` | `content`, `line`, `clear_screen` | LCD 显示内容 |

### 命令确认 (ESP32 → Server)

```json
{
  "type": "command_ack",
  "command_id": 123,
  "success": true,
  "message": "Motor started"
}
```

---

## 📁 项目结构

```
esp32_motor_control/
├── src/
│   ├── main.cpp              # 主程序入口
│   ├── config.h              # 全局配置参数
│   ├── motor_state.h         # 状态数据结构
│   ├── display.h/cpp         # LCD 显示模块
│   ├── relay.h/cpp           # 继电器控制模块
│   ├── wifi_manager.h/cpp    # WiFi 管理模块
│   ├── websocket.h/cpp       # WebSocket 通信模块
│   ├── esc_pwm.h/cpp         # ESC PWM 电机控制
│   ├── pid_controller.h/cpp  # PID 闭环控制
│   ├── hall_sensor.h/cpp     # 霍尔传感器测速
│   ├── ina219_sensor.h/cpp   # INA219 电流传感器
│   ├── motor_fsm.h/cpp       # 电机状态机
│   ├── alarm.h/cpp           # 报警保护系统
│   └── rtos_tasks.h/cpp      # FreeRTOS 多任务
├── docs/
│   ├── HARDWARE_WIRING.md   # 硬件接线指南
│   └── learning/             # 📚 深度学习文档
│       ├── learning-phase1-basics.md
│       ├── learning-phase2-communication.md
│       ├── learning-phase3-rtos.md
│       ├── learning-phase4-pid.md
│       ├── learning-phase5-safety.md
│       └── diagrams/         # 🎨 可视化图表
│           ├── software-architecture.html
│           ├── phase1-diagram.html
│           ├── phase2-diagram.html
│           ├── phase3-diagram.html
│           ├── phase4-diagram.html
│           └── phase5-diagram.html
├── platformio.ini            # PlatformIO 配置
├── README.md                 # 项目说明
└── PLAN.md                   # 开发计划
```

---

## 🔧 配置选项

### platformio.ini

```ini
[env:esp32-c3-devkitm-1]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200

; 依赖库
lib_deps =
    Adafruit GFX Library@^1.11.9
    https://github.com/adafruit/Adafruit-ST7735-Library.git
    ArduinoJson@^7.0.0
    https://github.com/Links2004/arduinoWebSockets.git
    WiFiManager@^2.0.17
    adafruit/Adafruit INA219 @ ^1.2.0

; 编译优化
build_flags =
    -DCORE_DEBUG_LEVEL=3
    -DBOARD_HAS_PSRAM=0
    -Os
    -DNDEBUG

; 上传配置
upload_speed = 921600
upload_port = COM3

; Flash 配置
board_build.flash_mode = dio
board_build.f_flash = 80000000L
```

---

## 📊 资源占用

| 资源 | 使用量 | 总量 | 占比 |
|------|--------|------|------|
| RAM | ~40 KB | 320 KB | 12% |
| Flash | ~905 KB | 4 MB | 23% |

---

## 📄 许可证

MIT License

---

## 🙏 致谢

本项目学习文档旨在帮助嵌入式开发者从"会用"到"理解原理"，适合：
- 嵌入式开发初学者
- 准备嵌入式面试的开发者
- 想深入理解 RTOS、PID、状态机的工程师
