# pyre-unsafe
import subprocess

from typing import Union


def run_cmd(
    cmd: Union[str, list[str]],
    **kwargs,  # pyre-ignore kwargs
):  # pyre-ignore Result (subprocess doesn't implement typing)
    result = subprocess.run(cmd, capture_output=True, **kwargs)
    return result


def check_cmd(cmd: Union[str, list[str]], **kwargs) -> None:
    result = subprocess.run(cmd, capture_output=True, **kwargs)
    ret_code = result.returncode
    stdout = result.stdout.decode("utf-8")  # pyre-ignore
    stderr = result.stderr.decode("utf-8")

    if ret_code != 0:
        raise Exception(
            f"Command failed: `{' '.join(cmd)}`, stdout={stdout}, stderr={stderr}"
        )
