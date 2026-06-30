"""Generate SVG diagrams for the MSP430F5529 environment monitor."""

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
        '<style>text{font-family:Arial,Helvetica,sans-serif;font-size:13px;fill:#111827}.title{font-size:18px;font-weight:bold}.box{fill:#f8fafc;stroke:#334155;stroke-width:1.5}.core{fill:#ecfeff;stroke:#0f766e;stroke-width:2}.warn{fill:#fff7ed;stroke:#c2410c;stroke-width:1.5}.note{font-size:12px;fill:#475569}.line{stroke:#1f2937;stroke-width:1.5;fill:none;marker-end:url(#arrow)}</style>',
    ]


def add_box(parts: list[str], x: int, y: int, width: int, height: int, title: str, lines: list[str], klass: str = "box") -> None:
    parts.append(f'<rect class="{klass}" x="{x}" y="{y}" width="{width}" height="{height}" rx="6"/>')
    parts.append(f'<text x="{x + 12}" y="{y + 22}" font-weight="bold">{escape(title)}</text>')
    for index, line in enumerate(lines):
        parts.append(f'<text class="note" x="{x + 12}" y="{y + 44 + index * 17}">{escape(line)}</text>')


def add_arrow(parts: list[str], x1: int, y1: int, x2: int, y2: int, label: str = "") -> None:
    parts.append(f'<path class="line" d="M{x1},{y1} L{x2},{y2}"/>')
    if label:
        parts.append(f'<text class="note" text-anchor="middle" x="{(x1 + x2) // 2}" y="{(y1 + y2) // 2 - 6}">{escape(label)}</text>')


def generate_hardware() -> str:
    parts = svg_header(1040, 600)
    parts.append('<text class="title" x="30" y="34">MSP430F5529 多点温度/环境监测系统硬件框图</text>')

    add_box(parts, 410, 220, 220, 130, "MSP430F5529", ["GPIO / ADC12 / TimerA", "UART UCA1", "内部 Information Flash", "1 ms tick scheduler"], "core")
    add_box(parts, 40, 70, 230, 85, "DHT11 温湿度", ["DATA=P1.0", "单总线 40 bit"])
    add_box(parts, 40, 205, 230, 85, "MQ-2 可燃气体", ["AO=P6.0 / ADC12_A0", "注意模拟电压限幅"], "warn")
    add_box(parts, 40, 340, 230, 95, "HC-SR04 超声波", ["Trig=P1.2", "Echo=P1.3", "Echo 需分压到 3.3 V"], "warn")
    add_box(parts, 40, 480, 230, 75, "EC11 编码器", ["A=P2.1 B=P2.2 SW=P2.3", "调节温度报警阈值"])
    add_box(parts, 770, 80, 230, 85, "0.96 OLED SSD1306", ["SDA=P3.0 SCL=P3.1", "软件 I2C 四行显示"])
    add_box(parts, 770, 225, 230, 85, "HC-05 蓝牙", ["TX/RX=P4.5/P4.4", "手机 SPP 数据/命令"])
    add_box(parts, 770, 365, 230, 75, "蜂鸣器报警", ["GPIO=P2.0", "等级越高节奏越急"])
    add_box(parts, 770, 490, 230, 75, "内部 Flash 历史", ["Info B/C/D", "记录可独立读出"])

    add_arrow(parts, 270, 112, 410, 250, "1-Wire")
    add_arrow(parts, 270, 248, 410, 270, "ADC")
    add_arrow(parts, 270, 388, 410, 292, "GPIO timing")
    add_arrow(parts, 270, 518, 410, 330, "EC11")
    add_arrow(parts, 630, 252, 770, 122, "I2C")
    add_arrow(parts, 630, 276, 770, 268, "UART")
    add_arrow(parts, 630, 315, 770, 402, "alarm")
    add_arrow(parts, 630, 340, 770, 528, "log")

    parts.append('<text class="note" x="30" y="580">说明：HC-SR04 和 MQ-2 常见模块为 5 V 供电，接入 MSP430 输入/ADC 前必须确认 3.3 V 电平兼容或使用分压/限流。</text>')
    parts.append("</svg>")
    return "\n".join(parts)


def generate_software() -> str:
    parts = svg_header(1040, 640)
    parts.append('<text class="title" x="30" y="34">MSP430F5529 多点温度/环境监测系统软件流程图</text>')

    nodes = [
        (410, 60, "上电初始化", ["时钟/GPIO/tick", "UART/EC11/传感器/OLED/Flash"]),
        (80, 200, "周期采样", ["DHT11 温湿度", "MQ-2 ADC 平均", "HC-SR04 距离"]),
        (410, 200, "应用调度", ["报警等级计算", "阈值/命令/历史记录"],),
        (740, 200, "显示与上报", ["OLED 四行刷新", "DATA 蓝牙主动上传"]),
        (80, 390, "蓝牙命令", ["? i w h x", "T+ T- SETT=", "HIST/DUMP/CLRLOG"]),
        (410, 390, "本地控制", ["EC11 旋转调阈值", "按键立即刷新/保存提示"]),
        (740, 390, "报警与存储", ["蜂鸣器节奏服务", "Flash 定时写入"]),
    ]
    for x, y, title, lines in nodes:
        add_box(parts, x, y, 230, 100, title, lines, "core" if title == "应用调度" else "box")

    add_arrow(parts, 525, 160, 525, 200)
    add_arrow(parts, 310, 250, 410, 250, "sample")
    add_arrow(parts, 640, 250, 740, 250, "state")
    add_arrow(parts, 310, 440, 410, 440, "command")
    add_arrow(parts, 525, 300, 525, 390, "threshold")
    add_arrow(parts, 640, 440, 740, 440, "alarm/log")
    add_arrow(parts, 855, 300, 855, 390, "periodic")
    add_arrow(parts, 740, 250, 525, 60, "loop")

    parts.append('<text class="note" x="30" y="610">验证：run_verification.ps1 覆盖 clean build、静态检查、可读性审计、协议仿真、整板场景、Flash 日志和阈值调整。</text>')
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
    assert_svg(hardware, ["DHT11", "MQ-2", "HC-SR04", "OLED", "HC-05", "Flash"])
    assert_svg(software, ["周期采样", "蓝牙命令", "报警与存储"])
    HARDWARE_SVG.write_text(hardware, encoding="utf-8", newline="\n")
    SOFTWARE_SVG.write_text(software, encoding="utf-8", newline="\n")
    print(f"diagram generated: {HARDWARE_SVG}")
    print(f"diagram generated: {SOFTWARE_SVG}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
