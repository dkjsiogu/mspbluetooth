# 单片机技术课程设计报告：多点温度/环境监测系统

## 1. 课题与平台

- 课题名称：多点温度/环境监测系统
- 主控平台：TI MSP430F5529 LaunchPad
- 固件版本：2.0.0
- 外设：DHT11、MQ-2、HC-SR04、SSD1306 OLED、HC-05、EC11、蜂鸣器
- 存储：MSP430 内部 Information Flash

## 2. 设计目标

系统周期采集温湿度、可燃气体 ADC 值和超声波距离，在 OLED 上显示三项核心数据，通过 HC-05 蓝牙向手机发送实时数据，并把历史记录保存到内部 Flash。温度或气体超过阈值时触发蜂鸣器报警，报警等级越高鸣叫越急促。温度报警阈值可通过 EC11 旋转编码器或蓝牙命令调整。

## 3. 系统总体方案

- 应用层：`application/env_monitor.*` 负责周期调度、报警等级、OLED 页面、蓝牙协议、Flash 记录和 EC11 阈值调整。
- 传感器驱动：`drivers/env_sensors.*` 负责 DHT11 单总线、MQ-2 ADC12 平均采样、HC-SR04 微秒级测距。
- 输出驱动：`drivers/oled_ssd1306.*` 负责软件 I2C OLED，`drivers/alarm_buzzer.*` 负责报警节奏。
- 存储驱动：`drivers/flash_log.*` 使用 Info B/C/D 保存固定长度历史记录，支持追加、清空和按序号读取。
- 通信与输入：`drivers/bluetooth_uart.*` 使用 UCA1 P4.4/P4.5，`drivers/encoder.*` 识别 EC11 旋转和按键事件。
- 验证工具：`tools/` 提供 clean build、静态检查、可读性审计、协议仿真、整板场景仿真、Flash 日志仿真、阈值调整仿真、报告和图生成。

系统硬件框图见下图：

![硬件框图](hardware_block_diagram.svg)

## 4. 硬件连接

| 模块 | 连接 |
| --- | --- |
| DHT11 | VCC=3.3V，GND，DATA=P1.0 |
| MQ-2 | AO=P6.0/ADC12_A0，GND，模块供电按规格接入，AO 必须限制到 3.3V 以内 |
| HC-SR04 | Trig=P1.2，Echo=P1.3，Echo 需要分压/电平转换到 3.3V |
| OLED SSD1306 | SDA=P3.0，SCL=P3.1，软件 I2C |
| HC-05 | HC-05 RXD 接 P4.4/TXD，HC-05 TXD 接 P4.5/RXD |
| 蜂鸣器 | P2.0 GPIO 控制 |
| EC11 | A=P2.1，B=P2.2，SW=P2.3 |
| 状态 LED | P4.7，避开 DHT11 的 P1.0 |

## 5. 软件流程

上电后初始化时钟、GPIO、系统 tick、UART、EC11、传感器、OLED、蜂鸣器和 Flash。主循环持续执行 `env_monitor_service()`：按周期采样三类传感器，刷新 OLED，主动推送蓝牙 `DATA`，定时写 Flash 历史，轮询蓝牙命令和 EC11 事件，最后根据温度超限和气体等级刷新蜂鸣器报警等级。

软件流程图见下图：

![软件流程图](software_flowchart.svg)

## 6. 蓝牙命令协议

| 命令 | 功能 |
| --- | --- |
| `?` | 立即上传一行实时数据 |
| `i` | 固件名称、版本和硬件摘要 |
| `w` | 输出接线清单 |
| `h` | 输出命令帮助 |
| `x` | 输出最近控制轨迹 |
| `T+` | 温度报警阈值增加 0.5°C |
| `T-` | 温度报警阈值减少 0.5°C |
| `SETT=32.0` | 直接设置温度报警阈值 |
| `HIST?` | 查询 Flash 历史记录数量 |
| `HIST n` | 读取第 n 条历史记录 |
| `DUMP` | 输出全部历史记录 |
| `CLRLOG` | 清空内部 Flash 日志 |

示例回传：

```text
DATA T=26.3 H=58 GAS=1250 L=0 D=342 TH=30.0 ALM=0
TH=32.0 SAVED
HIST COUNT=2
REC 1 U=10 T=25.0 H=55 GAS=1200 D=410 TH=30.0 ALM=0
pin hcsr04 trig=P1.2 echo=P1.3 note=echo-divide-to-3v3
```

## 7. 验证证据

自动验证命令：

```powershell
powershell -ExecutionPolicy Bypass -File tools\run_verification.ps1
```

验证覆盖：

- CCS gmake clean build，生成 `Debug/mspbluetooth.out`。
- `firmware_static_check.py` 检查新固件模块、命令协议、引脚表、makefile 对象和 RAM 余量。
- `readability_audit.py` 检查当前项目文件的中文注释、宏说明、声明参数说明和顶层静态项说明。
- `bluetooth_protocol_sim.py` 覆盖实时数据、阈值、历史、清空、接线、信息和轨迹命令。
- `board_scenario_sim.py` 覆盖正常、温度报警、气体报警、组合危险和超声波超时场景。
- `flash_log_sim.py` 覆盖内部 Flash 追加、满容量擦除、读取边界和 CRC。
- `encoder_threshold_sim.py` 覆盖 EC11 与蓝牙命令共同调整温度阈值。
- `generate_diagrams.py`、`generate_course_report.py`、`generate_effect_report.py` 生成图表和验收报告。

## 8. 实物验收计划

1. 断电检查所有电源和地线，确认 MQ-2 AO 与 HC-SR04 Echo 不会把 5V 直接送入 MSP430。
2. 烧录 `Debug/mspbluetooth.out`，串口观察启动信息和 `cmd ? i w x...` 帮助。
3. 单独接入 DHT11，确认 `DATA T=... H=...` 合理。
4. 接入 MQ-2，等待预热后观察 `GAS` 和 `L` 等级变化。
5. 接入 HC-SR04，移动障碍物确认 `D=...` 和 OLED 第三行变化。
6. 连接 HC-05 手机串口，测试 `?`、`T+`、`T-`、`SETT=32.0`、`HIST?`、`DUMP`、`CLRLOG`。
7. 旋转 EC11，确认阈值随旋转方向增减；按键触发单次刷新。
8. 提高温度或 MQ-2 模拟值，确认蜂鸣器节奏随报警等级变急。
9. 运行一段时间后读取 Flash 历史记录，确认断开传感器后仍可独立读取已有历史。

## 9. 风险与说明

当前完成的是软件构建和主机侧仿真验证，未进行真实烧录和硬件测量。DHT11 时序、HC-SR04 Echo 分压、MQ-2 预热与 ADC 电压范围、OLED 地址、蜂鸣器驱动能力、HC-05 配对稳定性和 Flash 实写寿命都必须在实物上确认。烧录前不要把 5V 信号直接接到 MSP430 GPIO/ADC。

## 10. 总结

本项目把原有蓝牙音频播放器结构转换为多点温度/环境监测系统，形成了传感器采集、OLED 显示、蓝牙通信、内部 Flash 历史、蜂鸣器报警和编码器阈值调整的完整软件框架。代码按应用、驱动、通信、存储和验证工具分层，并通过自动化脚本保留可重复的构建与仿真证据，便于课程验收和后续上板调试。
