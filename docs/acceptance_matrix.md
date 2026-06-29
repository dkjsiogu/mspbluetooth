# 功能验收矩阵

| 要求 | 当前实现 | 验证证据 |
| --- | --- | --- |
| 手机蓝牙播放/暂停 | APK `Play/Pause` 发送 `p` | `tools/bluetooth_protocol_sim.py` |
| 手机蓝牙音量加减 | APK `Vol +`/`Vol -` 发送 `+`/`-` | `tools/bluetooth_protocol_sim.py` |
| 手机蓝牙上一曲/下一曲 | APK `Prev`/`Next` 发送 `b`/`n` | `tools/bluetooth_protocol_sim.py` |
| 手机蓝牙曲目选择 | APK `Track 1-9` 发送 `1-9` | APK 源码检查、协议仿真 |
| Android APK 命令覆盖 | APK 对固件全部蓝牙命令提供按钮，并声明 HC-05 SPP 所需蓝牙权限 | `tools/android_command_coverage.py`、`docs/android_command_coverage_report.md` |
| Android 连接后同步 | APK 连接成功后自动发送 `?`、`l`、`d`，首屏填充状态、曲目列表和显示帧 | `tools/android_command_coverage.py`、`tools/end_to_end_demo_sim.py` |
| 端到端演示效果 | APK 按钮发送 HC-05 命令，状态改变后立即收到 `status=...` 和 `display 1/2/3:...`，手机状态面板、显示帧和曲目列表随回包变化 | `tools/end_to_end_demo_sim.py`、`docs/end_to_end_demo_report.md` |
| 手机蓝牙曲目扫描 | APK `Track List` 发送 `l`，并把 `tracks ...` 回传解析到曲目状态面板 | `tools/bluetooth_protocol_sim.py`、`tools/android_ui_parser_sim.py` |
| 手机蓝牙播放顺序 | APK `Order` 发送 `o`，固件循环 sequence/repeat_all/repeat_one | `tools/bluetooth_protocol_sim.py`、`tools/board_scenario_sim.py` |
| 蓝牙诊断与异常输入 | `i/e/l/d/?` 形成现场自检转录，分片输入、大小写命令、换行和无效字符不破坏状态 | `tools/bluetooth_diagnostic_sim.py`、`docs/bluetooth_diagnostic_report.md` |
| EC11 音量和按键 | `drivers/encoder.*` 事件映射到音量、播放/暂停和长按停止，A/B 相位解码为 CW/CCW，按键短按/长按去抖，动作后立即推送状态和显示帧 | `tools/board_scenario_sim.py`、`tools/encoder_quadrature_sim.py` |
| 本地按键 | S1/S2/S4 短按映射播放/上一曲/下一曲，长按映射停止/静音/播放顺序，动作后立即推送状态和显示帧 | `tools/board_scenario_sim.py`、`tools/local_button_sim.py` |
| TF 卡 WAV | FatFs + `wav_reader` 支持 16-bit PCM | 固件编译、WAV 解析静态覆盖 |
| TF 卡测试音频 | `tools/prepare_sdcard_assets.py` 生成 TRACK01-03.WAV，并用 RIFF chunk 校验 | `tools/wav_asset_check.py`、`tools/run_verification.ps1` |
| PCM5102A 输出 | 软件 I2S 输出 16-bit stereo frame，32-bit 左/右声道 slot，标准 I2S MSB 延迟 | 固件编译、`t` 测试音命令、`tools/audio_stream_sim.py`、`tools/i2s_frame_sim.py` |
| 音频流效果 | TF WAV 按固件读块大小解码为非静音、音量缩放后的 stereo 样本，进度到 100% | `tools/audio_stream_sim.py`、`docs/audio_stream_report.md` |
| 状态显示 | APK 日志、含播放进度的状态面板、蓝牙/EC11/本地按键控制后即时状态和显示帧、自动状态、`d` 三行显示帧 | 协议仿真、APK 构建、APK 源码检查、`tools/android_ui_parser_sim.py`、`tools/end_to_end_demo_sim.py`、`tools/board_scenario_sim.py`、`docs/effect_acceptance_report.md` |
| 状态 LED | P1.0 表示播放器模式：播放快闪、暂停慢闪、停止常亮、错误双闪 | `tools/status_led_pattern_sim.py`、`docs/status_led_report.md` |
| 墨水屏扩展 | 未默认接入，保留显示模型、APK 显示帧、PGM 黑白预览和播放/暂停/停止/错误多状态画廊，并说明引脚冲突 | `tools/epaper_preview_sim.py`、`docs/epaper_gallery_report.md`、`docs/hardware_verification.md` |
| 硬件框图/软件流程图 | 自动生成 SVG 图并纳入报告和交付包 | `tools/generate_diagrams.py`、`docs/hardware_block_diagram.svg`、`docs/software_flowchart.svg` |
| 课程报告材料 | 自动生成课程设计报告初稿，保留实物验证待补项 | `tools/generate_course_report.py`、`docs/course_report_draft.md` |
| 软件效果验收报告 | 自动汇总蓝牙命令、Android 解析、整板仿真、按键时序、显示帧、音频流和 WAV 资产证据 | `tools/generate_effect_report.py`、`docs/effect_acceptance_report.md` |
| 注释规范 | 自写头文件、声明、define、static 注释 | `tools/firmware_static_check.py` |
| GitHub 仓库 | 已推送到 `dkjsiogu/mspbluetooth` | `git remote -v` / GitHub |
| Android acceptance summary | APK visible `Acceptance X/8` panel marks SD, info, selftest, tracks, display, status, tone, and WAV open evidence | `tools/android_acceptance_log_sim.py`, `docs/android_acceptance_script_report.md` |
| Bluetooth link counters | Firmware `k` command reports `link rx=... status=... display=... bad=... last=... uptime=...ms`, and APK renders it in the Link panel | `tools/bluetooth_protocol_sim.py`, `tools/android_ui_parser_sim.py`, `tools/end_to_end_demo_sim.py` |
| Local input counters | Firmware `u` command reports EC11 and S1/S2/S4 short/long event counters, and APK renders it in the Input panel | `tools/board_scenario_sim.py`, `tools/android_ui_parser_sim.py`, `tools/end_to_end_demo_sim.py` |
| Android log save | APK `Save Log` writes the phone TX/RX transcript as a text file through Android document picker | `tools/android_command_coverage.py`, `tools/android_acceptance_log_sim.py` |
