# pyre-unsafe

import inspect
from typing import Callable, Optional


class Remediation:
    def __init__(
        self,
        name: str,
        func: Callable,
        description: Optional[str] = None,
        timeout: Optional[int] = None,
    ) -> None:
        self.name = name
        self.func = func
        self.description = description
        self.result = None

    def __call__(self, *args, **kwargs):
        self.result = self.func(*args, **kwargs)
        return self.result

    def __repr__(self) -> str:
        return f"<{self.__class__.__name__} {self.name}>"

    def run(self) -> None:
        self.func()


def remediation(func=None, description=None, name=None):
    """Register func as a remediation
    This is the remediation version of fixmybmc.bmccheck.bmcCheck();
    """

    def remediation_decorator(func):
        nonlocal description, name
        docstring = inspect.getdoc(func)
        if not description:
            if docstring:
                description = docstring.splitlines()[0]
            else:
                description = func.__name__
        if not name:
            name = func.__name__

        remediation = Remediation(
            name=name,
            func=func,
            description=description,
        )
        return remediation

    if func is None:
        return remediation_decorator
    else:
        return remediation_decorator(func)
