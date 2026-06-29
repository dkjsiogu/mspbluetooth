"""Host-side simulator for the Android app's incoming-line parser.

The real app receives arbitrary RFCOMM byte chunks from HC-05. This script
feeds fragmented firmware responses into a small model of MainActivity's line
buffer, dashboard parser, and display-frame parser so the APK verification can
cover visible phone-side effects instead of only checking that buttons exist.
"""

from __future__ import annotations

from dataclasses import dataclass, field


STATUS_LINE = "status=playing track=3 volume=19 order=repeat_one rate=16000Hz channels=2 progress=42"
DISPLAY_LINES = [
    "display 1:playing T3 V19 ONE",
    "display 2:SD:OK WAV:OPEN",
    "display 3:16000Hz 2ch P42%",
]


@dataclass
class AndroidUiState:
    """AndroidUiState: parsed dashboard, display frame, and raw line buffer."""

    dashboard_text: str = "Mode: --\nTrack: --\nVolume: --\nOrder: --\nProgress: --"
    display_lines: list[str] = field(default_factory=lambda: ["--", "--", "--"])
    rx_line_buffer: str = ""
    parsed_lines: list[str] = field(default_factory=list)

    @property
    def display_text(self) -> str:
        return "Display frame\n" + "\n".join(self.display_lines)


def field_value(line: str, key: str) -> str:
    start = line.find(key)
    if start < 0:
        return "--"
    start += len(key)
    end = line.find(" ", start)
    if end < 0:
        end = len(line)
    return line[start:end]


def update_dashboard(state: AndroidUiState, status_line: str) -> None:
    mode = field_value(status_line, "status=")
    track = field_value(status_line, "track=")
    volume = field_value(status_line, "volume=")
    order = field_value(status_line, "order=")
    progress = field_value(status_line, "progress=")
    state.dashboard_text = (
        f"Mode: {mode}\nTrack: {track}\nVolume: {volume}\n"
        f"Order: {order}\nProgress: {progress}%"
    )


def parse_incoming_line(state: AndroidUiState, line: str) -> None:
    state.parsed_lines.append(line)
    if line.startswith("status="):
        update_dashboard(state, line)
    elif line.startswith("display 1:"):
        state.display_lines[0] = line[len("display 1:") :]
    elif line.startswith("display 2:"):
        state.display_lines[1] = line[len("display 2:") :]
    elif line.startswith("display 3:"):
        state.display_lines[2] = line[len("display 3:") :]


def handle_incoming_text(state: AndroidUiState, text: str) -> None:
    state.rx_line_buffer += text
    while "\n" in state.rx_line_buffer:
        newline = state.rx_line_buffer.find("\n")
        line = state.rx_line_buffer[:newline].strip()
        state.rx_line_buffer = state.rx_line_buffer[newline + 1 :]
        if line:
            parse_incoming_line(state, line)


def run_fragmented_flow() -> AndroidUiState:
    state = AndroidUiState()
    stream = "\r\n".join([STATUS_LINE, *DISPLAY_LINES]) + "\r\n"
    chunks = [stream[:7], stream[7:29], stream[29:61], stream[61:92], stream[92:130], stream[130:]]
    for chunk in chunks:
        handle_incoming_text(state, chunk)

    expected_dashboard = "Mode: playing\nTrack: 3\nVolume: 19\nOrder: repeat_one\nProgress: 42%"
    expected_display = [
        "playing T3 V19 ONE",
        "SD:OK WAV:OPEN",
        "16000Hz 2ch P42%",
    ]
    if state.dashboard_text != expected_dashboard:
        raise AssertionError(f"dashboard mismatch: {state.dashboard_text!r}")
    if state.display_lines != expected_display:
        raise AssertionError(f"display mismatch: {state.display_lines!r}")
    if state.rx_line_buffer != "":
        raise AssertionError(f"line buffer should be empty, got {state.rx_line_buffer!r}")
    return state


def main() -> int:
    state = run_fragmented_flow()
    print("Android UI parser simulation passed")
    print(state.dashboard_text.replace("\n", " | "))
    print(state.display_text.replace("\n", " | "))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
