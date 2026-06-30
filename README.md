# MSP430F5529 多点温度/环境监测系统

本工程面向“单片机技术课程设计”，主控为 TI MSP430F5529 LaunchPad。
系统采集 DHT11 温湿度、MQ-2 可燃气体 ADC 值和 HC-SR04 超声波距离，
通过 OLED 显示、HC-05 蓝牙上传、内部 Flash 保存历史记录，并用蜂鸣器
进行分级报警。温度报警阈值可由 EC11 编码器或蓝牙命令调整。

## 功能

- DHT11：P1.0 单总线读取温湿度。
- MQ-2：P6.0/ADC12_A0 多次采样求平均，换算 0~5 气体等级。
- HC-SR04：Trig=P1.2，Echo=P1.3，软件微秒计时测距。
- OLED SSD1306：P3.0/P3.1 软件 I2C，显示温湿度、气体、距离、阈值和报警等级。
- HC-05：UCA1 P4.4/P4.5，手机串口收发实时数据和控制命令。
- 内部 Flash：Info B/C/D 固定结构保存历史记录，支持按序号读取和清空。
- 蜂鸣器：P2.0，报警等级越高节奏越急促。
- EC11：P2.1/P2.2/P2.3，旋转调整温度阈值，按键触发刷新。

## 目录

```text
application/env_monitor.*    应用调度、蓝牙协议、OLED 页面、报警和历史记录
drivers/env_sensors.*        DHT11、MQ-2、HC-SR04 采样驱动
drivers/oled_ssd1306.*       SSD1306 软件 I2C 显示驱动
drivers/flash_log.*          MSP430 内部 Information Flash 历史记录
drivers/alarm_buzzer.*       蜂鸣器分级报警节奏
drivers/bluetooth_uart.*     HC-05 UART 通信
drivers/encoder.*            EC11 正交解码和按键事件
tools/                       编译验证、协议仿真、报告和图生成
android/                     HC-05 手机端监测 APK
docs/                        课程报告、框图、流程图和验证报告
```

## 蓝牙命令

| 命令 | 功能 |
| --- | --- |
| `?` | 立即上传实时数据 |
| `i` | 固件信息 |
| `w` | 接线清单 |
| `h` | 帮助 |
| `x` | 最近控制轨迹 |
| `T+` / `T-` | 温度阈值增减 0.5°C |
| `SETT=32.0` | 直接设置温度阈值 |
| `HIST?` | 历史记录数量 |
| `HIST 1` | 读取第 1 条历史记录 |
| `DUMP` | 输出全部历史记录 |
| `CLRLOG` | 清空历史记录 |

示例：

```text
DATA T=26.3 H=58 GAS=1250 L=0 D=342 TH=30.0 ALM=0
REC 1 U=10 T=25.0 H=55 GAS=1200 D=410 TH=30.0 ALM=0
```

## 编译与验证

```powershell
powershell -ExecutionPolicy Bypass -File tools\run_verification.ps1
powershell -ExecutionPolicy Bypass -File tools\verify_android_apk.ps1
```

`run_verification.ps1` 会执行 CCS clean build、固件静态检查、可读性审计、
蓝牙协议仿真、整板场景仿真、Flash 日志仿真、阈值调整仿真，并生成课程
报告、硬件框图、软件流程图和软件效果验收报告。

## 重要硬件注意

- HC-SR04 Echo 常为 5V，接 P1.3 前必须分压或电平转换到 3.3V。
- MQ-2 模块 AO 不能超过 MSP430 ADC 输入范围。
- DHT11 占用 P1.0，状态 LED 已改到 P4.7。
- HC-05 默认使用 P4.4/P4.5，避免 P3.3 与其他外设冲突。
- 本仓库已完成软件编译和主机侧仿真；真实传感器精度、蓝牙配对、OLED
  显示、蜂鸣器效果和 Flash 实写仍需上板验证。
