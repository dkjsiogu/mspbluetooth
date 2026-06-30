"""Generate the software-effect report for the environment monitor."""

from __future__ import annotations

import argparse
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REPORT = ROOT / "docs" / "effect_acceptance_report.md"


EVIDENCE_FILES = [
    ("Firmware static check", "tools/firmware_static_check.py", "Debug build, active modules, protocol tokens, pin map, RAM margin"),
    ("Readability audit", "tools/readability_audit.py", "Chinese module comments, documented macros, public declarations, top-level static intent"),
    ("Bluetooth protocol", "docs/bluetooth_protocol_report.md", "DATA, threshold, history, clear-log, wiring, info, trace"),
    ("Board scenarios", "docs/board_scenario_report.md", "Sensor values to OLED, Bluetooth DATA, and alarm level"),
    ("Flash log", "docs/flash_log_report.md", "Information Flash append, erase-on-full, bounds, CRC"),
    ("Encoder threshold", "docs/encoder_threshold_report.md", "EC11 and Bluetooth threshold adjustment use the same 0.5 C step"),
]


def row(columns: list[object]) -> str:
    return "| " + " | ".join(str(column).replace("|", "\\|") for column in columns) + " |"


def require_files() -> None:
    for _, relative, _ in EVIDENCE_FILES:
        path = ROOT / relative
        if not path.exists():
            raise FileNotFoundError(f"missing evidence file: {relative}")


def render_report() -> str:
    require_files()
    lines = [
        "# Software Effect Acceptance Report",
        "",
        "This report summarizes host-side verification for the MSP430F5529 environment monitor. It proves the software-visible behavior before physical flashing; real sensor accuracy, HC-05 pairing, OLED electrical timing, buzzer loudness, and Flash endurance still require board testing.",
        "",
        "## Verified Effects",
        "",
        row(["Effect", "Evidence", "Coverage"]),
        row(["---", "---", "---"]),
    ]
    for effect, relative, coverage in EVIDENCE_FILES:
        lines.append(row([effect, relative, coverage]))

    lines.extend(
        [
            "",
            "## Functional Coverage",
            "",
            row(["Requirement", "Software evidence"]),
            row(["---", "---"]),
            row(["DHT11 temperature/humidity", "`DATA T=... H=...` and OLED line `T:... H:...` in board scenarios"]),
            row(["MQ-2 gas concentration", "`GAS=... L=...` with level and alarm escalation"]),
            row(["HC-SR04 distance", "`D=...` and OLED `D:...MM`, including timeout display"]),
            row(["OLED display", "Four bounded 21-character display lines checked in `board_scenario_sim.py`"]),
            row(["Bluetooth upload/control", "Protocol simulation covers `?`, `T+`, `T-`, `SETT=`, `HIST`, `DUMP`, `CLRLOG`, `i`, `w`, `h`, `x`"]),
            row(["Internal Flash history", "Append, read, capacity wrap, clear, and CRC corruption behavior checked"]),
            row(["Buzzer alarm", "Alarm level model verifies higher temperature/gas produces higher level for faster beep service"]),
            row(["Threshold adjustment", "EC11 clockwise/counterclockwise and Bluetooth commands share the same threshold step"]),
            "",
            "## Remaining Physical Checks",
            "",
            "- Flash `Debug/mspbluetooth.out` to MSP430F5529 only after hardware is wired and powered safely.",
            "- Confirm DHT11 one-wire timing on P1.0 and HC-SR04 Echo level conversion to 3.3 V on P1.3.",
            "- Confirm MQ-2 analog output does not exceed MSP430 ADC input limits.",
            "- Confirm OLED I2C address and pull-up resistors on P3.0/P3.1.",
            "- Confirm HC-05 UCA1 wiring P4.4/P4.5 and phone-side SPP log.",
            "- Confirm buzzer rhythm changes as temperature/gas alarm level rises.",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=DEFAULT_REPORT)
    args = parser.parse_args()
    text = render_report()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(text, encoding="utf-8", newline="\n")
    print(f"software effect report generated: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
