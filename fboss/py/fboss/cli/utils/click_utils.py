from typing import Optional

import click
from click.core import Command, Context


class AliasedGroup(click.Group):
    """
    For command abbreviation
        http://click.pocoo.org/5/advanced/#command-aliases
    """

    def get_command(self, ctx: Context, cmd_name: str) -> Command | None:
        rv = click.Group.get_command(self, ctx, cmd_name)
        if rv is not None:
            return rv

        matches = [x for x in self.list_commands(ctx) if x.startswith(cmd_name)]
        if not matches:
            return None
        elif len(matches) == 1:
            return click.Group.get_command(self, ctx, matches[0])
        ctx.fail("Too many matches: %s" % ", ".join(sorted(matches)))
