# pyre-strict

import subprocess
from enum import Enum
from typing import Any, Callable


def indent(text: str, level: int = 1) -> str:
    indention = "  " * level
    return "\n".join(f"{indention}{line}" for line in text.splitlines())


def run_cmd(cmd: str, **kwargs: Any) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        **kwargs,
    )
    return result


def check_cmd(cmd: str, **kwargs: Any) -> bool:
    return run_cmd(cmd, **kwargs).returncode == 0


def overwrite_print(text: str) -> None:
    print("\033[K" + text, end="\r")


def clear_line() -> None:
    print("\033[K", end="\r")


class Color(Enum):
    RED = "\033[91m"
    GREEN = "\033[92m"
    ORANGE = "\033[38;5;208m"
    YELLOW = "\033[93m"


color_to_bg: dict[Color, str] = {
    Color.RED: "\033[41m",
    Color.GREEN: "\033[42m",
    Color.ORANGE: "\033[48;5;208m",
    Color.YELLOW: "\033[43m",
}

StyleFunc = Callable[[str], str]


def color(c: Color) -> StyleFunc:
    return lambda text: f"{c.value}{text}"


def color_bg(c: Color) -> StyleFunc:
    return lambda text: f"{color_to_bg[c]}{text}"


def bold() -> StyleFunc:
    return lambda text: f"\033[1m{text}"


def styled(text: str, *styles: StyleFunc) -> str:
    reset_style = "\033[0m"  # Reset all stying
    for style in styles:
        text = style(text)
    return f"{text}{reset_style}"
