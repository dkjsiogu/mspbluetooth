"""Whole-board control scenario simulator.

This validates that the intended physical controls do not fight each other:
Bluetooth controls transport, EC11 controls volume/play-pause, and S1/S2/S4
provide local fallback transport keys. It mirrors the firmware mapping rather
than emulating MSP430 registers.
"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass
class BoardState:
    mode: str = "playing"
    track: int = 1
    volume: int = 18
    led_toggles: int = 0
    status_reports: int = 0


def apply_event(state: BoardState, event: str) -> None:
    if event in ("bt:p", "enc:press", "s1"):
        state.mode = "paused" if state.mode == "playing" else "playing"
    elif event in ("bt:+", "enc:cw"):
        state.volume = min(32, state.volume + 1)
    elif event in ("bt:-", "enc:ccw"):
        state.volume = max(0, state.volume - 1)
    elif event in ("bt:n", "s4"):
        state.track = 1 if state.track >= 9 else state.track + 1
        state.mode = "playing"
    elif event in ("bt:b", "s2"):
        state.track = 9 if state.track <= 1 else state.track - 1
        state.mode = "playing"
    elif event == "tick:500ms":
        state.led_toggles += 1
    elif event == "tick:5s":
        state.status_reports += 1
    else:
        raise ValueError(f"unknown event: {event}")


def main() -> int:
    state = BoardState()
    scenario = [
        "enc:cw",
        "enc:cw",
        "s1",
        "s1",
        "s4",
        "bt:n",
        "s2",
        "bt:-",
        "enc:press",
        "tick:500ms",
        "tick:5s",
    ]

    for event in scenario:
        apply_event(state, event)

    assert state.mode == "paused", state
    assert state.track == 2, state
    assert state.volume == 19, state
    assert state.led_toggles == 1, state
    assert state.status_reports == 1, state

    print("whole-board control scenario passed")
    print(state)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

