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
- 曲目 1-9：发送 `1` 到 `9`
- 查询状态：发送 `?`
- 实时显示 MSP430 固件回传的 `status=...` 文本

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
