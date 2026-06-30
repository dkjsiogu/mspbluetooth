# 功能验收矩阵

| 要求 | 当前实现 | 验证证据 |
| --- | --- | --- |
| DHT11 温湿度采集 | `drivers/env_sensors.*` 读取 P1.0 单总线并输出 0.1 单位温湿度 | CCS 编译、`tools/firmware_static_check.py` |
| MQ-2 气体采集 | P6.0/ADC12_A0 多次采样，换算 0~5 气体等级 | `tools/board_scenario_sim.py` |
| HC-SR04 距离采集 | P1.2 Trig、P1.3 Echo 软件微秒测距，超时显示 `----` | `tools/board_scenario_sim.py` |
| OLED 显示 | SSD1306 软件 I2C 四行显示温湿度、气体、距离、阈值和报警等级 | `docs/board_scenario_report.md` |
| 蓝牙实时上传 | HC-05 UCA1 输出 `DATA T=... H=... GAS=... D=...` | `tools/bluetooth_protocol_sim.py` |
| 蓝牙远程控制 | 支持 `? i w h x T+ T- SETT= HIST? HIST n DUMP CLRLOG` | `docs/bluetooth_protocol_report.md` |
| 内部 Flash 历史 | Info B/C/D 追加固定记录，支持数量、读取、DUMP 和清空 | `tools/flash_log_sim.py` |
| 蜂鸣器报警 | 温度超限和气体等级共同决定 0~5 报警等级，等级越高节奏越急 | `tools/board_scenario_sim.py` |
| EC11 调阈值 | CW/CCW 以 0.5°C 步进调整温度阈值，按键触发刷新 | `tools/encoder_threshold_sim.py` |
| Android 手机端 | APK 显示实时数据、阈值、历史、接线、轨迹，支持 Demo RX 和日志保存 | `tools/verify_android_apk.ps1` |
| 可读性 | 当前项目头文件、宏、声明参数和顶层 static 均有说明 | `tools/readability_audit.py` |
| 课程材料 | 自动生成硬件框图、软件流程图、课程报告和软件效果报告 | `tools/run_verification.ps1` |
| 交付打包 | 固件、APK、源码、文档、验证脚本和校验和统一打包 | `tools/package_release.ps1` |
| 实物安全 | 文档明确 HC-SR04 Echo 与 MQ-2 AO 的 5V 风险 | `README.md`、`docs/course_report_draft.md` |
