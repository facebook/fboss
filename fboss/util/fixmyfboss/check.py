# pyre-strict

import traceback
from functools import wraps
from typing import Any, Callable, Dict, Optional

from fboss.util.fixmyfboss import status

_checks: Dict[str, "Check"] = {}


class Check:
    def __init__(
        self,
        name: str,
        func: Callable[..., status.Status],
    ) -> None:
        self.name = name
        self.func = func
        self.result: Optional[status.Status] = None

    def run(self) -> None:
        try:
            result = self.func()
        except Exception as e:
            traceback.print_exc()
            result = status.Error(exception=e)

        if not isinstance(result, status.Status):
            te = TypeError(f"check function returned non-Status: {result!r}")
            result = status.Error(exception=te)

        self.result = result

    def __repr__(self) -> str:
        return f"<Check {self.name}>"


def check(
    func: Callable[..., Optional[status.Status]],
) -> "Check":
    def check_decorator(
        func: Callable[..., Optional[status.Status]],
    ) -> "Check":
        name = func.__name__

        @wraps(func)
        def returnStatusOkWrapper(*args: Any, **kwargs: Any) -> status.Status:
            result = func(*args, **kwargs)
            if result is None:
                return status.Ok()
            return result

        check = Check(
            name,
            func=returnStatusOkWrapper,
        )
        _checks[name] = check
        return check

    return check_decorator(func)


def load_checks() -> None:
    import fboss.util.fixmyfboss.modules  # noqa: F401
