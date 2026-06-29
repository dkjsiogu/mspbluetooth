# Android 蓝牙控制端

这是课程设计配套的安卓控制 APK。手机先在系统蓝牙中与 HC-05 配对，然后打开 App 选择 HC-05 并连接。

## 支持功能

- 播放/暂停：发送 `p`
- 停止：发送 `s`
- 上一曲/下一曲：发送 `b`、`n`
- 音量加减：发送 `+`、`-`
- 重播当前曲：发送 `r`
- 静音/恢复：发送 `m`
- 播放顺序：发送 `o`，在顺序停止、全部循环、单曲循环之间切换
- DAC 测试音：发送 `t`
- 固件信息：发送 `i`
- 自检摘要：发送 `e`
- 曲目扫描：发送 `l`
- 三行显示帧：发送 `d`
- 蓝牙链路计数：发送 `k`
- EC11/本地按键计数：发送 `u`
- 曲目 1-9：发送 `1` 到 `9`
- 查询状态：发送 `?`
- 实时显示 MSP430 固件回传的 `status=...` 文本
- 自动解析 `status=...` 为 Mode、Track、Volume、Order、Progress 状态面板
- 自动解析 `display 1/2/3:...` 为三行显示帧，便于模拟后续墨水屏显示效果
- 自动解析 `link ...` 为 RX、状态上报、显示帧上报、异常命令、最后命令和运行时间面板
- 自动解析 `input ...` 为 EC11 和 S1/S2/S4 短按、长按计数面板

## 构建

```powershell
cd E:\code\ccs\mspbluetooth\android
..\.tools\gradle\bin\gradle.bat assembleDebug
```

实际验证命令以仓库根目录的 `tools\build_android_apk.ps1` 为准。

产物：

```text
android\app\build\outputs\apk\debug\app-debug.apk
```

## Acceptance Button

The APK includes `Run Acceptance`. After connecting to HC-05, this button sends
`h i e l d ? t 1 p + n b o 3 k u`, appends `TX>` markers to the phone log, and lets
the status, display-frame, track-list, Link, Input, and acceptance summary panels update from firmware responses.
Use `Save Log` to write the phone log as a text file, or `Share Log` to send it through the Android share sheet, then check it on the PC:

```powershell
python tools\serial_acceptance_check.py --input path\to\phone_log.txt
python tools\android_acceptance_log_sim.py
```
