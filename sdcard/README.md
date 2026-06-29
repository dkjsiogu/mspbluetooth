# TF 卡测试文件

运行脚本生成可直接复制到 TF 卡根目录的测试 WAV：

```powershell
cd E:\code\ccs\mspbluetooth
python tools\prepare_sdcard_assets.py
```

生成后可校验 WAV 头和数据格式：

```powershell
python tools\wav_asset_check.py --report dist\verification\wav_asset_report.md
```

默认生成：

```text
sdcard\TRACK01.WAV
sdcard\TRACK02.WAV
sdcard\TRACK03.WAV
```

这些文件是 16-bit PCM、16 kHz、双声道 WAV，适合当前固件的 WAV 解析和软件 I2S 输出链路。
