"""Bluetooth UART diagnostic simulator and acceptance transcript generator.

The firmware receives HC-05 data as independent UART bytes. This simulator
checks the byte-level contract that matters on real phones: fragmented command
delivery, uppercase aliases, harmless line endings, ignored noise, and stable
diagnostic responses for classroom bring-up.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass, field
from pathlib import Path

from bluetooth_protocol_sim import FIRMWARE_NAME, FIRMWARE_VERSION, HARDWARE_PROFILE, SimulatedPlayer


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REPORT = ROOT / "docs" / "bluetooth_diagnostic_report.md"

ALLOWED_TRANSCRIPT_PREFIXES = (
    "open ",
    "play",
    "pause",
    "stop",
    "replay",
    "next ",
    "previous ",
    "volume=",
    "mute=",
    "order=",
    "tone ",
    "info ",
    "selftest ",
    "tracks ",
    "display ",
    "status=",
    "link ",
    "help",
    "end",
)


@dataclass
class DiagnosticCase:
    name: str
    chunks: list[str]
    expected_tail: list[str]
    expected_state: dict[str, object] = field(default_factory=dict)


@dataclass
class DiagnosticResult:
    name: str
    input_repr: str
    transcript: list[str]
    mode: str
    track: int
    volume: int
    order: str


def feed_chunks(player: SimulatedPlayer, chunks: list[str]) -> None:
    """Feeds command chunks exactly as a UART receive loop would see bytes."""

    for chunk in chunks:
        for byte in chunk:
            player.send(byte)


def assert_tail(transcript: list[str], expected_tail: list[str], name: str) -> None:
    if transcript[-len(expected_tail) :] != expected_tail:
        raise AssertionError(f"{name}: expected transcript tail {expected_tail!r}, got {transcript!r}")


def assert_state(player: SimulatedPlayer, expected_state: dict[str, object], name: str) -> None:
    for field_name, expected in expected_state.items():
        actual = getattr(player, field_name)
        if actual != expected:
            raise AssertionError(f"{name}: expected {field_name}={expected!r}, got {actual!r}")


def assert_transcript_is_known(transcript: list[str]) -> None:
    for line in transcript:
        if not line.startswith(ALLOWED_TRANSCRIPT_PREFIXES):
            raise AssertionError(f"unexpected diagnostic transcript line: {line!r}")


def run_case(case: DiagnosticCase) -> DiagnosticResult:
    player = SimulatedPlayer()
    feed_chunks(player, case.chunks)
    assert_tail(player.transcript, case.expected_tail, case.name)
    assert_state(player, case.expected_state, case.name)
    assert_transcript_is_known(player.transcript)
    return DiagnosticResult(
        name=case.name,
        input_repr=" | ".join(repr(chunk) for chunk in case.chunks),
        transcript=list(player.transcript),
        mode=player.mode,
        track=player.track,
        volume=player.volume,
        order=player.order,
    )


def run_diagnostic_cases() -> list[DiagnosticResult]:
    cases = [
        DiagnosticCase(
            name="bring-up transcript",
            chunks=["i", "e", "l", "d", "?", "k"],
            expected_tail=[
                f"info name={FIRMWARE_NAME} version={FIRMWARE_VERSION} profile={HARDWARE_PROFILE}",
                "selftest bt=ok sd=ok file=open dac=test-with-t",
                "tracks 1=ok 2=-- 3=ok 4=-- 5=-- 6=-- 7=-- 8=-- 9=--",
                "display 1:paused T1 V18 ALL",
                "display 2:SD:OK WAV:OPEN",
                "display 3:16000Hz 2ch P0%",
                "status=paused track=1 volume=18 order=repeat_all rate=16000Hz channels=2 progress=0",
                "link rx=6 status=1 display=1 bad=0 last=k uptime=1234ms",
            ],
        ),
        DiagnosticCase(
            name="fragmented android stream",
            chunks=["p+", "+n", "b-", "m", "m", "o", "3", "?"],
            expected_tail=[
                "open TRACK03.WAV",
                "status=playing track=3 volume=19 order=repeat_one rate=16000Hz channels=2 progress=0",
                "display 1:playing T3 V19 ONE",
                "display 2:SD:OK WAV:OPEN",
                "display 3:16000Hz 2ch P0%",
                "status=playing track=3 volume=19 order=repeat_one rate=16000Hz channels=2 progress=0",
            ],
            expected_state={"mode": "playing", "track": 3, "volume": 19, "order": "repeat_one"},
        ),
        DiagnosticCase(
            name="uppercase aliases",
            chunks=["PNBMTIELDO?"],
            expected_tail=[
                "order=repeat_one",
                "status=playing track=1 volume=0 order=repeat_one rate=16000Hz channels=2 progress=0",
                "display 1:playing T1 V0 ONE",
                "display 2:SD:OK WAV:OPEN",
                "display 3:16000Hz 2ch P0%",
                "status=playing track=1 volume=0 order=repeat_one rate=16000Hz channels=2 progress=0",
            ],
            expected_state={"mode": "playing", "track": 1, "volume": 0, "order": "repeat_one"},
        ),
        DiagnosticCase(
            name="noise and line endings ignored",
            chunks=["\r\n", "x", "\t", "?", "#", "\n"],
            expected_tail=[
                "status=paused track=1 volume=18 order=repeat_all rate=16000Hz channels=2 progress=0",
            ],
            expected_state={"mode": "paused", "track": 1, "volume": 18, "order": "repeat_all"},
        ),
    ]
    return [run_case(case) for case in cases]


def row(columns: list[object]) -> str:
    return "| " + " | ".join(str(column).replace("|", "\\|") for column in columns) + " |"


def render_report(results: list[DiagnosticResult]) -> str:
    lines = [
        "# Bluetooth UART Diagnostic Report",
        "",
        "This report is generated by `tools/bluetooth_diagnostic_sim.py`. It verifies the HC-05 byte-stream command contract before real hardware pairing. Physical UART voltage, pairing latency, and RF stability still require board testing.",
        "",
        row(["Case", "Input chunks", "Final state", "Transcript"]),
        row(["---", "---", "---", "---"]),
    ]
    for result in results:
        final_state = f"mode={result.mode}, track={result.track}, volume={result.volume}, order={result.order}"
        lines.append(row([result.name, result.input_repr, final_state, " -> ".join(result.transcript)]))

    lines.extend(
        [
            "",
            "## Real Board Bring-Up Commands",
            "",
            row(["Step", "Send", "Expected response"]),
            row(["---", "---", "---"]),
            row(["Version and wiring", "`i`", f"`info name={FIRMWARE_NAME} version={FIRMWARE_VERSION} ...`"]),
            row(["Software-visible self-test", "`e`", "`selftest bt=ok sd=ok file=open dac=test-with-t`"]),
            row(["Track scan", "`l`", "`tracks 1=ok ...` with available WAV files marked `ok`"]),
            row(["Display model", "`d`", "`display 1:`/`display 2:`/`display 3:` lines"]),
            row(["Current status", "`?`", "`status=... track=... volume=... order=... progress=...`"]),
            row(["Bluetooth link counters", "`k`", "`link rx=... status=... display=... bad=... last=... uptime=...ms`"]),
            "",
        ]
    )
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT, help="Markdown report path")
    args = parser.parse_args()

    results = run_diagnostic_cases()
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(render_report(results), encoding="utf-8", newline="\n")

    print("Bluetooth UART diagnostic simulation passed")
    for result in results:
        print(f"{result.name}: {result.mode} track={result.track} volume={result.volume} order={result.order}")
    print(f"diagnostic report generated: {args.report}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
