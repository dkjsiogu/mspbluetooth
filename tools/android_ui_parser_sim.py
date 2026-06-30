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
LINK_LINE = "link rx=15 status=10 display=9 bad=0 last=k uptime=1234ms"
INPUT_LINE = "input ecw=3 eccw=1 eb=2 elong=1 s1=2 s1l=1 s2=1 s2l=1 s4=1 s4l=1"
TRACE_LINE = "trace count=6 1=bt:vol+ 2=bt:next 3=bt:prev 4=bt:order 5=bt:track 6=bt:trace"
WIRING_LINES = [
    "pin profile=TF:P3.1-3.3 I2S:P4.1-4.3 BT:UCA1",
    "pin tf cs=P4.0 sck=P3.1 mosi=P3.2 miso=P3.3",
    "pin i2s bck=P4.1 lrck=P4.2 din=P4.3",
    "pin ec11 a=P2.1 b=P2.2 sw=P2.3",
    "pin local s1=P1.2 s2=P1.3 s4=P2.6 led=P1.0",
    "pin bt tx=P4.4 rx=P4.5 mode=UCA1 note=no-tf-conflict",
    "pin epaper optional=P6.0-P6.5 default=disabled",
]


@dataclass
class AndroidUiState:
    """AndroidUiState: parsed dashboard, display frame, and raw line buffer."""

    dashboard_text: str = "Mode: --\nTrack: --\nVolume: --\nOrder: --\nProgress: --"
    volume_bar: int = 0
    progress_bar: int = 0
    health_sd: str = "--"
    health_info: str = "--"
    health_selftest: str = "--"
    health_tone: str = "--"
    health_file: str = "--"
    health_error: str = "--"
    display_lines: list[str] = field(default_factory=lambda: ["--", "--", "--"])
    track_list_text: str = "Tracks\n1: --  2: --  3: --\n4: --  5: --  6: --\n7: --  8: --  9: --"
    link_text: str = "Link\nRX: --  Status: --  Display: --\nBad: --  Last: --  Uptime: --"
    input_text: str = "Input\nEC11 CW:-- CCW:-- SW:-- Long:--\nS1:--/-- S2:--/-- S4:--/--"
    trace_text: str = "Trace\n--"
    wiring_lines: list[str] = field(default_factory=lambda: ["Profile: --", "TF: --", "I2S: --", "EC11: --", "Local: --", "BT: --", "E-paper: --"])
    acceptance_text: str = "Acceptance 0/9\nSD:-- Info:-- Selftest:-- Tracks:-- Wiring:--\nDisplay:-- Status:-- Tone:-- Open:--"
    hardware_text: str = "Hardware 0/9\nBT:-- TX:-- RX:-- SD:-- Audio:--\nStatus:-- Wiring:-- Input:-- Trace:--"
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
    acceptance_wiring: bool = False
    hardware_connected: bool = False
    hardware_tx: bool = False
    hardware_rx: bool = False
    hardware_sd: bool = False
    hardware_audio: bool = False
    hardware_status: bool = False
    hardware_wiring: bool = False
    hardware_input: bool = False
    hardware_trace: bool = False

    @property
    def display_text(self) -> str:
        return "Display frame\n" + "\n".join(self.display_lines)

    @property
    def health_text(self) -> str:
        return (
            "Health\n"
            f"SD:{self.health_sd} Info:{self.health_info} "
            f"Selftest:{compact_health(self.health_selftest)} Tone:{self.health_tone}\n"
            f"File:{self.health_file}\nError:{self.health_error}"
        )

    @property
    def wiring_text(self) -> str:
        return "Wiring\n" + "\n".join(self.wiring_lines)


def field_value(line: str, key: str) -> str:
    start = line.find(key)
    if start < 0:
        return "--"
    start += len(key)
    end = line.find(" ", start)
    if end < 0:
        end = len(line)
    return line[start:end]


def bounded_int(text: str, minimum: int, maximum: int) -> int:
    try:
        value = int(text.replace("%", "").strip())
    except ValueError:
        return minimum
    return min(max(value, minimum), maximum)


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
    state.volume_bar = bounded_int(volume, 0, 32)
    state.progress_bar = bounded_int(progress, 0, 100)


def compact_health(text: str) -> str:
    if len(text) <= 24:
        return text
    return text[:24]


def update_health_panel(state: AndroidUiState, line: str) -> None:
    if line.startswith("sd mounted"):
        state.health_sd = "OK"
    elif line.startswith("info name="):
        state.health_info = field_value(line, "version=")
    elif line.startswith("selftest "):
        state.health_selftest = line[len("selftest ") :]
    elif line.startswith("tone start"):
        state.health_tone = "running"
    elif line.startswith("tone done"):
        state.health_tone = "done"
    elif line.startswith("open TRACK0"):
        state.health_file = line[len("open ") :]
    elif line.startswith("error: "):
        state.health_error = line[len("error: ") :]


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


def update_link_panel(state: AndroidUiState, link_line: str) -> None:
    rx = field_value(link_line, "rx=")
    status = field_value(link_line, "status=")
    display = field_value(link_line, "display=")
    bad = field_value(link_line, "bad=")
    last = field_value(link_line, "last=")
    uptime = field_value(link_line, "uptime=")
    state.link_text = f"Link\nRX: {rx}  Status: {status}  Display: {display}\nBad: {bad}  Last: {last}  Uptime: {uptime}"


def update_input_panel(state: AndroidUiState, input_line: str) -> None:
    ecw = field_value(input_line, "ecw=")
    eccw = field_value(input_line, "eccw=")
    eb = field_value(input_line, "eb=")
    elong = field_value(input_line, "elong=")
    s1 = field_value(input_line, "s1=")
    s1l = field_value(input_line, "s1l=")
    s2 = field_value(input_line, "s2=")
    s2l = field_value(input_line, "s2l=")
    s4 = field_value(input_line, "s4=")
    s4l = field_value(input_line, "s4l=")
    state.input_text = f"Input\nEC11 CW:{ecw} CCW:{eccw} SW:{eb} Long:{elong}\nS1:{s1}/{s1l} S2:{s2}/{s2l} S4:{s4}/{s4l}"


def update_trace_panel(state: AndroidUiState, trace_line: str) -> None:
    shown = 0
    lines = ["Trace"]
    current_line: list[str] = []
    for part in trace_line.split(" "):
        if "=" not in part or part.startswith("count="):
            continue
        current_line.append(part)
        shown += 1
        if shown % 3 == 0:
            lines.append("  ".join(current_line))
            current_line = []
    if current_line:
        lines.append("  ".join(current_line))
    if shown == 0:
        lines.append("--")
    state.trace_text = "\n".join(lines)


def update_wiring_panel(state: AndroidUiState, pin_line: str) -> None:
    if pin_line.startswith("pin profile="):
        state.wiring_lines[0] = "Profile: " + pin_line[len("pin profile=") :]
    elif pin_line.startswith("pin tf "):
        state.wiring_lines[1] = "TF: " + pin_line[len("pin tf ") :]
    elif pin_line.startswith("pin i2s "):
        state.wiring_lines[2] = "I2S: " + pin_line[len("pin i2s ") :]
    elif pin_line.startswith("pin ec11 "):
        state.wiring_lines[3] = "EC11: " + pin_line[len("pin ec11 ") :]
    elif pin_line.startswith("pin local "):
        state.wiring_lines[4] = "Local: " + pin_line[len("pin local ") :]
    elif pin_line.startswith("pin bt "):
        state.wiring_lines[5] = "BT: " + pin_line[len("pin bt ") :]
    elif pin_line.startswith("pin epaper "):
        state.wiring_lines[6] = "E-paper: " + pin_line[len("pin epaper ") :]


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
            state.acceptance_wiring,
        ]
    )
    state.acceptance_text = (
        f"Acceptance {passed}/9\n"
        f"SD:{mark(state.acceptance_sd_mounted)} Info:{mark(state.acceptance_info)} "
        f"Selftest:{mark(state.acceptance_selftest)} Tracks:{mark(state.acceptance_tracks)} "
        f"Wiring:{mark(state.acceptance_wiring)}\n"
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
    elif line.startswith("pin bt "):
        state.acceptance_wiring = True
    render_acceptance_summary(state)


def render_hardware_summary(state: AndroidUiState) -> None:
    passed = sum(
        [
            state.hardware_connected,
            state.hardware_tx,
            state.hardware_rx,
            state.hardware_sd,
            state.hardware_audio,
            state.hardware_status,
            state.hardware_wiring,
            state.hardware_input,
            state.hardware_trace,
        ]
    )
    state.hardware_text = (
        f"Hardware {passed}/9\n"
        f"BT:{mark(state.hardware_connected)} TX:{mark(state.hardware_tx)} "
        f"RX:{mark(state.hardware_rx)} SD:{mark(state.hardware_sd)} "
        f"Audio:{mark(state.hardware_audio)}\n"
        f"Status:{mark(state.hardware_status)} Wiring:{mark(state.hardware_wiring)} "
        f"Input:{mark(state.hardware_input)} Trace:{mark(state.hardware_trace)}"
    )


def update_hardware_summary(state: AndroidUiState, line: str) -> None:
    state.hardware_rx = True
    if line.startswith("sd mounted") or line.startswith("tracks"):
        state.hardware_sd = True
    elif line.startswith("tone done") or line.startswith("open TRACK0") or (
        line.startswith("selftest ") and "i2s=ok" in line
    ):
        state.hardware_audio = True
    elif line.startswith("status=") or line.startswith("display "):
        state.hardware_status = True
    elif line.startswith("pin bt "):
        state.hardware_wiring = True
    elif line.startswith("input "):
        state.hardware_input = True
    elif line.startswith("trace "):
        state.hardware_trace = True
    render_hardware_summary(state)


def mark_connected(state: AndroidUiState) -> None:
    state.hardware_connected = True
    render_hardware_summary(state)


def mark_tx(state: AndroidUiState) -> None:
    state.hardware_tx = True
    render_hardware_summary(state)


def parse_incoming_line(state: AndroidUiState, line: str) -> None:
    state.parsed_lines.append(line)
    update_acceptance_summary(state, line)
    update_hardware_summary(state, line)
    if line.startswith("status="):
        update_dashboard(state, line)
    elif (
        line.startswith("sd mounted")
        or line.startswith("info name=")
        or line.startswith("selftest ")
        or line.startswith("tone start")
        or line.startswith("tone done")
        or line.startswith("open TRACK0")
        or line.startswith("error: ")
    ):
        update_health_panel(state, line)
    elif line.startswith("display 1:"):
        state.display_lines[0] = line[len("display 1:") :]
    elif line.startswith("display 2:"):
        state.display_lines[1] = line[len("display 2:") :]
    elif line.startswith("display 3:"):
        state.display_lines[2] = line[len("display 3:") :]
    elif line.startswith("tracks"):
        update_track_list(state, line)
    elif line.startswith("link "):
        update_link_panel(state, line)
    elif line.startswith("input "):
        update_input_panel(state, line)
    elif line.startswith("trace "):
        update_trace_panel(state, line)
    elif line.startswith("pin "):
        update_wiring_panel(state, line)


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
    stream = "\r\n".join([*BOOT_LINES, INFO_LINE, SELFTEST_LINE, TRACKS_LINE, *DISPLAY_LINES, STATUS_LINE, *TONE_LINES, LINK_LINE, INPUT_LINE, TRACE_LINE, *WIRING_LINES]) + "\r\n"
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
    if state.volume_bar != 19 or state.progress_bar != 42:
        raise AssertionError(f"visual bars mismatch: volume={state.volume_bar}, progress={state.progress_bar}")
    expected_health = "Health\nSD:OK Info:1.4.1 Selftest:bt=ok sd=ok wav=ok i2s=o Tone:done\nFile:TRACK01.WAV\nError:--"
    if state.health_text != expected_health:
        raise AssertionError(f"health panel mismatch: {state.health_text!r}")
    if state.display_lines != expected_display:
        raise AssertionError(f"display mismatch: {state.display_lines!r}")
    expected_tracks = "Tracks\n1: ok  2: --  3: ok\n4: --  5: --  6: --\n7: --  8: --  9: --"
    if state.track_list_text != expected_tracks:
        raise AssertionError(f"track list mismatch: {state.track_list_text!r}")
    if not state.acceptance_text.startswith("Acceptance 9/9"):
        raise AssertionError(f"acceptance summary mismatch: {state.acceptance_text!r}")
    expected_hardware = "Hardware 7/9\nBT:-- TX:-- RX:OK SD:OK Audio:OK\nStatus:OK Wiring:OK Input:OK Trace:OK"
    if state.hardware_text != expected_hardware:
        raise AssertionError(f"hardware summary mismatch: {state.hardware_text!r}")
    expected_link = "Link\nRX: 15  Status: 10  Display: 9\nBad: 0  Last: k  Uptime: 1234ms"
    if state.link_text != expected_link:
        raise AssertionError(f"link panel mismatch: {state.link_text!r}")
    expected_input = "Input\nEC11 CW:3 CCW:1 SW:2 Long:1\nS1:2/1 S2:1/1 S4:1/1"
    if state.input_text != expected_input:
        raise AssertionError(f"input panel mismatch: {state.input_text!r}")
    expected_trace = "Trace\n1=bt:vol+  2=bt:next  3=bt:prev\n4=bt:order  5=bt:track  6=bt:trace"
    if state.trace_text != expected_trace:
        raise AssertionError(f"trace panel mismatch: {state.trace_text!r}")
    if "BT: tx=P4.4 rx=P4.5 mode=UCA1 note=no-tf-conflict" not in state.wiring_text:
        raise AssertionError(f"wiring panel mismatch: {state.wiring_text!r}")
    if state.rx_line_buffer != "":
        raise AssertionError(f"line buffer should be empty, got {state.rx_line_buffer!r}")
    return state


def main() -> int:
    state = run_fragmented_flow()
    print("Android UI parser simulation passed")
    print(state.dashboard_text.replace("\n", " | "))
    print(f"Bars volume={state.volume_bar}/32 progress={state.progress_bar}/100")
    print(state.health_text.replace("\n", " | "))
    print(state.display_text.replace("\n", " | "))
    print(state.track_list_text.replace("\n", " | "))
    print(state.link_text.replace("\n", " | "))
    print(state.input_text.replace("\n", " | "))
    print(state.trace_text.replace("\n", " | "))
    print(state.wiring_text.replace("\n", " | "))
    print(state.acceptance_text.replace("\n", " | "))
    print(state.hardware_text.replace("\n", " | "))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
