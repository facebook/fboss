# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.


# pyre-strict
import click


@click.group()
def cli() -> None:
    """
    click decorator will take over and run the analyze function after parsing the arguments
    """
    pass


@cli.command(name="analyze")
@click.option("--conveyor", required=True, help="Conveyor ID")
def analyze(conveyor: str) -> None:
    """
    Analyze conveyor releases and nodes
    """
    print("Conveyor parsed: ", conveyor)


def main() -> None:
    """
    Entry point in BUCK target fboss_conveyor
    """
    cli()


if __name__ == "__main__":
    cli()
