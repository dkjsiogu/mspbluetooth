# MSP430F5529 蓝牙遥控音频播放器

本工程对应课程设计选题（三）：蓝牙遥控音频播放器。代码按参考工程的思路拆成三层，但去掉了和本项目无关的显示、温度采集和 RTOS 代码，方便阅读、调试和写报告。

## 功能

- TF 卡读取 `TRACK01.WAV` 到 `TRACK09.WAV`。
- 解析 16-bit PCM WAV 文件，并用软件 I2S 输出到 PCM5102A。
- EC11 旋转编码器调节音量，按键播放/暂停。
- S1/S2/S4 本地按键：短按播放/暂停、上一曲、下一曲；长按停止、静音、播放顺序切换。
- HC-05 蓝牙串口接收遥控命令。
- 蓝牙命令、EC11 和本地按键改变状态后立即回传 `status=...` 和 `display 1/2/3:...`，并每 5 秒自动推送一次当前状态，方便手机端和串口助手确认链路。
- 蓝牙 `t` 命令可直接让 PCM5102A 输出测试音，不依赖 TF 卡音频文件，便于现场确认 DAC/功放/喇叭链路。
- 蓝牙 `i`/`e` 命令可输出固件版本、硬件映射和软件可见自检摘要。
- 蓝牙 `l` 命令扫描 `TRACK01.WAV` 到 `TRACK09.WAV`，显示哪些曲目可播放。
- 蓝牙 `d` 命令输出三行显示帧，当前由 APK/串口显示，后续可接到墨水屏渲染层。
- `tools\epaper_preview_sim.py` 可把三行显示帧渲染成 296x128 黑白 PGM 预览图，用于未接墨水屏时确认显示效果。
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
- 本地按键：`S1=P1.2` 短按播放/暂停、长按停止；`S2=P1.3` 短按上一曲、长按静音；`S4=P2.6` 短按下一曲、长按切换播放顺序
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

可直接生成测试音频：

```powershell
python tools\prepare_sdcard_assets.py
python tools\wav_asset_check.py --report dist\verification\wav_asset_report.md
```

然后把 `sdcard\TRACK01.WAV`、`sdcard\TRACK02.WAV`、`sdcard\TRACK03.WAV` 复制到 TF 卡根目录。

## 蓝牙命令

```text
p    播放/暂停
s    停止并回到当前曲目开头
r    重播当前曲目
n/>  下一曲
b/<  上一曲
+    音量加
-    音量减
m    静音/恢复
o    切换播放顺序：顺序停止/全部循环/单曲循环
t    DAC 测试音
i    固件与硬件映射信息
e    自检摘要
l    扫描曲目文件
d    三行显示帧
1-9  直接播放对应曲目
?    查询状态
h    输出命令帮助
```

状态回传格式示例。播放、停止、上下曲、音量、静音、播放顺序、测试音和直接选曲命令执行后会立即追加 `status=...` 和三行 `display`：

```text
status=playing track=1 volume=18 order=repeat_all rate=16000Hz channels=2 progress=0
order=repeat_one
info name=MSP430F5529-BT-WAV version=1.4.0 profile=TF:P3.1-3.3 I2S:P4.1-4.3 BT:UCA1
selftest bt=ok sd=ok file=open dac=test-with-t
tracks 1=ok 2=-- 3=ok 4=-- 5=-- 6=-- 7=-- 8=-- 9=--
display 1:playing T1 V18 ALL
display 2:SD:OK WAV:OPEN
display 3:16000Hz 2ch P0%
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
- 静态检查头文件/源码文件头、声明参数说明、宏说明、static 项说明、关键命令、引脚冲突说明、RAM 余量
- 模拟 HC-05 单字符蓝牙命令链路
- 模拟 HC-05 字节流诊断链路，覆盖分片输入、大小写命令、换行噪声和现场自检转录
- 模拟 Android 端对碎片化蓝牙回传的状态面板和显示帧解析
- 检查 Android APK 命令按钮覆盖、HC-05 SPP 权限、连接后自动同步状态/曲目/显示帧和结构化回传解析覆盖
- 模拟端到端演示流程：APK 按钮、HC-05 命令、固件回包、手机状态面板和显示帧变化
- 模拟蓝牙、EC11、本地按键混合控制场景
- 模拟 EC11 A/B 正交相位解码、反向抖动抵消和按键去抖
- 模拟 S1/S2/S4 去抖、短按、长按事件，确认长按不会误触发短按
- 生成并检查墨水屏风格黑白预览图和多状态预览画廊，确认显示帧不是空白
- 校验 TF 卡测试 WAV 是否为固件支持的 RIFF/WAVE PCM、16-bit、单/双声道格式
- 模拟 TF WAV 到软件 I2S 样本流，确认读块、音量缩放、非静音输出和播放进度
- 模拟 PCM5102A 软件 I2S 帧结构，确认 64 个 BCK、LRCK 左右槽、MSB 延迟和 padding
- 生成软件效果验收报告，汇总蓝牙命令、整板场景、按键、显示帧、APK 解析和 WAV 资产证据

## 交付打包

```powershell
cd E:\code\ccs\mspbluetooth
powershell -ExecutionPolicy Bypass -File tools\package_release.ps1
```

脚本默认先执行固件验证和 APK 验证，然后生成：

```text
dist\mspbluetooth_delivery\
```

目录内包含 `Debug\mspbluetooth.out`、Android 控制端 APK、TF 卡测试 WAV、验收文档、软件效果验收报告、蓝牙诊断报告、Android 命令覆盖报告、端到端演示报告、音频流仿真报告、I2S 帧仿真报告、报告提纲、测试记录表、墨水屏预览图和多状态预览画廊、`MANIFEST.txt` 和 `SHA256SUMS.txt`。该包用于课程提交前的软件交付整理；实物烧录、HC-05 连接、DAC 出声和 EC11 操作仍需按现场清单逐项确认。

## 实物验证

上板前按 [硬件现场验证清单](docs/hardware_verification.md) 分阶段检查蓝牙、DAC 测试音、TF 卡 WAV 播放、EC11 和本地按键。墨水屏引脚冲突分析也在该文档中。

课程报告和验收材料：

- [课程设计报告提纲](docs/report_outline.md)
- [课程设计报告初稿](docs/course_report_draft.md)
- [软件效果验收报告](docs/effect_acceptance_report.md)
- [蓝牙诊断报告](docs/bluetooth_diagnostic_report.md)
- [Android 命令覆盖报告](docs/android_command_coverage_report.md)
- [端到端演示仿真报告](docs/end_to_end_demo_report.md)
- [音频流仿真报告](docs/audio_stream_report.md)
- [I2S 帧仿真报告](docs/i2s_frame_report.md)
- [墨水屏多状态预览报告](docs/epaper_gallery_report.md)
- [硬件框图](docs/hardware_block_diagram.svg)
- [软件流程图](docs/software_flowchart.svg)
- [功能验收矩阵](docs/acceptance_matrix.md)
- [测试记录表](docs/test_record.csv)

## Android 控制端

仓库包含 `android/` 原生 Java 控制端，用于手机通过 HC-05 控制播放器。APK 不只发送命令，也会解析固件回传的 `status=...`、`display 1/2/3:...` 和 `tracks ...`，在手机上显示当前播放状态、三行显示帧和曲目可用状态，便于未接墨水屏时确认显示效果。构建 APK：

```powershell
cd E:\code\ccs\mspbluetooth
powershell -ExecutionPolicy Bypass -File tools\build_android_apk.ps1
```

生成文件：

```text
android\app\build\outputs\apk\debug\app-debug.apk
```

验证 APK 包名、SDK 和蓝牙权限：

```powershell
powershell -ExecutionPolicy Bypass -File tools\verify_android_apk.ps1
```

## Serial Acceptance Transcript

Before real flashing, the software verifier generates a no-hardware HC-05
acceptance transcript and checks that the expected evidence is present:

```powershell
python tools\serial_acceptance_check.py
```

After real board testing, save the Bluetooth/serial-assistant log as text and
run:

```powershell
python tools\serial_acceptance_check.py --input path\to\capture.txt
```

The checker looks for `sd mounted`, `info name=MSP430F5529-BT-WAV`,
`selftest bt=ok`, `tracks 1=ok`, `display 1/2/3:`, `status=...`,
`tone start`, `tone done`, and `open TRACK01.WAV`/`open TRACK03.WAV`. If the
log includes `TX>` command markers, it also verifies that state-changing
commands immediately return `status=...` plus the three display lines.
