# MSP430F5529 蓝牙遥控音频播放器

本工程对应课程设计选题（三）：蓝牙遥控音频播放器。代码按参考工程的思路拆成三层，但去掉了和本项目无关的显示、温度采集和 RTOS 代码，方便阅读、调试和写报告。

## 功能

- TF 卡读取 `TRACK01.WAV` 到 `TRACK09.WAV`。
- 解析 16-bit PCM WAV 文件，并用软件 I2S 输出到 PCM5102A。
- EC11 旋转编码器调节音量，按键播放/暂停。
- S1/S2/S4 本地按键：播放/暂停、上一曲、下一曲。
- HC-05 蓝牙串口接收遥控命令。
- 蓝牙端每 5 秒自动推送一次当前状态，方便手机端和串口助手确认链路。
- DAC 模拟输出可同时接 PAM8403 功放输入和 3.5mm 耳机座。

## 代码结构

```text
main.c                    启动入口，只保留初始化顺序和主循环
application/audio_player.* 播放器状态机、蓝牙命令、曲目切换、音量控制
drivers/                  MSP430 寄存器驱动：时钟、UART、EC11、软件 I2S、板级引脚
middleware/wav_reader.*    WAV 文件头解析
fatfs/                    FatFs 和 TF 卡底层 SPI 适配
Debug/makefile             命令行编译入口
```

## 硬件接线

- TF 卡：`CS=P4.0`，`SCK=P3.1`，`MOSI=P3.2`，`MISO=P3.3`
- PCM5102A：`BCK=P4.1`，`LRCK=P4.2`，`DIN=P4.3`
- EC11：`A=P2.1`，`B=P2.2`，`SW=P2.3`
- 本地按键：`S1=P1.2` 播放/暂停，`S2=P1.3` 上一曲，`S4=P2.6` 下一曲
- 状态 LED：`P1.0`
- 默认蓝牙：`HC-05 RXD<-P4.4`，`HC-05 TXD->P4.5`

注意：课程资料中写了 HC-05 复用 `P3.3/P3.4`，但 `P3.3` 同时也是 TF 卡 `MISO`。两者同时接会产生总线冲突。当前代码默认把蓝牙放到 `UCA1 P4.4/P4.5`，这样 TF 卡和蓝牙可以同时工作。如必须使用 `P3.3/P3.4`，需要先改硬件分配或只保留 HC-05 单向接收。

S3/P2.3 已分配给 EC11 按键，因此没有再作为独立本地按键使用。

## TF 卡文件

把 WAV 文件放在 TF 卡根目录，命名为：

```text
TRACK01.WAV
TRACK02.WAV
...
TRACK09.WAV
```

当前播放器支持 PCM、16-bit、单声道或双声道 WAV。软件 I2S 由 CPU 翻转 GPIO 产生，建议测试文件先使用 8 kHz 或 16 kHz 采样率，便于课堂演示和逻辑分析仪观察。

## 蓝牙命令

```text
p    播放/暂停
s    停止并回到当前曲目开头
n/>  下一曲
b/<  上一曲
+    音量加
-    音量减
1-9  直接播放对应曲目
?    查询状态
h    输出命令帮助
```

自动状态上报格式示例：

```text
status=playing track=1 volume=18 rate=16000Hz channels=2
```

## 编译

```powershell
cd E:\code\ccs\mspbluetooth\Debug
D:\ccs\ccs\utils\bin\gmake.exe all
```

生成文件：

```text
Debug\mspbluetooth.out
```

本阶段只编译，不自动烧录。

## 自动验证

```powershell
cd E:\code\ccs\mspbluetooth
powershell -ExecutionPolicy Bypass -File tools\run_verification.ps1
```

验证内容：

- clean build 固件并生成 `Debug\mspbluetooth.out`
- 静态检查头文件注释、关键命令、引脚冲突说明、RAM 余量
- 模拟 HC-05 单字符蓝牙命令链路
- 模拟蓝牙、EC11、本地按键混合控制场景
