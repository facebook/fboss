# Copyright (c) 2004-present, Facebook, Inc.

""" build mode definitions for fboss/platform """

load("@fbcode_macros//build_defs:create_build_mode.bzl", "create_build_mode")

_extra_asan_options = {
    "detect_leaks": "1",
}

_mode = create_build_mode(
    asan_options = _extra_asan_options,
)

_modes = {
    "dbg": _mode,
    "dbgo": _mode,
    "dev": _mode,
    "opt": _mode,
}

def get_modes():
    """ Return modes for this hierarchy """
    return _modes
