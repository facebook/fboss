# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.


# pyre-strict
from typing import Optional

import click


@click.group()
def cli() -> None:
    """
    click decorator will take over and run the analyze function after parsing the arguments
    """
    pass


@click.command(name="analyze")
@click.option("--conveyor", required=True, help="Conveyor ID")
@click.option("--release", required=False, help="Release Instance (eg: R1112.3)")
def analyze(
    conveyor: str,
    release: Optional[str],
) -> None:
    """
    Analyze conveyor releases and nodes
    """
    print("Conveyor parsed: ", conveyor)
    if release:
        print("Release parsed: ", release)


def main() -> None:
    """
    Entry point in BUCK target fboss_conveyor
    """
    cli()


if __name__ == "__main__":
    cli()
