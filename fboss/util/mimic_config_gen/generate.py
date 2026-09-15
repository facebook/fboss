# Copyright (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

# pyre-strict

"""Drive off-box coop generation via coop_test.

coop_test already knows how to run the production generators against a supplied
netwhoami and local input overrides, so we shell out to it rather than
re-hosting the generators. Nothing here touches a switch.
"""

from __future__ import annotations

import logging
import pathlib
import re
import shutil
import subprocess
import typing as t

from fboss.util.mimic_config_gen.defs import COOP_TEST_TARGET, MimicError

logger: logging.Logger = logging.getLogger(__name__)

_BUCK_MODE = "@fbcode//mode/opt"
_DEFAULT_TIMEOUT = 3600


def _run(cmd: list[str], cwd: pathlib.Path, timeout: int) -> tuple[int, str]:
    logger.info("running: %s", " ".join(cmd))
    try:
        proc = subprocess.run(
            cmd,
            cwd=str(cwd),
            # Merge the streams so the saved log preserves interleaving; reading
            # every print() before every log line misleads whoever we point at it.
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            # buck2 can prompt for auth; without this it would block until the
            # timeout with no indication why.
            stdin=subprocess.DEVNULL,
            text=True,
            timeout=timeout,
            check=False,
            # Own process group so a timeout can kill the whole tree, not just
            # the buck2 wrapper.
            start_new_session=True,
        )
    except subprocess.TimeoutExpired as e:
        raise MimicError(f"timed out after {timeout}s running: {' '.join(cmd)}") from e
    return proc.returncode, proc.stdout or ""


def fbcode_root() -> pathlib.Path:
    """Locate the fbcode source root.

    Name-matching alone is not enough: under `buck2 run` this module lives
    inside `buck-out/v2/gen/fbcode/...`, whose path contains an `fbcode`
    component that is not the source root.
    """
    candidates = [pathlib.Path(__file__).resolve(), pathlib.Path.cwd().resolve()]
    for start in candidates:
        for parent in (start, *start.parents):
            if parent.name != "fbcode" or "buck-out" in parent.parts:
                continue
            if (parent.parent / ".buckconfig").is_file() or (parent / "fboss").is_dir():
                return parent
    raise MimicError("could not locate the fbcode root; run from inside fbcode")


def _reset(out_dir: pathlib.Path) -> None:
    """Start from an empty coop dir.

    coop does not clean the directory it is handed, and it applies every file
    under `local_overrides/`. A leftover override from an earlier run -- a
    different target hardware, or a feature name that is no longer pinned --
    would be silently reapplied, producing a config assembled from two runs.
    """
    shutil.rmtree(out_dir, ignore_errors=True)
    out_dir.mkdir(parents=True, exist_ok=True)


def baseline(
    device: str, out_dir: pathlib.Path, timeout: int = _DEFAULT_TIMEOUT
) -> tuple[bool, str]:
    """Generate the real config for the source device.

    Serves three purposes: it is the reference to diff against, it writes the
    device's real netwhoami as thrift JSON for us to respin, and it records the
    feature states we will pin. `--no-rsync` fetches identity from the
    netwhoami service, so no switch is contacted.
    """
    _reset(out_dir)
    rc, log = _run(
        [
            "buck2",
            "run",
            _BUCK_MODE,
            COOP_TEST_TARGET,
            "--",
            "--switch",
            device,
            "--no-rsync",
            "--no-diff",
            "--coop-dir",
            str(out_dir),
        ],
        fbcode_root(),
        timeout,
    )
    return rc == 0, log


def mimic(
    whoami_path: pathlib.Path,
    out_dir: pathlib.Path,
    input_overrides: t.Mapping[str, pathlib.Path],
    feature_pins: t.Mapping[str, str],
    timeout: int = _DEFAULT_TIMEOUT,
) -> tuple[bool, str]:
    """Generate the target config from the forged identity."""
    _reset(out_dir)
    cmd = [
        "buck2",
        "run",
        _BUCK_MODE,
        COOP_TEST_TARGET,
        "--",
        "--netwhoami-path",
        str(whoami_path),
        "--no-diff",
        "--coop-dir",
        str(out_dir),
    ]
    if input_overrides:
        cmd += [
            "--input-overrides",
            ",".join(f"{n}:{p}" for n, p in sorted(input_overrides.items())),
        ]
    if feature_pins:
        cmd += [
            "--feature-overrides",
            ",".join(f"{n}:{s}" for n, s in sorted(feature_pins.items())),
        ]
    rc, log = _run(cmd, fbcode_root(), timeout)
    return rc == 0, log


def failure_reason(log: str) -> str:
    """Extract the useful line from a coop_test traceback.

    A mimic that fails is still informative -- the exception usually names the
    exact incompatibility between the two platforms -- so surface it rather
    than making the caller read the whole log.
    """
    # Anchor on a Python exception line. A substring test for "Error:" also
    # matches glog output from C++ teardown, which trails the real traceback
    # and would therefore win.
    exception_line = re.compile(r"^(\w+(\.\w+)*(Error|Exception))(:|$)")
    interesting = [
        stripped
        for line in log.splitlines()
        if exception_line.match(stripped := line.strip())
    ]
    return interesting[-1] if interesting else "see log for details"
