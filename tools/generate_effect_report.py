"""Generate a software-effect acceptance report from executable simulations.

The report is meant for course review before real board testing. It gathers the
actual host-side simulation results for Bluetooth commands, whole-board control,
local button timing, display preview, Android command coverage, and TF WAV
assets into one markdown file.
"""

from __future__ import annotations

import argparse
from pathlib import Path

from android_ui_parser_sim import run_fragmented_flow
from android_command_coverage import verify_coverage
from audio_stream_sim import run_audio_stream_simulation
from bluetooth_diagnostic_sim import run_diagnostic_cases
from bluetooth_protocol_sim import run_order_flow, run_required_flow
from board_scenario_sim import BoardState, apply_event
from end_to_end_demo_sim import run_demo
from encoder_quadrature_sim import run_button_sequence, run_sequence
from local_button_sim import LONG_PRESS_TICKS, run_samples
from wav_asset_check import DEFAULT_INPUT, WavAsset, find_tracks, parse_wav


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REPORT = ROOT / "docs" / "effect_acceptance_report.md"


def run_board_scenario() -> tuple[list[str], BoardState]:
    scenario = [
        "enc:cw",
        "enc:cw",
        "s1",
        "s1",
        "s1:long",
        "s1",
        "s4",
        "bt:n",
        "s2",
        "bt:-",
        "s2:long",
        "enc:cw",
        "enc:cw",
        "enc:cw",
        "bt:t",
        "bt:r",
        "s4:long",
        "enc:press",
        "tick:500ms",
        "tick:5s",
    ]
    state = BoardState()
    for event in scenario:
        apply_event(state, event)

    if state != BoardState(
        mode="paused",
        track=2,
        volume=3,
        order="repeat_one",
        led_toggles=1,
        status_reports=19,
        display_reports=18,
        tone_tests=1,
    ):
        raise AssertionError(f"unexpected board scenario result: {state}")
    return scenario, state


def run_button_cases() -> list[tuple[str, list[str], list[str]]]:
    short_press = [0] * 6 + [1] * 20 + [0] * 8
    long_press = [0] * 6 + [1] * (LONG_PRESS_TICKS + 16) + [0] * 8
    bounce_then_short = [0, 1, 0, 1, 0, 1, 1, 1, 1, 1] + [0] * 8
    cases = [
        ("S1 short press", ["play_pause"], run_samples(short_press, "play_pause", "stop")),
        ("S1 long press", ["stop"], run_samples(long_press, "play_pause", "stop")),
        ("S2 bounce then short", ["previous"], run_samples(bounce_then_short, "previous", "mute")),
    ]
    for name, expected, actual in cases:
        if actual != expected:
            raise AssertionError(f"{name}: expected {expected}, got {actual}")
    return cases


def run_encoder_cases() -> list[tuple[str, list[str], list[str]]]:
    cases = [
        ("EC11 clockwise detent", ["cw"], run_sequence([0, 2, 3, 1, 0])),
        ("EC11 counter-clockwise detent", ["ccw"], run_sequence([0, 1, 3, 2, 0])),
        ("EC11 back-and-forth jitter", [], run_sequence([0, 2, 0, 1, 0])),
        ("EC11 switch debounce", ["button"], run_button_sequence([0, 1, 0, 1, 1, 1, 1, 1, 1])),
    ]
    for name, expected, actual in cases:
        if actual != expected:
            raise AssertionError(f"{name}: expected {expected}, got {actual}")
    return cases


def android_source_evidence() -> tuple[list[str], list[str]]:
    source = (ROOT / "android" / "app" / "src" / "main" / "java" / "com" / "dkjsiogu" / "mspbluetooth" / "MainActivity.java").read_text(
        encoding="utf-8"
    )
    commands = ["p", "s", "r", "n", "b", "+", "-", "m", "o", "t", "i", "e", "l", "d", "?"]
    missing_commands = [command for command in commands if f'"{command}"' not in source]
    parser_markers = ["status=", "progress=", "display 1:", "display 2:", "display 3:"]
    missing_markers = [marker for marker in parser_markers if f'"{marker}"' not in source]
    if missing_commands:
        raise AssertionError(f"Android source missing command buttons: {missing_commands}")
    if missing_markers:
        raise AssertionError(f"Android source missing parser markers: {missing_markers}")
    return commands, parser_markers


def parse_assets(input_dir: Path) -> list[WavAsset]:
    return [parse_wav(path) for path in find_tracks(input_dir)]


def row(columns: list[object]) -> str:
    return "| " + " | ".join(str(column).replace("|", "\\|") for column in columns) + " |"


def render_report(input_dir: Path) -> str:
    player = run_required_flow()
    run_order_flow()
    diagnostic_results = run_diagnostic_cases()
    demo_snapshots = run_demo()
    scenario, board_state = run_board_scenario()
    button_cases = run_button_cases()
    encoder_cases = run_encoder_cases()
    android_state = run_fragmented_flow()
    android_buttons, _, _ = verify_coverage()
    audio_config, audio_results = run_audio_stream_simulation(input_dir)
    commands, parser_markers = android_source_evidence()
    wav_assets = parse_assets(input_dir)

    display_lines = [line for line in player.transcript if line.startswith("display ")]
    status_line = next(line for line in reversed(player.transcript) if line.startswith("status="))

    lines = [
        "# Software Effect Acceptance Report",
        "",
        "This report is generated from host-side simulations and source checks. It proves the software-visible effects before physical flashing; real HC-05 pairing, TF-card electrical I/O, DAC output, amplifier output, and EC11 feel still require board testing.",
        "",
        "## Bluetooth Command Chain",
        "",
        row(["Effect", "Evidence"]),
        row(["---", "---"]),
        row(["Android command buttons", ", ".join(commands)]),
        row(["APK source command coverage", f"{sum(len(labels) for labels in android_buttons.values())} buttons verified in docs/android_command_coverage_report.md"]),
        row(["Firmware command transcript", " -> ".join(player.transcript)]),
        row(["Final status line", status_line]),
        row(["End-to-end demo", f"{len(demo_snapshots)} APK button steps verified in docs/end_to_end_demo_report.md"]),
        "",
        "## Bluetooth UART Diagnostics",
        "",
        row(["Case", "Final state", "Transcript tail"]),
        row(["---", "---", "---"]),
    ]
    for result in diagnostic_results:
        final_state = f"mode={result.mode}, track={result.track}, volume={result.volume}, order={result.order}"
        transcript_tail = " -> ".join(result.transcript[-4:])
        lines.append(row([result.name, final_state, transcript_tail]))

    lines.extend(
        [
            "",
            "## Whole-Board Control Scenario",
            "",
            row(["Input sequence", "Final state"]),
            row(["---", "---"]),
            row([", ".join(scenario), board_state]),
            "",
            "## Local Button Timing",
            "",
            row(["Case", "Expected event", "Actual event"]),
            row(["---", "---", "---"]),
        ]
    )
    for name, expected, actual in button_cases:
        lines.append(row([name, ", ".join(expected), ", ".join(actual)]))

    lines.extend(
        [
            "",
            "## EC11 Encoder Signal Decoding",
            "",
            row(["Case", "Expected event", "Actual event"]),
            row(["---", "---", "---"]),
        ]
    )
    for name, expected, actual in encoder_cases:
        lines.append(row([name, ", ".join(expected) if expected else "none", ", ".join(actual) if actual else "none"]))

    lines.extend(
        [
            "",
            "## Display And APK Parsing",
            "",
            row(["Effect", "Evidence"]),
            row(["---", "---"]),
            row(["Display frame", " / ".join(display_lines)]),
            row(["Android parser markers", ", ".join(parser_markers)]),
            row(["Android parsed dashboard", android_state.dashboard_text.replace("\n", " / ")]),
            row(["Android parsed display", android_state.display_text.replace("\n", " / ")]),
            row(["Android parsed track list", android_state.track_list_text.replace("\n", " / ")]),
            row(["E-paper preview", "tools/epaper_preview_sim.py renders and checks a nonblank 296x128 PGM frame"]),
            row(["E-paper gallery", "playing, paused, stopped, and error previews are generated in docs/epaper_gallery_report.md"]),
            "",
            "## Audio Stream Simulation",
            "",
            f"Firmware stream buffer: {audio_config.buffer_bytes} bytes. Volume scale: {audio_config.default_volume}/{audio_config.volume_max}.",
            "",
            row(["File", "Chunks", "Output frames", "Peak", "Final progress"]),
            row(["---", "---", "---", "---", "---"]),
        ]
    )
    for result in audio_results:
        lines.append(
            row(
                [
                    result.file_name,
                    result.chunks,
                    result.output_frames,
                    f"{result.min_sample}..{result.max_sample}",
                    f"{result.final_progress}%",
                ]
            )
        )

    lines.extend(
        [
            "",
            "## I2S Frame Simulation",
            "",
            row(["Effect", "Evidence"]),
            row(["---", "---"]),
            row(["PCM5102A frame layout", "docs/i2s_frame_report.md verifies 64 BCK pulses, LRCK left/right slots, one-bit MSB delay, and 15 padding bits"]),
            "",
            "## TF WAV Assets",
            "",
            row(["File", "Channels", "Rate", "Bits", "Frames", "Data bytes"]),
            row(["---", "---", "---", "---", "---", "---"]),
        ]
    )
    for asset in wav_assets:
        lines.append(
            row(
                [
                    asset.path.name,
                    asset.channels,
                    asset.sample_rate,
                    asset.bits_per_sample,
                    asset.frames,
                    asset.data_bytes,
                ]
            )
        )

    lines.extend(
        [
            "",
            "## Remaining Physical Checks",
            "",
            "- Flash `Debug/mspbluetooth.out` to MSP430F5529.",
            "- Pair Android phone with HC-05 and confirm command/response latency.",
            "- Confirm TF-card mount and real WAV stream from the course wiring.",
            "- Confirm PCM5102A analog output, PAM8403 speaker output, and 3.5mm headphone output.",
            "- Confirm EC11 detent direction and S1/S2/S4 long-press timing on the physical buttons.",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, default=DEFAULT_INPUT, help="Directory containing TRACKxx.WAV files")
    parser.add_argument("--output", type=Path, default=DEFAULT_REPORT, help="Markdown report path")
    args = parser.parse_args()

    report = render_report(args.input)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(report, encoding="utf-8", newline="\n")
    print(f"software effect report generated: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
