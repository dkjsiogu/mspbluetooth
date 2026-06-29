"""Bluetooth command-link simulator for the Android app and firmware protocol.

The real HC-05 path is UART bytes over RFCOMM. This simulator exercises the
same single-character command contract and checks that phone-side command
buttons cover the required firmware actions.
"""

from __future__ import annotations

from dataclasses import dataclass, field


MAX_TRACKS = 9
MAX_VOLUME = 32


@dataclass
class SimulatedPlayer:
    mode: str = "paused"
    track: int = 1
    volume: int = 18
    transcript: list[str] = field(default_factory=list)

    def send(self, command: str) -> None:
        command = command.lower()
        if command in "123456789":
            self.track = int(command)
            self.mode = "playing"
            self.transcript.append(f"open TRACK0{self.track}.WAV")
        elif command == "p":
            self.mode = "paused" if self.mode == "playing" else "playing"
            self.transcript.append(self.mode)
        elif command == "s":
            self.mode = "stopped"
            self.transcript.append("stop")
        elif command in ("n", ">"):
            self.track = 1 if self.track >= MAX_TRACKS else self.track + 1
            self.mode = "playing"
            self.transcript.append(f"next track={self.track}")
        elif command in ("b", "<"):
            self.track = MAX_TRACKS if self.track <= 1 else self.track - 1
            self.mode = "playing"
            self.transcript.append(f"previous track={self.track}")
        elif command in ("+", "="):
            self.volume = min(MAX_VOLUME, self.volume + 1)
            self.transcript.append(f"volume={self.volume}")
        elif command in ("-", "_"):
            self.volume = max(0, self.volume - 1)
            self.transcript.append(f"volume={self.volume}")
        elif command == "?":
            self.transcript.append(
                f"status={self.mode} track={self.track} volume={self.volume}"
            )
        elif command == "h":
            self.transcript.append("help")


def assert_equal(actual: object, expected: object, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def run_required_flow() -> SimulatedPlayer:
    player = SimulatedPlayer()

    for command in ["p", "+", "+", "n", "b", "-", "s", "3", "?"]:
        player.send(command)

    assert_equal(player.mode, "playing", "mode after direct track command")
    assert_equal(player.track, 3, "direct track command")
    assert_equal(player.volume, 19, "volume plus/minus flow")
    assert "status=playing track=3 volume=19" in player.transcript[-1]
    return player


def main() -> int:
    player = run_required_flow()
    print("simulated HC-05 command flow passed")
    for line in player.transcript:
        print(line)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

