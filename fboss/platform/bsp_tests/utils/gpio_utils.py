import re
from dataclasses import dataclass
from typing import List, Optional

from fboss.platform.bsp_tests.utils.cmd_utils import run_cmd


@dataclass
class GpioDetectResults:
    name: str
    label: str
    lines: int


def gpiodetect(label: Optional[str] = None) -> List[GpioDetectResults]:
    ret: List[GpioDetectResults] = []
    results = run_cmd(["gpiodetect"]).stdout.decode()
    # format of gpiodetect output is:
    # <name> [<label>] (<num_lines> lines)
    for line in results.splitlines():
        match = re.match(r"(\w+)\s+\[(.*?)\]\s+\((.*?) lines\)", line)
        if match:
            name = match.group(1)
            label = match.group(2)
            num_lines = int(match.group(3))
            ret.append(GpioDetectResults(name, label, num_lines))
    if label is None:
        return ret
    else:
        return [x for x in ret if label == x.label]
    return ret
