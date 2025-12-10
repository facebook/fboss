#
# Copyright 2004-present Facebook. All Rights Reserved.
#
#
# Enums for sdkcastle configuration based on thrift spec
#

# pyre-unsafe

from enum import Enum


class BuildMode(Enum):
    DEV = "dev"
    DEV_ASAN = "dev-asan"
    DEV_TSAN = "dev-tsan"
    OPT = "opt"
    OPT_ASAN = "opt-asan"
    OPT_TSAN = "opt-tsan"
    DBG = "dbg"
    DBG_ASAN = "dbg-asan"
    DBG_TSAN = "dbg-tsan"


class BootType(Enum):
    COLDBOOT = "coldboot"
    WARMBOOT = "warmboot"


class Hardware(Enum):
    WEDGE100S = "wedge100s"
    WEDGE400 = "wedge400"
    MINIPACK = "minipack"
    YAMP = "yamp"
    WEDGE400C = "wedge400c"
    FUJI = "fuji"
    ELBERT = "elbert"
    DARWIN = "darwin"
    MONTBLANC = "montblanc"
    MERU800BIA = "meru800bia"
    MERU800BFA = "meru800bfa"
    MORGAN800CC = "morgan800cc"
    JANGA800BIC = "janga800bic"
    TAHAN800BC = "tahan800bc"
    MINIPACK3N = "minipack3n"
    DARWIN_48V = "darwin_48v"
    ICECUBE800BC = "icecube800bc"


class NpuMode(Enum):
    MONO = "mono"
    MULTI_SWITCH = "multi_switch"


class MultiStage(Enum):
    NONE = "none"
    FE2 = "fe2"
    FE13 = "fe13"
    FAP = "fap"


class LinkTestType(Enum):
    L1 = "l1"
    L2 = "l2"
    STRESS = "stress"


class RunMode(Enum):
    FULL_RUN = "full-run"
    LIST_TEST_RUNNER_CMDS_ONLY = "list-test-runner-cmds-only"


class TestRunnerMode(Enum):
    OSS = "oss"
    META_INTERNAL = "meta-internal"


class AsicType(Enum):
    TOMAHAWK = "tomahawk"
    TOMAHAWK2 = "tomahawk2"
    TOMAHAWK3 = "tomahawk3"
    TOMAHAWK4 = "tomahawk4"
    TOMAHAWK5 = "tomahawk5"
    TOMAHAWK6 = "tomahawk6"
    JERICHO3 = "jericho3"
    RAMON3 = "ramon3"
    GIBRALTAR = "gibraltar"
    GRAPHENE200 = "graphene200"
    SPECTRUM4 = "spectrum4"


def string_to_enum(value, enum_class):
    """Convert string value to enum"""
    if isinstance(value, str):
        for enum_item in enum_class:
            # print(f"enum_item.value = {enum_item.value}, passed in value = {value}")
            # if enum_item.value == value.lower().replace("-", "_"):
            if enum_item.value == value.lower():
                # print(f"enum_item.value = {enum_item.value}, passed in value = {value}")
                return enum_item
        raise ValueError(f"Invalid {enum_class.__name__} value: {value}")
    return value
