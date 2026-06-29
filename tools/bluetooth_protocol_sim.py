"""Bluetooth command-link simulator for the Android app and firmware protocol.

The real HC-05 path is UART bytes over RFCOMM. This simulator exercises the
same single-character command contract and checks that phone-side command
buttons cover the required firmware actions.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MAX_TRACKS = 9
MAX_VOLUME = 32
TRACE_DEPTH = 6
AVAILABLE_TRACKS = (1, 3)


def platform_define(name: str) -> str:
    """Returns one quoted string #define from drivers/platform_config.h."""

    text = (ROOT / "drivers" / "platform_config.h").read_text(encoding="utf-8")
    match = re.search(rf"{name}\s+\"([^\"]+)\"", text)
    if not match:
        raise RuntimeError(f"{name} not found in platform_config.h")
    return match.group(1)


FIRMWARE_NAME = platform_define("PLAYER_FIRMWARE_NAME")
FIRMWARE_VERSION = platform_define("PLAYER_FIRMWARE_VERSION")
HARDWARE_PROFILE = platform_define("PLAYER_HARDWARE_PROFILE")
WIRING_LINES = [
    f"pin profile={HARDWARE_PROFILE}",
    "pin tf cs=P4.0 sck=P3.1 mosi=P3.2 miso=P3.3",
    "pin i2s bck=P4.1 lrck=P4.2 din=P4.3",
    "pin ec11 a=P2.1 b=P2.2 sw=P2.3",
    "pin local s1=P1.2 s2=P1.3 s4=P2.6 led=P1.0",
    "pin bt tx=P4.4 rx=P4.5 mode=UCA1 note=no-tf-conflict",
    "pin epaper optional=P6.0-P6.5 default=disabled",
]


@dataclass
class SimulatedPlayer:
    mode: str = "paused"
    track: int = 1
    volume: int = 18
    saved_volume: int = 18
    order: str = "repeat_all"
    progress: int = 0
    rx_count: int = 0
    bad_count: int = 0
    status_reports: int = 0
    display_reports: int = 0
    last_command: str = "-"
    input_encoder_cw: int = 0
    input_encoder_ccw: int = 0
    input_encoder_button: int = 0
    input_encoder_long: int = 0
    input_s1_short: int = 0
    input_s1_long: int = 0
    input_s2_short: int = 0
    input_s2_long: int = 0
    input_s4_short: int = 0
    input_s4_long: int = 0
    trace: list[str] = field(default_factory=list)
    transcript: list[str] = field(default_factory=list)

    def trace_event(self, label: str) -> None:
        self.trace.append(label)
        if len(self.trace) > TRACE_DEPTH:
            self.trace = self.trace[-TRACE_DEPTH:]

    def open_track(self, track: int) -> None:
        self.track = track
        self.mode = "playing"
        self.transcript.append(f"open TRACK0{self.track}.WAV")

    def next_available_track(self) -> int:
        higher_tracks = [track for track in AVAILABLE_TRACKS if track > self.track]
        return (higher_tracks or list(AVAILABLE_TRACKS))[0]

    def previous_available_track(self) -> int:
        lower_tracks = [track for track in AVAILABLE_TRACKS if track < self.track]
        return (lower_tracks or list(AVAILABLE_TRACKS))[-1]

    def status_line(self) -> str:
        return (
            f"status={self.mode} track={self.track} volume={self.volume} order={self.order} "
            f"rate=16000Hz channels=2 progress={self.progress}"
        )

    def report_status(self) -> None:
        self.status_reports += 1
        self.transcript.append(self.status_line())

    def display_lines(self) -> list[str]:
        order_short = {"sequence": "SEQ", "repeat_all": "ALL", "repeat_one": "ONE"}[self.order]
        return [
            f"display 1:{self.mode} T{self.track} V{self.volume} {order_short}",
            "display 2:SD:OK WAV:OPEN",
            f"display 3:16000Hz 2ch P{self.progress}%",
        ]

    def report_ui_snapshot(self) -> None:
        self.report_status()
        self.transcript.extend(self.display_lines())
        self.display_reports += 1

    def finish_track(self) -> None:
        if self.order == "repeat_one":
            self.mode = "playing"
            self.transcript.append(f"replay TRACK0{self.track}.WAV")
            return

        higher_tracks = [track for track in AVAILABLE_TRACKS if track > self.track]
        if self.order == "sequence":
            if higher_tracks:
                self.track = higher_tracks[0]
                self.mode = "playing"
                self.transcript.append(f"open TRACK0{self.track}.WAV")
            else:
                self.mode = "stopped"
                self.transcript.append("end")
            return

        self.open_track((higher_tracks or list(AVAILABLE_TRACKS))[0])

    def send(self, command: str) -> None:
        command = command.lower()
        self.rx_count += 1
        self.last_command = command
        if command in "123456789":
            self.trace_event("bt:track")
            self.track = int(command)
            self.mode = "playing"
            self.transcript.append(f"open TRACK0{self.track}.WAV")
            self.report_ui_snapshot()
        elif command == "p":
            self.trace_event("bt:play")
            self.mode = "paused" if self.mode == "playing" else "playing"
            self.transcript.append(self.mode)
            self.report_ui_snapshot()
        elif command == "s":
            self.trace_event("bt:stop")
            self.mode = "stopped"
            self.transcript.append("stop")
            self.report_ui_snapshot()
        elif command == "r":
            self.trace_event("bt:replay")
            self.mode = "playing"
            self.transcript.append("replay")
            self.report_ui_snapshot()
        elif command in ("n", ">"):
            self.trace_event("bt:next")
            self.open_track(self.next_available_track())
            self.report_ui_snapshot()
        elif command in ("b", "<"):
            self.trace_event("bt:prev")
            self.open_track(self.previous_available_track())
            self.report_ui_snapshot()
        elif command in ("+", "="):
            self.trace_event("bt:vol+")
            self.volume = min(MAX_VOLUME, self.volume + 1)
            self.transcript.append(f"volume={self.volume}")
            self.report_ui_snapshot()
        elif command in ("-", "_"):
            self.trace_event("bt:vol-")
            self.volume = max(0, self.volume - 1)
            self.transcript.append(f"volume={self.volume}")
            self.report_ui_snapshot()
        elif command == "m":
            self.trace_event("bt:mute")
            if self.volume > 0:
                self.saved_volume = self.volume
                self.volume = 0
                self.transcript.append("mute=on")
            else:
                self.volume = self.saved_volume or 18
                self.transcript.append("mute=off")
            self.report_ui_snapshot()
        elif command == "o":
            self.trace_event("bt:order")
            if self.order == "sequence":
                self.order = "repeat_all"
            elif self.order == "repeat_all":
                self.order = "repeat_one"
            else:
                self.order = "sequence"
            self.transcript.append(f"order={self.order}")
            self.report_ui_snapshot()
        elif command == "t":
            self.trace_event("bt:tone")
            self.transcript.append("tone start")
            self.transcript.append("tone done")
            self.report_ui_snapshot()
        elif command == "i":
            self.transcript.append(f"info name={FIRMWARE_NAME} version={FIRMWARE_VERSION} profile={HARDWARE_PROFILE}")
        elif command == "e":
            self.transcript.append("selftest bt=ok sd=ok file=open dac=test-with-t")
        elif command == "l":
            self.transcript.append("tracks 1=ok 2=-- 3=ok 4=-- 5=-- 6=-- 7=-- 8=-- 9=--")
        elif command == "d":
            self.transcript.extend(self.display_lines())
            self.display_reports += 1
        elif command == "?":
            self.report_status()
        elif command == "h":
            self.transcript.append("help")
        elif command == "k":
            self.transcript.append(
                f"link rx={self.rx_count} status={self.status_reports} display={self.display_reports} "
                f"bad={self.bad_count} last={self.last_command} uptime=1234ms"
            )
        elif command == "u":
            self.transcript.append(
                f"input ecw={self.input_encoder_cw} eccw={self.input_encoder_ccw} "
                f"eb={self.input_encoder_button} elong={self.input_encoder_long} "
                f"s1={self.input_s1_short} s1l={self.input_s1_long} "
                f"s2={self.input_s2_short} s2l={self.input_s2_long} "
                f"s4={self.input_s4_short} s4l={self.input_s4_long}"
            )
        elif command == "x":
            self.trace_event("bt:trace")
            self.transcript.append(
                "trace count="
                + str(len(self.trace))
                + "".join(f" {index + 1}={label}" for index, label in enumerate(self.trace))
            )
        elif command == "w":
            self.transcript.extend(WIRING_LINES)
        else:
            self.bad_count += 1
            self.trace_event("bt:bad")


def assert_equal(actual: object, expected: object, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def run_required_flow() -> SimulatedPlayer:
    player = SimulatedPlayer()

    for command in ["p", "+", "+", "n", "b", "-", "m", "m", "o", "s", "3", "r", "t", "i", "e", "l", "d", "?", "k", "u", "x", "w"]:
        player.send(command)

    assert_equal(player.mode, "playing", "mode after direct track command")
    assert_equal(player.track, 3, "direct track command")
    assert_equal(player.volume, 19, "volume plus/minus flow")
    assert_equal(player.order, "repeat_one", "play order cycle")
    assert "tone done" in player.transcript
    assert any(line.startswith("info name=") for line in player.transcript)
    assert any(line.startswith("selftest bt=ok") for line in player.transcript)
    assert any(line.startswith("tracks 1=ok") for line in player.transcript)
    assert any(line.startswith("display 1:playing T3 V19 ONE") for line in player.transcript)
    assert any(line.startswith("status=playing track=3 volume=19 order=repeat_one") for line in player.transcript)
    link_lines = [line for line in player.transcript if line.startswith("link rx=")]
    assert link_lines
    assert "last=k" in link_lines[-1]
    assert any(line.startswith("input ecw=") for line in player.transcript)
    trace_lines = [line for line in player.transcript if line.startswith("trace count=")]
    assert trace_lines
    assert "bt:trace" in trace_lines[-1] and "bt:tone" in trace_lines[-1]
    assert any(line.startswith("pin bt tx=P4.4") for line in player.transcript)
    return player


def run_order_flow() -> None:
    player = SimulatedPlayer(mode="playing", track=3)

    player.send("o")
    assert_equal(player.order, "repeat_one", "repeat-one command")
    player.finish_track()
    assert_equal(player.track, 3, "repeat-one keeps track")
    assert_equal(player.mode, "playing", "repeat-one keeps playing")

    player.send("o")
    assert_equal(player.order, "sequence", "sequence command")
    player.finish_track()
    assert_equal(player.mode, "stopped", "sequence stops after last available track")
    assert "end" in player.transcript

    player = SimulatedPlayer(mode="playing", track=3, order="repeat_all")
    player.finish_track()
    assert_equal(player.track, 1, "repeat-all wraps to first available track")
    assert_equal(player.mode, "playing", "repeat-all keeps playing")


def main() -> int:
    player = run_required_flow()
    run_order_flow()
    print("simulated HC-05 command flow passed")
    for line in player.transcript:
        print(line)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
