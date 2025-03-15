# pyre-strict
import argparse
import os

import pytest

PLATFORMS = ["meru800bia", "meru800bfa"]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", type=str, default="", choices=PLATFORMS)
    parser.add_argument("--config-file", type=str)
    parser.add_argument("--install-dir", type=str, default="")
    parser.add_argument("--config-subdir", type=str, default="configs")
    parser.add_argument(
        "--pm-config-dir",
        type=str,
        default="",
        help="Path to platform manager config dir. Expects same structure as in fboss/platform/configs",
    )
    parser.add_argument("--tests-subdir", type=str, default="tests")

    # Remainder of args are passed to pytest (e.g. --mark, --collect-only, etc)
    parser.add_argument("pytest_args", nargs=argparse.REMAINDER)

    return parser.parse_args()


def main() -> None:
    args = parse_args()

    # Some args exposed to pytest for use in fixt res
    pytest_args = [
        os.path.join(args.install_dir, args.tests_subdir)
    ] + args.pytest_args[1:]
    if args.platform:
        pytest_args += [f"--platform={args.platform}"]
    if args.config_file:
        pytest_args += [f"--config_file={args.config_file}"]
    if args.install_dir:
        pytest_args += [f"--install_dir={args.install_dir}"]
    if args.config_subdir:
        pytest_args += [f"--config_subdir={args.config_subdir}"]
    if args.pm_config_dir:
        pytest_args += [f"--pm_config_dir={args.pm_config_dir}"]

    pytest.main(pytest_args)


if __name__ == "__main__":
    main()
