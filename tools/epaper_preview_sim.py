"""Render the firmware display frame into a monochrome e-paper preview.

The default hardware map keeps real e-paper wiring out of the main build
because the audio project already uses the likely SPI/GPIO pins. This simulator
still verifies the visible display effect by rendering the same three short
lines used by the firmware and Android app into a 296x128 black/white PGM file.
"""

from __future__ import annotations

import argparse
from pathlib import Path


WIDTH = 296
HEIGHT = 128
WHITE = 255
BLACK = 0
SCALE = 2


FONT_5X7 = {
    " ": ["00000", "00000", "00000", "00000", "00000", "00000", "00000"],
    "-": ["00000", "00000", "00000", "11110", "00000", "00000", "00000"],
    ".": ["00000", "00000", "00000", "00000", "00000", "01100", "01100"],
    ":": ["00000", "01100", "01100", "00000", "01100", "01100", "00000"],
    "%": ["11001", "11010", "00100", "01000", "10110", "00110", "00000"],
    "/": ["00001", "00010", "00100", "01000", "10000", "00000", "00000"],
    "_": ["00000", "00000", "00000", "00000", "00000", "00000", "11111"],
    "0": ["01110", "10001", "10011", "10101", "11001", "10001", "01110"],
    "1": ["00100", "01100", "00100", "00100", "00100", "00100", "01110"],
    "2": ["01110", "10001", "00001", "00010", "00100", "01000", "11111"],
    "3": ["11110", "00001", "00001", "01110", "00001", "00001", "11110"],
    "4": ["00010", "00110", "01010", "10010", "11111", "00010", "00010"],
    "5": ["11111", "10000", "10000", "11110", "00001", "00001", "11110"],
    "6": ["01110", "10000", "10000", "11110", "10001", "10001", "01110"],
    "7": ["11111", "00001", "00010", "00100", "01000", "01000", "01000"],
    "8": ["01110", "10001", "10001", "01110", "10001", "10001", "01110"],
    "9": ["01110", "10001", "10001", "01111", "00001", "00001", "01110"],
    "A": ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    "B": ["11110", "10001", "10001", "11110", "10001", "10001", "11110"],
    "C": ["01110", "10001", "10000", "10000", "10000", "10001", "01110"],
    "D": ["11110", "10001", "10001", "10001", "10001", "10001", "11110"],
    "E": ["11111", "10000", "10000", "11110", "10000", "10000", "11111"],
    "F": ["11111", "10000", "10000", "11110", "10000", "10000", "10000"],
    "G": ["01110", "10001", "10000", "10111", "10001", "10001", "01110"],
    "H": ["10001", "10001", "10001", "11111", "10001", "10001", "10001"],
    "I": ["01110", "00100", "00100", "00100", "00100", "00100", "01110"],
    "J": ["00111", "00010", "00010", "00010", "10010", "10010", "01100"],
    "K": ["10001", "10010", "10100", "11000", "10100", "10010", "10001"],
    "L": ["10000", "10000", "10000", "10000", "10000", "10000", "11111"],
    "M": ["10001", "11011", "10101", "10101", "10001", "10001", "10001"],
    "N": ["10001", "11001", "10101", "10011", "10001", "10001", "10001"],
    "O": ["01110", "10001", "10001", "10001", "10001", "10001", "01110"],
    "P": ["11110", "10001", "10001", "11110", "10000", "10000", "10000"],
    "Q": ["01110", "10001", "10001", "10001", "10101", "10010", "01101"],
    "R": ["11110", "10001", "10001", "11110", "10100", "10010", "10001"],
    "S": ["01111", "10000", "10000", "01110", "00001", "00001", "11110"],
    "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    "U": ["10001", "10001", "10001", "10001", "10001", "10001", "01110"],
    "V": ["10001", "10001", "10001", "10001", "10001", "01010", "00100"],
    "W": ["10001", "10001", "10001", "10101", "10101", "10101", "01010"],
    "X": ["10001", "10001", "01010", "00100", "01010", "10001", "10001"],
    "Y": ["10001", "10001", "01010", "00100", "00100", "00100", "00100"],
    "Z": ["11111", "00001", "00010", "00100", "01000", "10000", "11111"],
}


def new_canvas() -> list[list[int]]:
    return [[WHITE for _ in range(WIDTH)] for _ in range(HEIGHT)]


def draw_rect(canvas: list[list[int]], x: int, y: int, width: int, height: int) -> None:
    for px in range(x, x + width):
        if 0 <= px < WIDTH:
            if 0 <= y < HEIGHT:
                canvas[y][px] = BLACK
            if 0 <= y + height - 1 < HEIGHT:
                canvas[y + height - 1][px] = BLACK
    for py in range(y, y + height):
        if 0 <= py < HEIGHT:
            if 0 <= x < WIDTH:
                canvas[py][x] = BLACK
            if 0 <= x + width - 1 < WIDTH:
                canvas[py][x + width - 1] = BLACK


def draw_char(canvas: list[list[int]], x: int, y: int, char: str) -> None:
    pattern = FONT_5X7.get(char.upper(), FONT_5X7[" "])
    for row_index, row in enumerate(pattern):
        for col_index, bit in enumerate(row):
            if bit != "1":
                continue
            for dy in range(SCALE):
                for dx in range(SCALE):
                    px = x + col_index * SCALE + dx
                    py = y + row_index * SCALE + dy
                    if 0 <= px < WIDTH and 0 <= py < HEIGHT:
                        canvas[py][px] = BLACK


def draw_text(canvas: list[list[int]], x: int, y: int, text: str) -> None:
    cursor = x
    for char in text[:24]:
        draw_char(canvas, cursor, y, char)
        cursor += 6 * SCALE


def render_frame(lines: list[str]) -> list[list[int]]:
    canvas = new_canvas()
    draw_rect(canvas, 0, 0, WIDTH, HEIGHT)
    draw_rect(canvas, 8, 8, WIDTH - 16, HEIGHT - 16)
    draw_text(canvas, 18, 18, "MSP430 BT WAV")
    draw_text(canvas, 18, 44, lines[0])
    draw_text(canvas, 18, 68, lines[1])
    draw_text(canvas, 18, 92, lines[2])
    return canvas


def write_pgm(canvas: list[list[int]], output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="ascii", newline="\n") as handle:
        handle.write(f"P2\n{WIDTH} {HEIGHT}\n255\n")
        for row in canvas:
            handle.write(" ".join(str(pixel) for pixel in row))
            handle.write("\n")


def assert_nonblank(canvas: list[list[int]]) -> None:
    black_pixels = sum(1 for row in canvas for pixel in row if pixel == BLACK)
    white_pixels = WIDTH * HEIGHT - black_pixels
    if black_pixels < 900:
        raise AssertionError(f"preview has too few black pixels: {black_pixels}")
    if white_pixels < 900:
        raise AssertionError(f"preview has too few white pixels: {white_pixels}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        default="dist/verification/epaper_preview.pgm",
        help="PGM preview path to write",
    )
    args = parser.parse_args()

    frame_lines = [
        "playing T3 V19 ONE",
        "SD:OK WAV:OPEN",
        "16000Hz 2ch P42%",
    ]
    canvas = render_frame(frame_lines)
    assert_nonblank(canvas)
    output = Path(args.output)
    write_pgm(canvas, output)
    print(f"e-paper display preview simulation passed: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
