# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.


# pyre-strict
from typing import Optional

import click

from . import conveyor_api


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
    """n
    Analyze conveyor releases and nodes
    """
    print("Conveyor parsed: ", conveyor)
    if release:
        handle_single_release_command(conveyor, release)


def handle_single_release_command(conveyor: str, release: str) -> None:
    """
    Handle command 1: Single release instance analyzis
    """
    click.echo(f"📊 Analyzing release {release} for conveyor {conveyor}...\n")

    release_status = conveyor_api.get_release_status(conveyor, release_instance=release)

    for release_data in release_status:
        for node_name, node_data in release_data.get("runs", {}).items():
            if node_data:
                print(node_name, node_data.get("status"))


def main() -> None:
    """
    Entry point in BUCK target fboss_conveyor
    """
    cli()


if __name__ == "__main__":
    cli()
