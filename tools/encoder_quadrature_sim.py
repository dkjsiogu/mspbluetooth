"""Simulate EC11 quadrature and push-button decoding.

The whole-board scenario works with abstract events such as ``enc:cw``. This
script goes one layer lower and feeds phase-A/phase-B samples into the same
transition table used by drivers/encoder.c, then checks that one mechanical
detent becomes exactly one player event.
"""

from __future__ import annotations

from dataclasses import dataclass, field


ENCODER_DEBOUNCE_TICKS = 5
ENCODER_LONG_PRESS_TICKS = 400
TRANSITION_TABLE = [
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0,
]


@dataclass
class EncoderDecoder:
    """EncoderDecoder: host model of the firmware EC11 state variables."""

    last_ab: int = 0
    quadrature_accum: int = 0
    button_sample: int = 0
    button_stable: int = 0
    button_ticks: int = 0
    button_hold_ticks: int = 0
    button_long_sent: int = 0
    events: list[str] = field(default_factory=list)

    def poll(self, now_ab: int, button_pressed: int = 0) -> None:
        index = ((self.last_ab << 2) | now_ab) & 0x0F
        step = TRANSITION_TABLE[index]
        self.last_ab = now_ab

        if step != 0:
            self.quadrature_accum += step
            if self.quadrature_accum >= 4:
                self.events.append("cw")
                self.quadrature_accum = 0
            elif self.quadrature_accum <= -4:
                self.events.append("ccw")
                self.quadrature_accum = 0

        if button_pressed != self.button_sample:
            self.button_sample = button_pressed
            self.button_ticks = 0
        elif self.button_ticks < ENCODER_DEBOUNCE_TICKS:
            self.button_ticks += 1
            if self.button_ticks >= ENCODER_DEBOUNCE_TICKS and self.button_stable != self.button_sample:
                self.button_stable = self.button_sample
                if self.button_stable != 0:
                    self.button_hold_ticks = 0
                    self.button_long_sent = 0
                elif self.button_long_sent == 0:
                    self.events.append("button")
        elif self.button_stable != 0 and self.button_long_sent == 0:
            if self.button_hold_ticks < ENCODER_LONG_PRESS_TICKS:
                self.button_hold_ticks += 1
            if self.button_hold_ticks >= ENCODER_LONG_PRESS_TICKS:
                self.events.append("button_long")
                self.button_long_sent = 1


def run_sequence(sequence: list[int]) -> list[str]:
    decoder = EncoderDecoder(last_ab=sequence[0])
    for sample in sequence[1:]:
        decoder.poll(sample)
    return decoder.events


def run_button_sequence(samples: list[int]) -> list[str]:
    decoder = EncoderDecoder()
    for sample in samples:
        decoder.poll(0, sample)
    return decoder.events


def main() -> int:
    cw_detent = [0, 2, 3, 1, 0]
    ccw_detent = [0, 1, 3, 2, 0]
    back_and_forth = [0, 2, 0, 1, 0]
    bounce_then_short_press = [0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0]
    long_press = [0] + [1] * (ENCODER_DEBOUNCE_TICKS + ENCODER_LONG_PRESS_TICKS + 3) + [0] * (ENCODER_DEBOUNCE_TICKS + 2)

    assert run_sequence(cw_detent) == ["cw"]
    assert run_sequence(ccw_detent) == ["ccw"]
    assert run_sequence(back_and_forth) == []
    assert run_button_sequence(bounce_then_short_press) == ["button"]
    assert run_button_sequence(long_press) == ["button_long"]

    print("encoder quadrature simulation passed")
    print("cw detent -> cw")
    print("ccw detent -> ccw")
    print("back-and-forth jitter -> no event")
    print("debounced short switch -> button")
    print("held switch -> button_long")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
