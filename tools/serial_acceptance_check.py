"""Check HC-05 serial acceptance transcripts for classroom bring-up evidence.

The checker can validate a real serial-assistant log captured from the board,
or generate and validate a deterministic software transcript when no hardware
log is available. It complements the protocol simulations by giving the final
demo a concrete text artifact to save and re-check.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path

from bluetooth_protocol_sim import SimulatedPlayer


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REPORT = ROOT / "docs" / "serial_acceptance_report.md"
DEFAULT_SAMPLE = ROOT / "dist" / "verification" / "serial_acceptance_sample.txt"

STATE_CHANGING_COMMANDS = ("p", "+", "n", "b", "o", "t", "3")


@dataclass(frozen=True)
class Criterion:
    label: str
    needle: str
    note: str


@dataclass
class CheckResult:
    label: str
    passed: bool
    evidence: str
    note: str


REQUIRED_CRITERIA = [
    Criterion("TF card mounted", "sd mounted", "boot path can see the storage layer"),
    Criterion("Help prompt", "help", "operator can discover the command set"),
    Criterion("Firmware info", "info name=MSP430F5529-BT-WAV", "firmware identity is visible"),
    Criterion("Self-test", "selftest bt=ok", "software-visible link self-test passes"),
    Criterion("Track scan", "tracks 1=ok", "TRACKxx.WAV scan is reported"),
    Criterion("Display line 1", "display 1:", "display model first row is reported"),
    Criterion("Display line 2", "display 2:", "display model second row is reported"),
    Criterion("Display line 3", "display 3:", "display model third row is reported"),
    Criterion("Status line", "status=", "structured status can feed APK/dashboard"),
    Criterion("Bluetooth link counters", "link rx=", "round-trip command counters are visible"),
    Criterion("Local input counters", "input ecw=", "EC11 and S1/S2/S4 event counters are visible"),
    Criterion("DAC tone start", "tone start", "test-tone command enters DAC path"),
    Criterion("DAC tone done", "tone done", "test-tone command completes"),
    Criterion("Track open", "open TRACK0", "WAV open action is visible"),
]


def normalize_lines(text: str) -> list[str]:
    """Returns stripped non-empty transcript lines with stable line endings."""

    return [line.strip() for line in text.replace("\r\n", "\n").replace("\r", "\n").split("\n") if line.strip()]


def build_sample_transcript() -> str:
    """Builds the no-hardware acceptance transcript from the protocol model."""

    player = SimulatedPlayer()
    lines = [
        "boot MSP430F5529-BT-WAV",
        "sd mounted",
        "open TRACK01.WAV",
    ]

    for command in ("h", "i", "e", "l", "d", "?", *STATE_CHANGING_COMMANDS, "k", "u"):
        before = len(player.transcript)
        player.send(command)
        lines.append(f"TX> {command}")
        lines.extend(f"RX> {line}" for line in player.transcript[before:])
    return "\n".join(lines) + "\n"


def line_contains(lines: list[str], needle: str) -> str:
    """Returns the first line containing needle, or an empty string."""

    for line in lines:
        if needle in line:
            return line
    return ""


def check_required_criteria(lines: list[str]) -> list[CheckResult]:
    """Checks global transcript markers required for acceptance evidence."""

    results: list[CheckResult] = []
    for criterion in REQUIRED_CRITERIA:
        evidence = line_contains(lines, criterion.needle)
        results.append(CheckResult(criterion.label, evidence != "", evidence or "missing", criterion.note))

    track_open = line_contains(lines, "open TRACK01.WAV") or line_contains(lines, "open TRACK03.WAV")
    results.append(
        CheckResult(
            "Known WAV opened",
            track_open != "",
            track_open or "missing open TRACK01.WAV/open TRACK03.WAV",
            "direct or startup track open can be confirmed",
        )
    )
    return results


def command_windows(lines: list[str]) -> dict[str, list[str]]:
    """Splits a transcript into TX-marked command response windows."""

    windows: dict[str, list[str]] = {}
    current_command = ""
    current_lines: list[str] = []

    def flush() -> None:
        if current_command:
            windows.setdefault(current_command, []).extend(current_lines)

    for line in lines:
        command = ""
        lowered = line.lower()
        for prefix in ("tx>", "send>", "cmd>", "command>"):
            if lowered.startswith(prefix):
                command = line[len(prefix) :].strip().lower()[:1]
                break

        if command:
            flush()
            current_command = command
            current_lines = []
        elif current_command:
            current_lines.append(line)

    flush()
    return windows


def check_state_command_snapshots(lines: list[str]) -> list[CheckResult]:
    """Checks that state-changing commands return status and display snapshots."""

    windows = command_windows(lines)
    if not windows:
        status_count = sum(1 for line in lines if "status=" in line)
        display1_count = sum(1 for line in lines if "display 1:" in line)
        passed = status_count >= 3 and display1_count >= 3
        return [
            CheckResult(
                "State command snapshots",
                passed,
                f"status lines={status_count}, display-1 lines={display1_count}",
                "no TX markers found; used global repeated-snapshot evidence",
            )
        ]

    results: list[CheckResult] = []
    for command in STATE_CHANGING_COMMANDS:
        window = windows.get(command, [])
        has_status = any("status=" in line for line in window)
        has_display = all(any(f"display {index}:" in line for line in window) for index in (1, 2, 3))
        results.append(
            CheckResult(
                f"Command {command} snapshot",
                has_status and has_display,
                " -> ".join(window) if window else "missing TX window",
                "state-changing commands should update APK panels without an extra query",
            )
        )
    return results


def row(columns: list[object]) -> str:
    """Formats one Markdown table row."""

    return "| " + " | ".join(str(column).replace("|", "\\|").replace("\n", " / ") for column in columns) + " |"


def render_report(source_name: str, lines: list[str], results: list[CheckResult]) -> str:
    """Renders a Markdown report for saved course evidence."""

    passed = sum(1 for result in results if result.passed)
    lines_out = [
        "# Serial Acceptance Transcript Report",
        "",
        "Generated by `tools/serial_acceptance_check.py`. It checks the text evidence expected from an HC-05 serial assistant or from the deterministic no-hardware transcript. Physical flashing, UART voltage level, TF-card electrical access, and real audio output still require board testing.",
        "",
        f"Transcript source: `{source_name}`",
        f"Checks passed: {passed}/{len(results)}",
        "",
        row(["Criterion", "Result", "Evidence", "Purpose"]),
        row(["---", "---", "---", "---"]),
    ]
    for result in results:
        lines_out.append(row([result.label, "PASS" if result.passed else "FAIL", result.evidence, result.note]))

    lines_out.extend(
        [
            "",
            "## Real Board Capture Procedure",
            "",
            "1. Pair the phone or PC with HC-05 and open a serial/Bluetooth terminal at the firmware UART rate.",
            "2. Enable local echo or manually keep `TX>` command markers in the saved log.",
            "3. Send `h`, `i`, `e`, `l`, `d`, `?`, `t`, `1`, `p`, `+`, `n`, `b`, `o`, `3`, `k`, and `u`.",
            "4. Save the transcript as text and run `python tools\\serial_acceptance_check.py --input path\\to\\capture.txt`.",
            "",
            "## Transcript Preview",
            "",
            "```text",
            *lines[:80],
            "```",
            "",
        ]
    )
    return "\n".join(lines_out)


def run_check(input_path: Path | None, sample_out: Path | None) -> tuple[str, list[str], list[CheckResult]]:
    """Loads or generates a transcript and returns all check results."""

    if input_path:
        source_name = str(input_path)
        text = input_path.read_text(encoding="utf-8", errors="ignore")
    else:
        source_name = "generated sample transcript"
        text = build_sample_transcript()
        if sample_out:
            sample_out.parent.mkdir(parents=True, exist_ok=True)
            sample_out.write_text(text, encoding="utf-8", newline="\n")

    lines = normalize_lines(text)
    results = check_required_criteria(lines)
    results.extend(check_state_command_snapshots(lines))
    return source_name, lines, results


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, help="Real serial-assistant transcript to check")
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT, help="Markdown report path")
    parser.add_argument("--sample-out", type=Path, default=DEFAULT_SAMPLE, help="Generated sample transcript path")
    args = parser.parse_args()

    source_name, lines, results = run_check(args.input, args.sample_out if not args.input else None)
    failures = [result for result in results if not result.passed]

    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(render_report(source_name, lines, results), encoding="utf-8", newline="\n")

    if failures:
        for failure in failures:
            print(f"FAIL {failure.label}: {failure.evidence}")
        print(f"serial acceptance report generated: {args.report}")
        return 1

    print("serial acceptance transcript check passed")
    print(f"checked lines={len(lines)}")
    print(f"serial acceptance report generated: {args.report}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
