# pyre-strict
"""
Python implementation of platform name utilities for FBOSS.
This module is referenced from and provides equivalent functionality to:
https://www.internalfb.com/code/fbsource/fbcode/fboss/platform/helpers/PlatformNameLib.cpp
"""

from typing import Optional

from fboss.util.fixmyfboss.utils import run_cmd


def sanitize_platform_name(platform_name_from_bios: str) -> str:
    """
    Sanitize platform name from BIOS.
    Some platforms do not have the standardized platform-names in dmidecode yet.
    For such platforms, we use a translation function to get the standardized
    platform-names.
    """
    platform_name_upper = platform_name_from_bios.upper()

    if platform_name_upper in ["MINIPACK3", "MINIPACK3_MCB"]:
        return "MONTBLANC"

    if platform_name_upper == "JANGA":
        return "JANGA800BIC"

    if platform_name_upper == "TAHAN":
        return "TAHAN800BC"

    # MERU800BIAB and MERU800BIA have equivalent configs
    if platform_name_upper == "MERU800BIAB":
        return "MERU800BIA"

    return platform_name_upper


def get_string_file_content(path: str) -> Optional[str]:
    """
    Read string content from file. Returns None if file doesn't exist or can't be read.
    """
    try:
        with open(path, "r") as f:
            return f.read().strip()
    except Exception:
        return None


def get_platform_name_from_bios() -> str:
    """
    Returns platform name from dmidecode. Platform names are UPPERCASE but
    otherwise should match config names. Known dmidecode values are mapped to
    config names.
    """

    dmidecode_command = "dmidecode -s system-product-name"

    standard_out = run_cmd(dmidecode_command).stdout
    if not standard_out:
        error_msg = "Failed to get platform name from bios"
        raise RuntimeError(error_msg)

    standard_out = standard_out.strip()
    result = sanitize_platform_name(standard_out)

    return result


def get_platform_name() -> Optional[str]:
    """
    Returns platform name matching get_platform_name_from_bios, but returns None
    instead of throwing exceptions.
    """
    cache_path = "/var/facebook/fboss/platform_name"
    name_from_cache = get_string_file_content(cache_path)

    if name_from_cache is not None:
        return name_from_cache

    try:
        return get_platform_name_from_bios()
    except Exception:
        return None
