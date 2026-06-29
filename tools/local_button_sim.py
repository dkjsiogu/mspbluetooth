"""Local button debounce and long-press simulator.

The firmware scans S1/S2/S4 periodically and should emit either a short event
on release or one long event while the key is held. This host-side model keeps
the timing contract explicit so local controls can be changed without guessing.
"""

from __future__ import annotations

from dataclasses import dataclass


DEBOUNCE_TICKS = 4
LONG_PRESS_TICKS = 160


@dataclass
class ButtonDebounce:
    sample: int = 0
    stable: int = 0
    ticks: int = 0
    hold_ticks: int = 0
    long_sent: int = 0

    def reset_hold(self) -> None:
        self.hold_ticks = 0
        self.long_sent = 0

    def update_hold(self, long_event: str) -> list[str]:
        if self.stable == 0 or self.long_sent != 0:
            return []

        if self.hold_ticks < LONG_PRESS_TICKS:
            self.hold_ticks += 1

        if self.hold_ticks >= LONG_PRESS_TICKS:
            self.long_sent = 1
            return [long_event]

        return []

    def update(self, pressed: int, short_event: str, long_event: str) -> list[str]:
        if pressed != self.sample:
            self.sample = pressed
            self.ticks = 0
            return []

        if self.ticks < DEBOUNCE_TICKS:
            self.ticks += 1
            if self.ticks >= DEBOUNCE_TICKS and self.stable != self.sample:
                self.stable = self.sample
                if self.stable != 0:
                    self.reset_hold()
                elif self.long_sent == 0:
                    return [short_event]
            return []

        return self.update_hold(long_event)


def run_samples(samples: list[int], short_event: str, long_event: str) -> list[str]:
    button = ButtonDebounce()
    events: list[str] = []
    for sample in samples:
        events.extend(button.update(sample, short_event, long_event))
    return events


def main() -> int:
    short_press = [0] * 6 + [1] * 20 + [0] * 8
    long_press = [0] * 6 + [1] * (LONG_PRESS_TICKS + 16) + [0] * 8
    bounce_then_short = [0, 1, 0, 1, 0, 1, 1, 1, 1, 1] + [0] * 8

    assert run_samples(short_press, "play_pause", "stop") == ["play_pause"]
    assert run_samples(long_press, "play_pause", "stop") == ["stop"]
    assert run_samples(bounce_then_short, "previous", "mute") == ["previous"]

    print("local button debounce and long-press simulation passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
