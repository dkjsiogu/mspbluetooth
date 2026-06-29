# 功能验收矩阵

| 要求 | 当前实现 | 验证证据 |
| --- | --- | --- |
| 手机蓝牙播放/暂停 | APK `Play/Pause` 发送 `p` | `tools/bluetooth_protocol_sim.py` |
| 手机蓝牙音量加减 | APK `Vol +`/`Vol -` 发送 `+`/`-` | `tools/bluetooth_protocol_sim.py` |
| 手机蓝牙上一曲/下一曲 | APK `Prev`/`Next` 发送 `b`/`n` | `tools/bluetooth_protocol_sim.py` |
| 手机蓝牙曲目选择 | APK `Track 1-9` 发送 `1-9` | APK 源码检查、协议仿真 |
| 手机蓝牙曲目扫描 | APK `Track List` 发送 `l` | `tools/bluetooth_protocol_sim.py` |
| 手机蓝牙播放顺序 | APK `Order` 发送 `o`，固件循环 sequence/repeat_all/repeat_one | `tools/bluetooth_protocol_sim.py`、`tools/board_scenario_sim.py` |
| EC11 音量和按键 | `drivers/encoder.*` 事件映射到音量和播放/暂停 | `tools/board_scenario_sim.py` |
| 本地按键 | S1/S2/S4 短按映射播放/上一曲/下一曲，长按映射停止/静音/播放顺序 | `tools/board_scenario_sim.py`、`tools/local_button_sim.py` |
| TF 卡 WAV | FatFs + `wav_reader` 支持 16-bit PCM | 固件编译、WAV 解析静态覆盖 |
| TF 卡测试音频 | `tools/prepare_sdcard_assets.py` 生成 TRACK01-03.WAV，并用 RIFF chunk 校验 | `tools/wav_asset_check.py`、`tools/run_verification.ps1` |
| PCM5102A 输出 | 软件 I2S 输出 16-bit stereo frame | 固件编译、`t` 测试音命令 |
| 状态显示 | APK 日志、含播放进度的状态面板、自动状态、`d` 三行显示帧 | 协议仿真、APK 构建、APK 源码检查、`docs/effect_acceptance_report.md` |
| 墨水屏扩展 | 未默认接入，保留显示模型、APK 显示帧、PGM 黑白预览并说明引脚冲突 | `tools/epaper_preview_sim.py`、`docs/hardware_verification.md` |
| 硬件框图/软件流程图 | 自动生成 SVG 图并纳入报告和交付包 | `tools/generate_diagrams.py`、`docs/hardware_block_diagram.svg`、`docs/software_flowchart.svg` |
| 课程报告材料 | 自动生成课程设计报告初稿，保留实物验证待补项 | `tools/generate_course_report.py`、`docs/course_report_draft.md` |
| 软件效果验收报告 | 自动汇总蓝牙命令、整板仿真、按键时序、显示帧、APK 解析和 WAV 资产证据 | `tools/generate_effect_report.py`、`docs/effect_acceptance_report.md` |
| 注释规范 | 自写头文件、声明、define、static 注释 | `tools/firmware_static_check.py` |
| GitHub 仓库 | 已推送到 `dkjsiogu/mspbluetooth` | `git remote -v` / GitHub |
