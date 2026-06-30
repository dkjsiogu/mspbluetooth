"""Bluetooth protocol simulation for the environment monitor firmware."""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "docs" / "bluetooth_protocol_report.md"
TRACE_DEPTH = 6


def config_string(name: str) -> str:
    text = (ROOT / "drivers" / "platform_config.h").read_text(encoding="utf-8")
    match = re.search(rf"{name}\s+\"([^\"]+)\"", text)
    if not match:
        raise RuntimeError(f"{name} not found")
    return match.group(1)


FIRMWARE_NAME = config_string("ENV_FIRMWARE_NAME")
FIRMWARE_VERSION = config_string("ENV_FIRMWARE_VERSION")
HARDWARE_PROFILE = config_string("ENV_HARDWARE_PROFILE")


@dataclass
class Sample:
    temperature_x10: int = 263
    humidity_x10: int = 580
    gas_adc: int = 1250
    gas_level: int = 0
    distance_mm: int = 342


@dataclass
class Record:
    seq: int
    uptime_s: int
    sample: Sample
    threshold_x10: int
    alarm_level: int


@dataclass
class SimulatedEnvMonitor:
    sample: Sample = field(default_factory=Sample)
    threshold_x10: int = 300
    alarm_level: int = 0
    history: list[Record] = field(default_factory=list)
    trace: list[str] = field(default_factory=list)
    transcript: list[str] = field(default_factory=list)

    def __post_init__(self) -> None:
        self.history.append(Record(1, 10, Sample(250, 550, 1200, 0, 410), self.threshold_x10, 0))
        self.history.append(Record(2, 20, Sample(321, 610, 2300, 2, 280), self.threshold_x10, 2))

    def trace_add(self, text: str) -> None:
        self.trace.insert(0, text[:7])
        self.trace = self.trace[:TRACE_DEPTH]

    @staticmethod
    def x10(value: int) -> str:
        sign = "-" if value < 0 else ""
        value = abs(value)
        return f"{sign}{value // 10}.{value % 10}"

    def data_line(self) -> str:
        return (
            f"DATA T={self.x10(self.sample.temperature_x10)} "
            f"H={self.sample.humidity_x10 // 10} GAS={self.sample.gas_adc} "
            f"L={self.sample.gas_level} D={self.sample.distance_mm} "
            f"TH={self.x10(self.threshold_x10)} ALM={self.alarm_level}"
        )

    def info_line(self) -> str:
        return f"info name={FIRMWARE_NAME} version={FIRMWARE_VERSION} profile={HARDWARE_PROFILE}"

    def wiring_lines(self) -> list[str]:
        return [
            "pin dht11 data=P1.0",
            "pin mq2 ao=P6.0 adc=A0",
            "pin hcsr04 trig=P1.2 echo=P1.3 note=echo-divide-to-3v3",
            "pin oled scl=P3.1 sda=P3.0 mode=soft-i2c",
            "pin bt tx=P4.4 rx=P4.5 mode=UCA1",
            "pin buzzer pwm=P2.0",
            "pin ec11 a=P2.1 b=P2.2 sw=P2.3",
        ]

    def help_line(self) -> str:
        return "cmd ? i w x T+ T- SETT=32.0 HIST? HIST n DUMP CLRLOG"

    def history_line(self, record: Record) -> str:
        sample = record.sample
        return (
            f"REC {record.seq} U={record.uptime_s} T={self.x10(sample.temperature_x10)} "
            f"H={sample.humidity_x10 // 10} GAS={sample.gas_adc} D={sample.distance_mm} "
            f"TH={self.x10(record.threshold_x10)} ALM={record.alarm_level}"
        )

    def send(self, command: str) -> None:
        command = command.strip()
        if command == "?":
            self.trace_add("bt:?")
            self.transcript.append(self.data_line())
        elif command.lower() == "i":
            self.trace_add("bt:info")
            self.transcript.append(self.info_line())
        elif command.lower() == "w":
            self.trace_add("bt:wire")
            self.transcript.extend(self.wiring_lines())
        elif command.lower() == "h":
            self.transcript.append(self.help_line())
        elif command.lower() == "x":
            self.trace_add("bt:trace")
            items = " ".join(f"{index + 1}={value}" for index, value in enumerate(self.trace))
            self.transcript.append(f"trace count={len(self.trace)} {items}".rstrip())
        elif command == "T+":
            self.trace_add("bt:T+")
            self.threshold_x10 += 5
            self.transcript.append(f"TH={self.x10(self.threshold_x10)}")
        elif command == "T-":
            self.trace_add("bt:T-")
            self.threshold_x10 -= 5
            self.transcript.append(f"TH={self.x10(self.threshold_x10)}")
        elif command.startswith("SETT="):
            self.trace_add("bt:SET")
            self.threshold_x10 = int(round(float(command[5:]) * 10))
            self.transcript.append(f"TH={self.x10(self.threshold_x10)} SAVED")
        elif command == "HIST?":
            self.trace_add("bt:H?")
            self.transcript.append(f"HIST COUNT={len(self.history)}")
        elif command.startswith("HIST "):
            self.trace_add("bt:H")
            index = int(command[5:])
            if 1 <= index <= len(self.history):
                self.transcript.append(self.history_line(self.history[index - 1]))
            else:
                self.transcript.append("ERR HIST")
        elif command == "DUMP":
            self.trace_add("bt:DUMP")
            self.transcript.extend(self.history_line(record) for record in self.history)
            self.transcript.append("DUMP END")
        elif command == "CLRLOG":
            self.trace_add("bt:CLR")
            self.history.clear()
            self.transcript.append("LOG CLEARED")
        else:
            self.transcript.append("ERR CMD")


def row(columns: list[object]) -> str:
    return "| " + " | ".join(str(column).replace("|", "\\|") for column in columns) + " |"


def assert_contains(lines: list[str], prefix: str) -> None:
    if not any(line.startswith(prefix) for line in lines):
        raise AssertionError(f"missing line prefix: {prefix}")


def render_report(commands: list[str], transcript: list[str]) -> str:
    return "\n".join(
        [
            "# Bluetooth Protocol Simulation",
            "",
            "Generated by `tools/bluetooth_protocol_sim.py` for the environment monitor protocol.",
            "",
            row(["Command", "Purpose"]),
            row(["---", "---"]),
            row(["?", "push one `DATA` line with T/H/GAS/distance/threshold/alarm"]),
            row(["T+ / T-", "adjust temperature alarm threshold by 0.5 C"]),
            row(["SETT=32.0", "set and save threshold directly"]),
            row(["HIST? / HIST n / DUMP / CLRLOG", "query, read, dump, and clear internal Flash history"]),
            row(["i / w / h / x", "firmware info, wiring, help, and recent trace"]),
            "",
            "## Simulated Commands",
            "",
            "`" + "`, `".join(commands) + "`",
            "",
            "## Transcript",
            "",
            "```text",
            *transcript,
            "```",
            "",
        ]
    )


def main() -> int:
    monitor = SimulatedEnvMonitor()
    commands = ["?", "T+", "T-", "SETT=32.0", "HIST?", "HIST 1", "DUMP", "CLRLOG", "HIST?", "i", "w", "h", "x"]
    for command in commands:
        monitor.send(command)

    required_prefixes = [
        "DATA T=",
        "TH=30.5",
        "TH=30.0",
        "TH=32.0 SAVED",
        "HIST COUNT=2",
        "REC 1",
        "DUMP END",
        "LOG CLEARED",
        "HIST COUNT=0",
        "info name=MSP430F5529-ENV-MON",
        "pin dht11",
        "cmd ? i w x",
        "trace count=",
    ]
    for prefix in required_prefixes:
        assert_contains(monitor.transcript, prefix)

    REPORT.write_text(render_report(commands, monitor.transcript), encoding="utf-8")
    print("bluetooth protocol simulation passed")
    print(f"report generated: {REPORT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
