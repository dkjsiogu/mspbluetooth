"""Whole-board control scenario simulator.

This validates that the intended physical controls do not fight each other:
Bluetooth controls transport, EC11 controls volume/play-pause/stop, and
S1/S2/S4 provide local fallback transport keys. It mirrors the firmware mapping
rather than emulating MSP430 registers.
"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass
class BoardState:
    mode: str = "playing"
    track: int = 1
    volume: int = 18
    order: str = "repeat_all"
    led_toggles: int = 0
    status_reports: int = 0
    display_reports: int = 0
    tone_tests: int = 0
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


def apply_event(state: BoardState, event: str) -> None:
    reports_status = event.startswith(("bt:", "enc:")) or event in ("s1", "s2", "s4", "s1:long", "s2:long", "s4:long")

    if event in ("bt:p", "enc:press", "s1"):
        if event == "enc:press":
            state.input_encoder_button += 1
        elif event == "s1":
            state.input_s1_short += 1
        state.mode = "paused" if state.mode == "playing" else "playing"
    elif event == "enc:long":
        state.input_encoder_long += 1
        state.mode = "stopped"
    elif event == "s1:long":
        state.input_s1_long += 1
        state.mode = "stopped"
    elif event in ("bt:+", "enc:cw"):
        if event == "enc:cw":
            state.input_encoder_cw += 1
        state.volume = min(32, state.volume + 1)
    elif event in ("bt:-", "enc:ccw"):
        if event == "enc:ccw":
            state.input_encoder_ccw += 1
        state.volume = max(0, state.volume - 1)
    elif event == "s2:long":
        state.input_s2_long += 1
        state.volume = 0
    elif event == "bt:t":
        state.tone_tests += 1
        state.mode = "paused"
    elif event == "bt:r":
        state.mode = "playing"
    elif event == "bt:o":
        if state.order == "sequence":
            state.order = "repeat_all"
        elif state.order == "repeat_all":
            state.order = "repeat_one"
        else:
            state.order = "sequence"
    elif event == "s4:long":
        state.input_s4_long += 1
        if state.order == "sequence":
            state.order = "repeat_all"
        elif state.order == "repeat_all":
            state.order = "repeat_one"
        else:
            state.order = "sequence"
    elif event in ("bt:n", "s4"):
        if event == "s4":
            state.input_s4_short += 1
        state.track = 1 if state.track >= 9 else state.track + 1
        state.mode = "playing"
    elif event in ("bt:b", "s2"):
        if event == "s2":
            state.input_s2_short += 1
        state.track = 9 if state.track <= 1 else state.track - 1
        state.mode = "playing"
    elif event == "tick:500ms":
        state.led_toggles += 1
    elif event == "tick:5s":
        state.status_reports += 1
    else:
        raise ValueError(f"unknown event: {event}")

    if reports_status:
        state.status_reports += 1
        state.display_reports += 1


def main() -> int:
    state = BoardState()
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
        "enc:long",
        "s1",
        "enc:press",
        "tick:500ms",
        "tick:5s",
    ]

    for event in scenario:
        apply_event(state, event)

    assert state.mode == "paused", state
    assert state.track == 2, state
    assert state.volume == 3, state
    assert state.order == "repeat_one", state
    assert state.led_toggles == 1, state
    assert state.status_reports == 21, state
    assert state.display_reports == 20, state
    assert state.tone_tests == 1, state
    assert state.input_encoder_cw == 5, state
    assert state.input_encoder_ccw == 0, state
    assert state.input_encoder_button == 1, state
    assert state.input_encoder_long == 1, state
    assert state.input_s1_short == 4, state
    assert state.input_s1_long == 1, state
    assert state.input_s2_short == 1, state
    assert state.input_s2_long == 1, state
    assert state.input_s4_short == 1, state
    assert state.input_s4_long == 1, state

    print("whole-board control scenario passed")
    print(state)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
