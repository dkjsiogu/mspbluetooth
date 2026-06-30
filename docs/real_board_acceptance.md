# 真实硬件验收入口

本页用于烧录实物后收集证据。软件仿真通过只能说明代码、APK、协议和检查器可工作；要声明板级完成，至少需要真实 HC-05/APK 日志和真实 I2S 逻辑分析仪 CSV。

## 1. 先跑一次流程自检

```powershell
powershell -ExecutionPolicy Bypass -File tools\run_real_board_acceptance.ps1 -UseGeneratedSamples
```

输出目录：

```text
dist\real_board_acceptance
```

`mode=sample` 只证明验收流程可运行，不能作为实物完成证据。

## 2. 实物证据采集

1. 烧录 `Debug\mspbluetooth.out` 到 MSP430F5529。
2. 手机安装 `android\app\build\outputs\apk\debug\app-debug.apk`，配对并连接 HC-05。
3. 在 APK 点击 `Run Hardware Check` 或 `Run Acceptance`，再用 `Save Log` 保存日志。
4. 用逻辑分析仪采集 PCM5102A 的 `BCK=P4.1`、`LRCK=P4.2`、`DIN=P4.3`，导出 CSV。
5. 记录喇叭/耳机可听声音、EC11 旋转/按键、本地按键、接线照片到报告或 `docs\test_record.csv`。

## 3. 生成真实验收包

```powershell
powershell -ExecutionPolicy Bypass -File tools\run_real_board_acceptance.ps1 `
  -SerialLog path\to\phone_log.txt `
  -I2sCsv path\to\i2s_capture.csv
```

通过后输出：

```text
dist\real_board_acceptance\REAL_BOARD_SUMMARY.txt
dist\real_board_acceptance\hardware_evidence_report.md
dist\real_board_acceptance\real_serial_log.txt
dist\real_board_acceptance\real_i2s_capture.csv
dist\real_board_acceptance\SHA256SUMS.txt
```

只有 `mode=real` 且报告中 HC-05/APK serial transcript 与 PCM5102A I2S capture 均为 `PASS`，才可以作为板级验收的强证据。
