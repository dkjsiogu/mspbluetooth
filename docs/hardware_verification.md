# 硬件现场验证清单

本清单用于把软件仿真推进到实物确认。建议按顺序做，每一步通过后再接下一类外设，避免多个问题叠加。

## 1. 烧录前检查

- 确认固件：`Debug\mspbluetooth.out`
- 确认 APK：`android\app\build\outputs\apk\debug\app-debug.apk`
- TF 卡根目录准备 16-bit PCM WAV：`TRACK01.WAV` 到 `TRACK09.WAV`
- 建议首测 WAV：8 kHz 或 16 kHz，双声道或单声道均可

## 2. 最小系统

只接 MSP430F5529 和 USB 调试器，不接 TF、DAC、功放。

预期：

- 烧录后 P1.0 状态 LED 约 500 ms 翻转一次
- 无明显复位循环

## 3. 蓝牙链路

接线：

- HC-05 VCC 接 3.3 V
- HC-05 GND 接 GND
- HC-05 RXD 接 P4.4
- HC-05 TXD 接 P4.5

手机操作：

- 系统蓝牙先配对 HC-05
- 安装并打开 APK
- 选择 HC-05 后连接

测试命令与预期：

```text
? -> status=...
i -> info name=MSP430F5529-BT-WAV version=1.1.0 ...
e -> selftest bt=ok sd=... file=...
d -> display 1/2/3 ...
```

如果连接失败：

- 先用系统蓝牙确认 HC-05 已配对
- 确认 APK 有 Bluetooth permission
- 确认当前固件默认蓝牙是 UCA1 P4.4/P4.5

## 4. DAC/功放/喇叭链路

接线：

- PCM5102A BCK 接 P4.1
- PCM5102A LRCK 接 P4.2
- PCM5102A DIN 接 P4.3
- PCM5102A 供电与 GND 按模块要求连接
- PCM5102A 模拟输出接 PAM8403 输入或耳机座

测试命令：

```text
t
```

预期：

- APK 日志显示 `tone start` 和 `tone done`
- 喇叭或耳机能听到短测试音

如果没有声音：

- 先确认功放供电、喇叭接线和音量
- 发送 `+` 多次提高音量
- 发送 `m` 确认没有静音
- 用逻辑分析仪看 P4.1/P4.2/P4.3 是否有跳变

## 5. TF 卡与 WAV 播放

接线：

- TF CS 接 P4.0
- TF SCK 接 P3.1
- TF MOSI 接 P3.2
- TF MISO 接 P3.3

测试命令：

```text
1 -> open TRACK01.WAV ...
p -> play/pause
n -> next track
b -> previous track
r -> replay
s -> stop
```

预期：

- 能打开 `TRACK01.WAV`
- 能听到 WAV 音频
- `n/b/r/s/p` 状态响应符合日志

## 6. 本地输入

EC11：

- A 接 P2.1
- B 接 P2.2
- SW 接 P2.3

本地按键：

- S1=P1.2：播放/暂停
- S2=P1.3：上一曲
- S4=P2.6：下一曲

预期：

- EC11 旋转改变音量，APK 日志显示 `volume=...`
- EC11 按下切换播放/暂停
- S1/S2/S4 映射与蓝牙命令一致

## 7. 墨水屏可行性说明

参考工程 `E:\code\ccs\GPIO\LED` 的墨水屏路径使用了多根 GPIO。该项目当前主线已经占用：

- P4.1/P4.2/P4.3：PCM5102A I2S
- P3.1/P3.2/P3.3：TF 卡 SPI
- P2.1/P2.2/P2.3：EC11
- P4.4/P4.5：HC-05

其中 P3.2/P3.3、P2.2 等与参考墨水屏方案存在冲突。为了不影响音频播放器主功能，当前不把墨水屏并入默认构建。状态显示由 APK、蓝牙自动上报和 `d` 三行显示帧承担；如果后续确实要加墨水屏，应先重新分配硬件引脚，再把 `middleware/display_model.*` 输出的三行文本接到屏幕渲染层。
