# 硬件现场验证清单

本清单用于 MSP430F5529 多点温度/环境监测系统上板验收。软件仿真通过只
证明固件、APK 和协议逻辑可工作；真实传感器、电平、显示和报警效果必须
按本清单逐项确认。

## 1. 上电前检查

- MSP430F5529、面包板、HC-05、DHT11、MQ-2、HC-SR04、OLED、蜂鸣器、
  EC11 共地。
- DHT11 DATA 接 P1.0，并有上拉。
- MQ-2 AO 接 P6.0 前先测量电压，确认进入 MSP430 ADC 的电压不超过 3.3 V。
- HC-SR04 Echo 接 P1.3 前必须经过分压或电平转换，确认 P1.3 不会收到 5 V。
- OLED SDA=P3.0，SCL=P3.1，确认模块地址为常见 SSD1306 地址。
- HC-05 RXD 接 P4.4/UCA1TXD，TXD 接 P4.5/UCA1RXD。
- 蜂鸣器控制脚接 P2.0；若使用无源蜂鸣器，应改成 PWM 驱动后再验收。
- EC11 A=P2.1，B=P2.2，SW=P2.3，三路均使用上拉。

## 2. 烧录后基础检查

1. 只连接 MSP430F5529 和 USB 调试器，暂不接 5 V 传感器输出。
2. 烧录 `Debug\mspbluetooth.out`。
3. 打开手机端或串口助手，连接 HC-05 后应看到启动帮助：

```text
MSP430F5529 ENV monitor
cmd ? i w x T+ T- SETT=32.0 HIST? HIST n DUMP CLRLOG
```

4. 发送 `i`，应返回：

```text
info name=MSP430F5529-ENV-MON version=2.0.0 ...
```

## 3. 传感器逐项验证

### DHT11

- 接入 DHT11 后发送 `?`。
- 期望 `DATA T=... H=...` 数值合理。
- 若长时间为默认值或跳变异常，检查 P1.0 上拉和单总线接线。

### MQ-2

- 上电后预热，预热前数据可显示但不作为报警依据。
- 发送 `?`，观察 `GAS=... L=...`。
- 安全改变气体环境时，`GAS` 和 `L` 应随之变化。

### HC-SR04

- 移动障碍物并发送 `?`。
- `D=...` 应随距离变化；超时或无回波时 OLED 第三行显示 `D:----MM`。
- 若读数固定为 0，重点检查 Trig、Echo 分压和共地。

## 4. OLED 显示

OLED 应显示四行：

```text
T:26.3C H:58%
GAS:1250 L:0
D:342MM
TH:30.0 ALM:0
```

实际数值可不同，但每行含义应一致，且文字不应明显乱码或越界。

## 5. 蓝牙和 Android APK

- 运行 `tools\verify_android_apk.ps1` 构建 APK。
- 手机安装 `android\app\build\outputs\apk\debug\app-debug.apk`。
- 连接 HC-05 后点击 `Data`，手机 DATA 面板应更新。
- 点击 `Wiring`，应显示 DHT11、MQ-2、HC-SR04、OLED、BT、buzzer、EC11 引脚。
- 点击 `Demo RX` 可在无硬件时演示面板解析效果。
- 点击 `Save Log` 保存 TXT 日志，作为课程验收记录。

## 6. Flash 历史记录

1. 等待至少一个 `ENV_FLASH_LOG_MS` 周期。
2. 发送 `HIST?`，应返回 `HIST COUNT=n`。
3. 发送 `HIST 1`，应返回 `REC 1 ...`。
4. 发送 `DUMP`，应逐条输出历史并以 `DUMP END` 结束。
5. 发送 `CLRLOG` 后再发送 `HIST?`，应返回 `HIST COUNT=0`。

## 7. 阈值和报警

- 发送 `T+`，温度阈值增加 0.5°C。
- 发送 `T-`，温度阈值减少 0.5°C。
- 发送 `SETT=32.0`，应返回 `TH=32.0 SAVED`。
- 顺时针旋转 EC11，阈值增加；逆时针旋转，阈值降低。
- 温度超过阈值越多或 MQ-2 等级越高，`ALM` 越高，蜂鸣器节奏应越急促。

## 8. 记录表

现场结果填写到 `docs\test_record.csv`。只有真实硬件逐项通过后，才能声明
硬件验收完成；软件仿真和打包通过不能替代实物验证。
