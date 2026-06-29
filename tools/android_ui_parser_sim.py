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
TRACKS_LINE = "tracks 1=ok 2=-- 3=ok 4=-- 5=-- 6=-- 7=-- 8=-- 9=--"
BOOT_LINES = [
    "sd mounted",
    "open TRACK01.WAV",
]
INFO_LINE = "info name=MSP430F5529-BT-WAV version=1.4.1 profile=bt_wav_player"
SELFTEST_LINE = "selftest bt=ok sd=ok wav=ok i2s=ok buttons=ok"
TONE_LINES = [
    "tone start",
    "tone done",
]


@dataclass
class AndroidUiState:
    """AndroidUiState: parsed dashboard, display frame, and raw line buffer."""

    dashboard_text: str = "Mode: --\nTrack: --\nVolume: --\nOrder: --\nProgress: --"
    display_lines: list[str] = field(default_factory=lambda: ["--", "--", "--"])
    track_list_text: str = "Tracks\n1: --  2: --  3: --\n4: --  5: --  6: --\n7: --  8: --  9: --"
    acceptance_text: str = "Acceptance 0/8\nSD:-- Info:-- Selftest:-- Tracks:--\nDisplay:-- Status:-- Tone:-- Open:--"
    rx_line_buffer: str = ""
    parsed_lines: list[str] = field(default_factory=list)
    acceptance_sd_mounted: bool = False
    acceptance_info: bool = False
    acceptance_selftest: bool = False
    acceptance_tracks: bool = False
    acceptance_display_1: bool = False
    acceptance_display_2: bool = False
    acceptance_display_3: bool = False
    acceptance_status: bool = False
    acceptance_tone_start: bool = False
    acceptance_tone_done: bool = False
    acceptance_track_open: bool = False

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


def update_track_list(state: AndroidUiState, tracks_line: str) -> None:
    parts = tracks_line.split(" ")
    shown = 0
    lines = ["Tracks"]
    current_line: list[str] = []
    for part in parts:
        if "=" not in part:
            continue
        track, status = part.split("=", 1)
        current_line.append(f"{track}: {status}")
        shown += 1
        if shown % 3 == 0:
            lines.append("  ".join(current_line))
            current_line = []
    if current_line:
        lines.append("  ".join(current_line))
    if shown == 0:
        lines.append("--")
    state.track_list_text = "\n".join(lines)


def mark(passed: bool) -> str:
    return "OK" if passed else "--"


def render_acceptance_summary(state: AndroidUiState) -> None:
    display_ok = state.acceptance_display_1 and state.acceptance_display_2 and state.acceptance_display_3
    tone_ok = state.acceptance_tone_start and state.acceptance_tone_done
    passed = sum(
        [
            state.acceptance_sd_mounted,
            state.acceptance_info,
            state.acceptance_selftest,
            state.acceptance_tracks,
            display_ok,
            state.acceptance_status,
            tone_ok,
            state.acceptance_track_open,
        ]
    )
    state.acceptance_text = (
        f"Acceptance {passed}/8\n"
        f"SD:{mark(state.acceptance_sd_mounted)} Info:{mark(state.acceptance_info)} "
        f"Selftest:{mark(state.acceptance_selftest)} Tracks:{mark(state.acceptance_tracks)}\n"
        f"Display:{mark(display_ok)} Status:{mark(state.acceptance_status)} "
        f"Tone:{mark(tone_ok)} Open:{mark(state.acceptance_track_open)}"
    )


def update_acceptance_summary(state: AndroidUiState, line: str) -> None:
    if line.startswith("sd mounted"):
        state.acceptance_sd_mounted = True
    elif line.startswith("info name="):
        state.acceptance_info = True
    elif line.startswith("selftest bt=ok"):
        state.acceptance_selftest = True
    elif line.startswith("tracks"):
        state.acceptance_tracks = True
    elif line.startswith("display 1:"):
        state.acceptance_display_1 = True
    elif line.startswith("display 2:"):
        state.acceptance_display_2 = True
    elif line.startswith("display 3:"):
        state.acceptance_display_3 = True
    elif line.startswith("status="):
        state.acceptance_status = True
    elif line.startswith("tone start"):
        state.acceptance_tone_start = True
    elif line.startswith("tone done"):
        state.acceptance_tone_done = True
    elif line.startswith("open TRACK0"):
        state.acceptance_track_open = True
    render_acceptance_summary(state)


def parse_incoming_line(state: AndroidUiState, line: str) -> None:
    state.parsed_lines.append(line)
    update_acceptance_summary(state, line)
    if line.startswith("status="):
        update_dashboard(state, line)
    elif line.startswith("display 1:"):
        state.display_lines[0] = line[len("display 1:") :]
    elif line.startswith("display 2:"):
        state.display_lines[1] = line[len("display 2:") :]
    elif line.startswith("display 3:"):
        state.display_lines[2] = line[len("display 3:") :]
    elif line.startswith("tracks"):
        update_track_list(state, line)


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
    stream = "\r\n".join([*BOOT_LINES, INFO_LINE, SELFTEST_LINE, TRACKS_LINE, *DISPLAY_LINES, STATUS_LINE, *TONE_LINES]) + "\r\n"
    chunks = [stream[:7], stream[7:29], stream[29:61], stream[61:92], stream[92:130], stream[130:181], stream[181:]]
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
    expected_tracks = "Tracks\n1: ok  2: --  3: ok\n4: --  5: --  6: --\n7: --  8: --  9: --"
    if state.track_list_text != expected_tracks:
        raise AssertionError(f"track list mismatch: {state.track_list_text!r}")
    if not state.acceptance_text.startswith("Acceptance 8/8"):
        raise AssertionError(f"acceptance summary mismatch: {state.acceptance_text!r}")
    if state.rx_line_buffer != "":
        raise AssertionError(f"line buffer should be empty, got {state.rx_line_buffer!r}")
    return state


def main() -> int:
    state = run_fragmented_flow()
    print("Android UI parser simulation passed")
    print(state.dashboard_text.replace("\n", " | "))
    print(state.display_text.replace("\n", " | "))
    print(state.track_list_text.replace("\n", " | "))
    print(state.acceptance_text.replace("\n", " | "))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
