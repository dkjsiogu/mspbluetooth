"""Generate SVG diagrams for the course report.

The project requires a hardware block diagram and a software flowchart. This
script creates deterministic SVG files without external graphing tools, so the
diagrams can be regenerated on the classroom PC and included in the delivery
package.
"""

from __future__ import annotations

from pathlib import Path
from xml.sax.saxutils import escape


ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"
HARDWARE_SVG = DOCS / "hardware_block_diagram.svg"
SOFTWARE_SVG = DOCS / "software_flowchart.svg"


def svg_header(width: int, height: int) -> list[str]:
    return [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        '<defs><marker id="arrow" markerWidth="10" markerHeight="10" refX="8" refY="3" orient="auto" markerUnits="strokeWidth"><path d="M0,0 L0,6 L9,3 z" fill="#1f2937"/></marker></defs>',
        '<style>text{font-family:Arial,Helvetica,sans-serif;font-size:13px;fill:#111827}.title{font-size:18px;font-weight:bold}.box{fill:#f8fafc;stroke:#334155;stroke-width:1.5}.core{fill:#ecfeff;stroke:#0f766e;stroke-width:2}.note{font-size:12px;fill:#475569}.line{stroke:#1f2937;stroke-width:1.5;fill:none;marker-end:url(#arrow)}</style>',
    ]


def add_box(parts: list[str], x: int, y: int, width: int, height: int, title: str, lines: list[str], klass: str = "box") -> None:
    parts.append(f'<rect class="{klass}" x="{x}" y="{y}" width="{width}" height="{height}" rx="6"/>')
    parts.append(f'<text x="{x + 12}" y="{y + 22}" font-weight="bold">{escape(title)}</text>')
    for index, line in enumerate(lines):
        parts.append(f'<text class="note" x="{x + 12}" y="{y + 44 + index * 17}">{escape(line)}</text>')


def add_arrow(parts: list[str], x1: int, y1: int, x2: int, y2: int, label: str = "") -> None:
    parts.append(f'<path class="line" d="M{x1},{y1} L{x2},{y2}"/>')
    if label:
        mid_x = (x1 + x2) // 2
        mid_y = (y1 + y2) // 2 - 6
        parts.append(f'<text class="note" text-anchor="middle" x="{mid_x}" y="{mid_y}">{escape(label)}</text>')


def generate_hardware() -> str:
    parts = svg_header(980, 560)
    parts.append('<text class="title" x="30" y="34">MSP430F5529 蓝牙遥控音频播放器硬件框图</text>')

    add_box(parts, 390, 210, 210, 120, "MSP430F5529", ["16 MHz MCLK/SMCLK", "GPIO / UART / SPI / I2S", "Timer / interrupt"], "core")
    add_box(parts, 40, 70, 220, 95, "Android + HC-05", ["SPP 串口透传", "UCA1 P4.4/P4.5"])
    add_box(parts, 40, 220, 220, 95, "TF 卡座", ["FATFS + WAV 文件", "P3.1/P3.2/P3.3 + P4.0"])
    add_box(parts, 40, 370, 220, 95, "本地输入", ["EC11旋转/短按/长按", "S1/S2/S4 短按/长按"])
    add_box(parts, 720, 100, 220, 95, "PCM5102A DAC", ["软件 I2S", "BCK/LRCK/DIN=P4.1/4.2/4.3"])
    add_box(parts, 720, 250, 220, 95, "PAM8403 + 喇叭", ["DAC 模拟输出", "功放驱动无源喇叭"])
    add_box(parts, 720, 400, 220, 95, "3.5mm 耳机座 / 状态显示", ["耳机并联输出", "APK + 三行显示帧"])

    add_arrow(parts, 260, 118, 390, 240, "UART")
    add_arrow(parts, 260, 268, 390, 270, "SPI")
    add_arrow(parts, 260, 418, 390, 300, "GPIO")
    add_arrow(parts, 600, 250, 720, 148, "I2S")
    add_arrow(parts, 830, 195, 830, 250, "analog")
    add_arrow(parts, 600, 300, 720, 448, "status")

    parts.append('<text class="note" x="30" y="530">说明：课程资料中的 HC-05 P3.3/P3.4 与 TF MISO=P3.3 冲突，本工程默认使用 UCA1 P4.4/P4.5。</text>')
    parts.append("</svg>")
    return "\n".join(parts)


def generate_software() -> str:
    parts = svg_header(980, 620)
    parts.append('<text class="title" x="30" y="34">MSP430F5529 蓝牙遥控音频播放器软件流程图</text>')

    nodes = [
        (390, 60, "上电复位", ["board_init", "UART/按键/I2S 初始化"]),
        (390, 170, "播放器初始化", ["挂载 TF 卡", "扫描并打开首个 WAV"]),
        (90, 300, "输入轮询", ["HC-05 命令", "EC11 / S1/S2/S4"]),
        (390, 300, "状态机处理", ["播放/暂停/停止", "音量/静音/播放顺序"]),
        (690, 300, "音频服务", ["FatFs 读取 WAV", "软件 I2S 输出 DAC"]),
        (390, 470, "状态与诊断", ["status 自动上报", "display 三行帧 / link / 自检 / 测试音"]),
    ]
    for x, y, title, lines in nodes:
        add_box(parts, x, y, 210, 90, title, lines, "core" if title == "状态机处理" else "box")

    add_arrow(parts, 495, 150, 495, 170)
    add_arrow(parts, 495, 260, 495, 300)
    add_arrow(parts, 300, 345, 390, 345, "events")
    add_arrow(parts, 600, 345, 690, 345, "PLAYING")
    add_arrow(parts, 795, 390, 600, 505, "EOF/error")
    add_arrow(parts, 495, 390, 495, 470, "report")
    add_arrow(parts, 390, 515, 195, 390, "next poll")

    parts.append('<text class="note" x="30" y="585">验证：run_verification.ps1 覆盖编译、协议仿真、整板场景、按键长按、显示预览和报告生成。</text>')
    parts.append("</svg>")
    return "\n".join(parts)


def assert_svg(text: str, required: list[str]) -> None:
    if not text.startswith("<svg"):
        raise RuntimeError("SVG output missing root svg tag")
    for token in required:
        if token not in text:
            raise RuntimeError(f"SVG output missing token: {token}")


def main() -> int:
    DOCS.mkdir(parents=True, exist_ok=True)
    hardware = generate_hardware()
    software = generate_software()
    assert_svg(hardware, ["MSP430F5529", "HC-05", "PCM5102A", "TF"])
    assert_svg(software, ["上电复位", "状态机处理", "音频服务"])
    HARDWARE_SVG.write_text(hardware, encoding="utf-8", newline="\n")
    SOFTWARE_SVG.write_text(software, encoding="utf-8", newline="\n")
    print(f"diagram generated: {HARDWARE_SVG}")
    print(f"diagram generated: {SOFTWARE_SVG}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
