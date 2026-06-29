"""Static project checks for the MSP430 Bluetooth WAV player.

The checks focus on requirements that are easy to regress while editing:
header documentation, command coverage, known pin conflicts, and successful
build artifacts. It is intentionally lightweight so it can run on the classroom
PC without extra Python packages.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


REQUIRED_FILES = [
    "main.c",
    "application/audio_player.c",
    "drivers/bluetooth_uart.c",
    "drivers/encoder.c",
    "drivers/local_buttons.c",
    "drivers/i2s_dac.c",
    "middleware/display_model.c",
    "middleware/wav_reader.c",
    "fatfs/HAL_SDCard.c",
    "Debug/mspbluetooth.out",
]


REQUIRED_COMMANDS = {
    "p": "play/pause",
    "s": "stop",
    "r": "replay",
    "n": "next",
    "b": "previous",
    "+": "volume up",
    "-": "volume down",
    "m": "mute",
    "o": "play order",
    "t": "tone test",
    "i": "info",
    "e": "selftest",
    "l": "track list",
    "d": "display frame",
    "?": "status",
}


HEADER_FILES = [
    "application/audio_player.h",
    "drivers/bluetooth_uart.h",
    "drivers/board.h",
    "drivers/board_pins.h",
    "drivers/encoder.h",
    "drivers/i2s_dac.h",
    "drivers/local_buttons.h",
    "drivers/platform_config.h",
    "fatfs/HAL_SDCard.h",
    "middleware/display_model.h",
    "middleware/wav_reader.h",
]


def fail(message: str) -> None:
    print(f"FAIL: {message}")
    sys.exit(1)


def read_text(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8")


def check_required_files() -> None:
    for relative in REQUIRED_FILES:
        path = ROOT / relative
        if not path.exists():
            fail(f"missing required file: {relative}")
        if path.is_file() and path.stat().st_size == 0:
            fail(f"empty required file: {relative}")


def check_header_comments() -> None:
    for relative in HEADER_FILES:
        text = read_text(relative)
        stripped = text.lstrip()
        if not stripped.startswith("/*"):
            fail(f"{relative} must start with a module comment")

        declarations = re.findall(r"^(?:[A-Za-z_][\\w\\s\\*]+\\s+)?[A-Za-z_]\\w+\\([^;{}]*\\);", text, re.M)
        for declaration in declarations:
            start = text.find(declaration)
            previous = text[:start].rstrip().splitlines()[-1] if text[:start].rstrip().splitlines() else ""
            if "/*" not in previous:
                fail(f"{relative} declaration lacks comment: {declaration.strip()}")

        defines = re.findall(r"^#define\\s+([A-Za-z_]\\w+)", text, re.M)
        for name in defines:
            if name.endswith("_H"):
                continue
            define_line = re.search(rf"^#define\\s+{name}\\b.*$", text, re.M)
            if not define_line:
                continue
            start = define_line.start()
            previous = text[:start].rstrip().splitlines()[-1] if text[:start].rstrip().splitlines() else ""
            if "/*" not in previous:
                fail(f"{relative} #define lacks comment: {name}")


def check_commands() -> None:
    audio = read_text("application/audio_player.c")
    readme = read_text("README.md")

    for command, meaning in REQUIRED_COMMANDS.items():
        escaped = re.escape(command)
        if f"case '{command}'" not in audio and f"case \"{command}\"" not in audio:
            fail(f"audio_player.c missing command {command!r} ({meaning})")
        if re.search(escaped, readme) is None:
            fail(f"README missing command {command!r} ({meaning})")

    if "1-9" not in readme and "1~9" not in readme:
        fail("README missing direct track command range 1-9")


def check_pin_conflict_documented() -> None:
    config = read_text("drivers/platform_config.h")
    pins = read_text("drivers/board_pins.h")
    readme = read_text("README.md")

    if "PLAYER_BT_UART_MODE             PLAYER_BT_UART_UCA1_P45" not in config:
        fail("Bluetooth must default to UCA1 P4.4/P4.5 to avoid TF-card MISO conflict")
    if "BT_UCA0_TX_BIT                  BIT3" not in pins:
        fail("alternate UCA0 conflict pin must remain visible in board_pins.h")
    if "P3.3" not in readme or "MISO" not in readme or "冲突" not in readme:
        fail("README must document the P3.3 Bluetooth/TF-card conflict")


def check_local_button_long_press() -> None:
    local_header = read_text("drivers/local_buttons.h")
    local_source = read_text("drivers/local_buttons.c")
    config = read_text("drivers/platform_config.h")

    for token in [
        "LOCAL_BUTTON_EVENT_STOP",
        "LOCAL_BUTTON_EVENT_MUTE",
        "LOCAL_BUTTON_EVENT_ORDER",
        "LOCAL_BUTTON_LONG_PRESS_MS",
        "LOCAL_BUTTON_LONG_PRESS_TICKS",
    ]:
        if token not in local_header and token not in local_source and token not in config:
            fail(f"local button long-press support missing token: {token}")


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
    check_required_files()
    check_header_comments()
    check_commands()
    check_pin_conflict_documented()
    check_local_button_long_press()
    check_build_map()
    print("firmware static checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
