"""Audit source readability rules requested for the course project.

The firmware already has functional simulations. This audit focuses on whether
the code remains easy to review in a classroom setting: every project header has
a module comment, public declarations explain their parameters, macros are
documented, and static items have local intent comments.
"""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REPORT = ROOT / "docs" / "readability_report.md"

HEADER_FILES = [
    "application/audio_player.h",
    "drivers/bluetooth_uart.h",
    "drivers/board.h",
    "drivers/board_pins.h",
    "drivers/encoder.h",
    "drivers/epaper_panel.h",
    "drivers/i2s_dac.h",
    "drivers/local_buttons.h",
    "drivers/platform_config.h",
    "fatfs/HAL_SDCard.h",
    "middleware/display_model.h",
    "middleware/wav_reader.h",
]

SOURCE_FILES = [
    "main.c",
    "application/audio_player.c",
    "drivers/bluetooth_uart.c",
    "drivers/board.c",
    "drivers/encoder.c",
    "drivers/epaper_panel.c",
    "drivers/local_buttons.c",
    "drivers/i2s_dac.c",
    "middleware/display_model.c",
    "middleware/wav_reader.c",
    "fatfs/HAL_SDCard.c",
]


@dataclass(frozen=True)
class Finding:
    """Finding: one readability rule result for report output."""

    file: str
    line: int
    rule: str
    result: str
    detail: str


@dataclass(frozen=True)
class AuditSummary:
    """AuditSummary: pass/fail rows plus aggregate coverage counters."""

    findings: list[Finding]
    header_count: int
    macro_count: int
    declaration_count: int
    parameter_count: int
    static_count: int


def read_text(relative: str) -> str:
    """Reads a project file as UTF-8 text."""

    return (ROOT / relative).read_text(encoding="utf-8")


def row(columns: list[object]) -> str:
    """Formats one Markdown table row."""

    return "| " + " | ".join(str(column).replace("|", "\\|").replace("\n", " / ") for column in columns) + " |"


def is_comment_line(line: str) -> bool:
    """Returns true when line is part of a block or line comment."""

    stripped = line.strip()
    return stripped.startswith("/*") or stripped.startswith("*") or stripped.startswith("//") or stripped.endswith("*/")


def previous_nonblank_line(lines: list[str], index: int) -> str:
    """Returns the nearest nonblank line before index, or an empty string."""

    previous_index = index - 1
    while previous_index >= 0 and lines[previous_index].strip() == "":
        previous_index -= 1
    if previous_index < 0:
        return ""
    return lines[previous_index]


def comment_block_before(lines: list[str], index: int) -> str:
    """Returns the contiguous comment block immediately before line index."""

    previous_index = index - 1
    while previous_index >= 0 and lines[previous_index].strip() == "":
        previous_index -= 1
    if previous_index < 0 or not is_comment_line(lines[previous_index]):
        return ""

    block: list[str] = []
    while previous_index >= 0 and is_comment_line(lines[previous_index]):
        block.append(lines[previous_index].strip())
        previous_index -= 1
    block.reverse()
    return "\n".join(block)


def declaration_parameter_names(declaration: str) -> list[str]:
    """Extracts parameter names from a simple C function declaration."""

    match = re.search(r"\((.*)\)", declaration)
    if not match:
        return []

    raw_params = match.group(1).strip()
    if raw_params == "" or raw_params == "void":
        return []

    names: list[str] = []
    for raw_param in raw_params.split(","):
        cleaned = raw_param.strip()
        if cleaned == "...":
            continue
        name_match = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?$", cleaned)
        if name_match:
            names.append(name_match.group(1))
    return names


def has_structured_parameter_comment(comment: str, param_name: str) -> bool:
    """Checks for a parameter description written as 'name:' inside comment."""

    return re.search(rf"\b{re.escape(param_name)}\s*:", comment) is not None


def check_module_header(relative: str, text: str) -> Finding:
    """Checks the file-level module comment at the top of a source file."""

    if text.lstrip().startswith("/*"):
        return Finding(relative, 1, "module-comment", "PASS", "file starts with a module description")
    return Finding(relative, 1, "module-comment", "FAIL", "file must start with a block comment explaining its role")


def audit_headers() -> tuple[list[Finding], int, int, int, int]:
    """Audits project-owned header files."""

    findings: list[Finding] = []
    macro_count = 0
    declaration_count = 0
    parameter_count = 0

    for relative in HEADER_FILES:
        text = read_text(relative)
        lines = text.splitlines()
        findings.append(check_module_header(relative, text))

        for match in re.finditer(r"^#define\s+([A-Za-z_]\w+)\b.*$", text, re.M):
            name = match.group(1)
            if name.endswith("_H"):
                continue
            macro_count += 1
            line_index = text[: match.start()].count("\n")
            if is_comment_line(previous_nonblank_line(lines, line_index)):
                findings.append(Finding(relative, line_index + 1, "macro-comment", "PASS", f"{name} is documented"))
            else:
                findings.append(Finding(relative, line_index + 1, "macro-comment", "FAIL", f"{name} lacks a preceding comment"))

        declaration_pattern = r"^(?:[A-Za-z_][\w\s\*]+\s+)?[A-Za-z_]\w+\([^;{}]*\);"
        for match in re.finditer(declaration_pattern, text, re.M):
            declaration = match.group(0).strip()
            declaration_count += 1
            line_index = text[: match.start()].count("\n")
            comment = comment_block_before(lines, line_index)
            if comment == "":
                findings.append(Finding(relative, line_index + 1, "declaration-comment", "FAIL", declaration))
                continue
            findings.append(Finding(relative, line_index + 1, "declaration-comment", "PASS", declaration))
            for param_name in declaration_parameter_names(declaration):
                parameter_count += 1
                if has_structured_parameter_comment(comment, param_name):
                    findings.append(Finding(relative, line_index + 1, "parameter-comment", "PASS", f"{param_name} described"))
                else:
                    findings.append(Finding(relative, line_index + 1, "parameter-comment", "FAIL", f"{param_name} needs 'param:' style text"))

    return findings, len(HEADER_FILES), macro_count, declaration_count, parameter_count


def audit_sources() -> tuple[list[Finding], int]:
    """Audits source file module comments and static item comments."""

    findings: list[Finding] = []
    static_count = 0

    for relative in SOURCE_FILES:
        text = read_text(relative)
        lines = text.splitlines()
        findings.append(check_module_header(relative, text))

        for index, line in enumerate(lines):
            if not re.match(r"^\s*static\s+", line):
                continue
            static_count += 1
            stripped = line.strip()
            if is_comment_line(previous_nonblank_line(lines, index)):
                findings.append(Finding(relative, index + 1, "static-comment", "PASS", stripped))
            else:
                findings.append(Finding(relative, index + 1, "static-comment", "FAIL", stripped))

    return findings, static_count


def run_audit() -> AuditSummary:
    """Runs all readability checks and returns the summary."""

    header_findings, header_count, macro_count, declaration_count, parameter_count = audit_headers()
    source_findings, static_count = audit_sources()
    return AuditSummary(
        findings=header_findings + source_findings,
        header_count=header_count,
        macro_count=macro_count,
        declaration_count=declaration_count,
        parameter_count=parameter_count,
        static_count=static_count,
    )


def render_report(summary: AuditSummary) -> str:
    """Renders the Markdown audit report."""

    failed = [finding for finding in summary.findings if finding.result == "FAIL"]
    lines = [
        "# Readability Audit Report",
        "",
        "Generated by `tools/readability_audit.py`. The audit covers project-owned firmware headers and source files, excluding upstream FatFs implementation files that should remain vendor-like.",
        "",
        row(["Metric", "Count"]),
        row(["---", "---"]),
        row(["Headers with module comment rule", summary.header_count]),
        row(["Documented macros checked", summary.macro_count]),
        row(["Public declarations checked", summary.declaration_count]),
        row(["Declaration parameters checked", summary.parameter_count]),
        row(["Static items checked", summary.static_count]),
        row(["Failures", len(failed)]),
        "",
        row(["File", "Line", "Rule", "Result", "Detail"]),
        row(["---", "---", "---", "---", "---"]),
    ]
    for finding in summary.findings:
        lines.append(row([finding.file, finding.line, finding.rule, finding.result, finding.detail]))
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    """CLI entry point."""

    parser = argparse.ArgumentParser()
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT, help="Markdown report path")
    args = parser.parse_args()

    summary = run_audit()
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(render_report(summary), encoding="utf-8", newline="\n")

    failed = [finding for finding in summary.findings if finding.result == "FAIL"]
    if failed:
        for finding in failed:
            print(f"FAIL {finding.file}:{finding.line} {finding.rule}: {finding.detail}")
        print(f"readability audit report generated: {args.report}")
        return 1

    print("readability audit passed")
    print(f"headers={summary.header_count} macros={summary.macro_count} declarations={summary.declaration_count} params={summary.parameter_count} statics={summary.static_count}")
    print(f"readability audit report generated: {args.report}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
