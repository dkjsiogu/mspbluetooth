"""Static checks for the MSP430F5529 environment monitor firmware.

The checker guards the active course-design baseline: DHT11, MQ-2, HC-SR04,
OLED display, Bluetooth telemetry, internal Flash history, buzzer alarm, and
EC11 threshold adjustment. It rejects legacy audio-player objects if they
accidentally re-enter the active build.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

ACTIVE_FILES = [
    "main.c",
    "application/env_monitor.c",
    "application/env_monitor.h",
    "drivers/alarm_buzzer.c",
    "drivers/alarm_buzzer.h",
    "drivers/bluetooth_uart.c",
    "drivers/bluetooth_uart.h",
    "drivers/board.c",
    "drivers/board.h",
    "drivers/board_pins.h",
    "drivers/encoder.c",
    "drivers/encoder.h",
    "drivers/env_sensors.c",
    "drivers/env_sensors.h",
    "drivers/flash_log.c",
    "drivers/flash_log.h",
    "drivers/oled_ssd1306.c",
    "drivers/oled_ssd1306.h",
    "drivers/platform_config.h",
    "Debug/mspbluetooth.out",
]

REMOVED_LEGACY_PATHS = [
    "application/audio_player.c",
    "application/audio_player.h",
    "drivers/i2s_dac.c",
    "drivers/i2s_dac.h",
    "drivers/local_buttons.c",
    "drivers/local_buttons.h",
    "middleware",
    "fatfs",
    "sdcard",
]

HEADER_FILES = [
    "application/env_monitor.h",
    "drivers/alarm_buzzer.h",
    "drivers/bluetooth_uart.h",
    "drivers/board.h",
    "drivers/board_pins.h",
    "drivers/encoder.h",
    "drivers/env_sensors.h",
    "drivers/flash_log.h",
    "drivers/oled_ssd1306.h",
    "drivers/platform_config.h",
]

SOURCE_FILES = [
    "main.c",
    "application/env_monitor.c",
    "drivers/alarm_buzzer.c",
    "drivers/bluetooth_uart.c",
    "drivers/board.c",
    "drivers/encoder.c",
    "drivers/env_sensors.c",
    "drivers/flash_log.c",
    "drivers/oled_ssd1306.c",
]

REQUIRED_COMMAND_TOKENS = {
    "?": "real-time DATA refresh",
    "i": "firmware information",
    "w": "wiring diagnostics",
    "h": "help text",
    "x": "recent operation trace",
    "T+": "temperature threshold up",
    "T-": "temperature threshold down",
    "SETT=": "set threshold directly",
    "HIST?": "history count",
    "HIST ": "read one history record",
    "DUMP": "dump all history records",
    "CLRLOG": "clear Flash history",
}

REQUIRED_PIN_TOKENS = [
    "DHT11_BIT                       BIT0",
    "MQ2_BIT                         BIT0",
    "HCSR04_TRIG_BIT                 BIT2",
    "HCSR04_ECHO_BIT                 BIT3",
    "OLED_SDA_BIT                    BIT0",
    "OLED_SCL_BIT                    BIT1",
    "BUZZER_BIT                      BIT0",
    "ENC_A_BIT                       BIT1",
    "ENC_B_BIT                       BIT2",
    "ENC_SW_BIT                      BIT3",
    "BT_UCA1_TX_BIT                  BIT4",
    "BT_UCA1_RX_BIT                  BIT5",
    "STATUS_LED_BIT                  BIT7",
]

REQUIRED_BUILD_OBJECTS = [
    "main.obj",
    "board.obj",
    "bluetooth_uart.obj",
    "encoder.obj",
    "alarm_buzzer.obj",
    "env_sensors.obj",
    "flash_log.obj",
    "oled_ssd1306.obj",
    "env_monitor.obj",
]

FORBIDDEN_BUILD_OBJECTS = [
    "audio_player.obj",
    "i2s_dac.obj",
    "local_buttons.obj",
    "wav_reader.obj",
    "display_model.obj",
    "HAL_SDCard.obj",
]


def fail(message: str) -> None:
    print(f"FAIL: {message}")
    sys.exit(1)


def read_text(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8")


def is_comment_line(line: str) -> bool:
    stripped = line.strip()
    return stripped.startswith("/*") or stripped.startswith("*") or stripped.startswith("//") or stripped.endswith("*/")


def previous_nonblank_line(lines: list[str], index: int) -> str:
    previous_index = index - 1
    while previous_index >= 0 and lines[previous_index].strip() == "":
        previous_index -= 1
    if previous_index < 0:
        return ""
    return lines[previous_index]


def comment_block_before(lines: list[str], index: int) -> str:
    previous_index = index - 1
    while previous_index >= 0 and lines[previous_index].strip() == "":
        previous_index -= 1
    if previous_index < 0 or not is_comment_line(lines[previous_index]):
        return ""

    block: list[str] = []
    while previous_index >= 0 and is_comment_line(lines[previous_index]):
        block.append(lines[previous_index].strip())
        previous_index -= 1
    block.reverse()
    return "\n".join(block)


def declaration_parameter_names(declaration: str) -> list[str]:
    match = re.search(r"\((.*)\)", declaration)
    if not match:
        return []

    raw_params = match.group(1).strip()
    if raw_params == "" or raw_params == "void":
        return []

    names: list[str] = []
    for raw_param in raw_params.split(","):
        cleaned = raw_param.strip()
        if cleaned == "...":
            continue
        name_match = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?$", cleaned)
        if name_match:
            names.append(name_match.group(1))
    return names


def check_active_files() -> None:
    for relative in ACTIVE_FILES:
        path = ROOT / relative
        if not path.exists():
            fail(f"missing active firmware file: {relative}")
        if path.is_file() and path.stat().st_size == 0:
            fail(f"empty active firmware file: {relative}")

    for relative in REMOVED_LEGACY_PATHS:
        if (ROOT / relative).exists():
            fail(f"legacy audio-player path must not exist in active project: {relative}")


def check_header_comments() -> None:
    declaration_pattern = r"^(?:[A-Za-z_][\w\s\*]+\s+)?[A-Za-z_]\w+\([^;{}]*\);"

    for relative in HEADER_FILES:
        text = read_text(relative)
        lines = text.splitlines()
        if not text.lstrip().startswith("/*"):
            fail(f"{relative} must start with a module comment")

        for match in re.finditer(r"^#define\s+([A-Za-z_]\w+)\b.*$", text, re.M):
            name = match.group(1)
            if name.endswith("_H"):
                continue
            line_index = text[: match.start()].count("\n")
            if not is_comment_line(previous_nonblank_line(lines, line_index)):
                fail(f"{relative} #define lacks comment: {name}")

        for match in re.finditer(declaration_pattern, text, re.M):
            declaration = match.group(0).strip()
            line_index = text[: match.start()].count("\n")
            comment = comment_block_before(lines, line_index)
            if comment == "":
                fail(f"{relative} declaration lacks comment: {declaration}")
            for param_name in declaration_parameter_names(declaration):
                if re.search(rf"\b{re.escape(param_name)}\s*:", comment) is None:
                    fail(f"{relative} declaration comment must describe parameter {param_name}: {declaration}")


def check_source_comments() -> None:
    for relative in SOURCE_FILES:
        text = read_text(relative)
        if not text.lstrip().startswith("/*"):
            fail(f"{relative} must start with a module comment")

        lines = text.splitlines()
        for index, line in enumerate(lines):
            if re.match(r"^static\s+", line) and not is_comment_line(previous_nonblank_line(lines, index)):
                fail(f"{relative} static item lacks comment near line {index + 1}: {line.strip()}")


def check_protocol() -> None:
    app = read_text("application/env_monitor.c")
    for token, meaning in REQUIRED_COMMAND_TOKENS.items():
        if token not in app:
            fail(f"env_monitor.c missing command token {token!r} ({meaning})")

    for token in ["DATA T=", "GAS=", "D=", "TH=", "ALM=", "HIST COUNT=", "REC ", "LOG CLEARED"]:
        if token not in app:
            fail(f"env_monitor.c missing Bluetooth output token: {token}")


def check_pins_and_config() -> None:
    pins = read_text("drivers/board_pins.h")
    config = read_text("drivers/platform_config.h")

    for token in REQUIRED_PIN_TOKENS:
        if token not in pins:
            fail(f"board_pins.h missing expected pin token: {token}")

    for token in [
        'ENV_FIRMWARE_NAME               "MSP430F5529-ENV-MON"',
        "ENV_BT_UART_MODE                ENV_BT_UART_UCA1_P45",
        "ENV_SAMPLE_PERIOD_MS",
        "ENV_FLASH_LOG_MS",
        "ENV_DEFAULT_TEMP_THRESHOLD_X10",
        "ENV_GAS_WARN_ADC",
        "ENV_TRACE_DEPTH",
    ]:
        if token not in config:
            fail(f"platform_config.h missing environment config token: {token}")


def check_makefile_objects() -> None:
    makefile = read_text("Debug/makefile")
    for obj in REQUIRED_BUILD_OBJECTS:
        if obj not in makefile:
            fail(f"Debug/makefile missing active object: {obj}")
    for obj in FORBIDDEN_BUILD_OBJECTS:
        if obj in makefile:
            fail(f"Debug/makefile still builds old audio object: {obj}")


def check_build_map() -> None:
    map_path = ROOT / "Debug/mspbluetooth.map"
    if not map_path.exists():
        fail("missing Debug/mspbluetooth.map; run gmake all first")

    text = map_path.read_text(encoding="utf-8", errors="ignore")
    ram_line = next((line for line in text.splitlines() if line.strip().startswith("RAM")), "")
    if not ram_line:
        fail("map file does not contain RAM usage line")
    parts = ram_line.split()
    if len(parts) < 5:
        fail(f"cannot parse RAM usage line: {ram_line}")
    used = int(parts[3], 16)
    available = int(parts[4], 16)
    if available <= 0:
        fail("RAM free space must stay positive")
    print(f"RAM used=0x{used:04x}, free=0x{available:04x}")


def main() -> int:
    check_active_files()
    check_header_comments()
    check_source_comments()
    check_protocol()
    check_pins_and_config()
    check_makefile_objects()
    check_build_map()
    print("environment firmware static checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
