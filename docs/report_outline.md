# 课程设计报告提纲

可运行以下命令按当前工程内容生成完整报告初稿：

```powershell
python tools\generate_course_report.py
```

输出文件为 `docs\course_report_draft.md`。实物测试完成后，应把 `docs\test_record.csv` 中的实际现象和照片/示波器截图补入正式报告。

硬件框图和软件流程图可运行以下命令生成：

```powershell
python tools\generate_diagrams.py
```

## 1. 课题名称

蓝牙遥控音频播放器

## 2. 设计目标

- 使用 MSP430F5529 作为主控
- 使用 HC-05 实现手机蓝牙远程控制
- 使用 TF 卡存储 WAV 音频文件
- 使用 PCM5102A I2S DAC 输出音频
- 使用 EC11 和本地按键完成离线控制
- 使用 APK 和蓝牙日志显示状态、Health/Storage 与自检结果

## 3. 系统方案

系统分为五个子系统：

- 主控与时钟：MSP430F5529，16 MHz MCLK/SMCLK
- 存储子系统：TF 卡、FatFs、软件 SPI
- 音频子系统：WAV 解析、软件 I2S、PCM5102A、PAM8403
- 人机交互：HC-05、Android APK、EC11、本地按键
- 验证与诊断：Health/Storage 面板、测试音、自检命令、显示帧、蓝牙链路计数、本地输入计数、接线诊断、仿真脚本

## 4. 硬件连接

核心连接见 `docs/hardware_verification.md`。报告中应重点说明 P3.3 的冲突：课程资料中 HC-05 和 TF MISO 都使用 P3.3，因此本工程默认把 HC-05 改到 UCA1 的 P4.4/P4.5。

## 5. 软件结构

```text
main.c                    初始化和主循环
application/audio_player.* 播放器状态机和命令解析
drivers/                  UART、EC11、本地按键、软件 I2S、板级时钟
middleware/               WAV 解析和显示帧模型
fatfs/                    FatFs 和 TF 卡适配
android/                  手机蓝牙控制 APK
tools/                    自动编译、静态检查、协议仿真和 APK 验证
```

## 6. 蓝牙命令协议

| 命令 | 功能 |
| --- | --- |
| `p` | 播放/暂停 |
| `s` | 停止 |
| `r` | 重播 |
| `n` | 下一曲 |
| `b` | 上一曲 |
| `+` | 音量加 |
| `-` | 音量减 |
| `m` | 静音/恢复 |
| `t` | DAC 测试音 |
| `i` | 固件和硬件映射 |
| `e` | 自检摘要 |
| `l` | 曲目扫描 |
| `d` | 三行显示帧 |
| `1-9` | 直接播放曲目 |
| `?` | 状态查询 |
| `k` | 蓝牙链路计数 |
| `u` | EC11 和本地按键计数 |
| `w` | 实际接线诊断 |

## 7. 测试方法

- 运行 `tools\run_verification.ps1` 验证固件编译和协议仿真
- 运行 `tools\verify_android_apk.ps1` 验证 APK 构建和权限
- 运行 `tools\prepare_sdcard_assets.py` 生成 TF 卡测试 WAV
- 按 `docs/hardware_verification.md` 做实物验证
- 填写 `docs/test_record.csv`

## 8. 结论

说明已实现功能、遇到的硬件冲突、解决方案、仍需通过真机确认的项目。
