#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

from argparse import ArgumentParser, Namespace

from fboss_test_runner.constants import (
    OPT_ARG_FORCE_5PIM_FUJI,
    OPT_ARG_PLATFORM_MAPPING_OVERRIDE_PATH,
    OPT_ARG_PORT_MANAGER_MODE,
    OPT_ARG_QSFP_BENCH,
    OPT_ARG_QSFP_CONFIG_FILE,
    OPT_ARG_SAI_BENCH,
    SUB_ARG_AGENT_RUN_MODE,
    SUB_ARG_AGENT_RUN_MODE_MONO,
    SUB_ARG_AGENT_RUN_MODE_MULTI,
    SUB_ARG_NUM_NPUS,
)
from fboss_test_runner.frameworks.benchmark_framework import BenchmarkFramework
from fboss_test_runner.frameworks.benchmark_suite import BenchmarkSuite
from fboss_test_runner.frameworks.qsfp_benchmark_suite import QsfpBenchmarkSuite
from fboss_test_runner.frameworks.sai_benchmark_suite import SaiBenchmarkSuite
from fboss_test_runner.runners.test_runner import TestRunner


class BenchmarkTestRunner(TestRunner):
    """Thin adapter for the ``benchmark`` subcommand.

    Defaults to SAI benchmarks; pass ``--qsfp`` to run QSFP benchmarks instead
    (``--sai`` / ``--qsfp`` are mutually exclusive). Delegates all execution to
    the shared ``BenchmarkFramework``. Benchmarks are standalone binaries (not
    gtest based), so this does not use the base gtest run path; the abstract
    hooks below only satisfy the ``TestRunner`` ABC.
    """

    def _get_test_binary_name(self) -> str:
        return self._select_suite(self.args).binary_path(self.args)

    def _get_warmboot_check_file(self) -> str:
        return ""

    def _get_test_run_args(self, conf_file: str) -> list[str]:  # noqa: ARG002
        return []

    @staticmethod
    def _select_suite(args: Namespace) -> BenchmarkSuite:
        # --sai / --qsfp are mutually exclusive; SAI is the default when neither
        # is passed (backward compatible with the original benchmark command).
        if getattr(args, "qsfp", False):
            return QsfpBenchmarkSuite()
        return SaiBenchmarkSuite()

    def add_subcommand_arguments(self, sub_parser: ArgumentParser) -> None:
        super().add_subcommand_arguments(sub_parser)
        self._add_sai_arguments(sub_parser)
        # SAI is the default; --sai / --qsfp are mutually exclusive but not
        # required, preserving backward compatibility of the benchmark command.
        family_group = sub_parser.add_mutually_exclusive_group()
        family_group.add_argument(
            OPT_ARG_SAI_BENCH,
            action="store_true",
            default=False,
            help="Run SAI agent benchmarks (default).",
        )
        family_group.add_argument(
            OPT_ARG_QSFP_BENCH,
            action="store_true",
            default=False,
            help="Run QSFP benchmarks.",
        )
        sub_parser.add_argument(
            OPT_ARG_PLATFORM_MAPPING_OVERRIDE_PATH,
            nargs="?",
            type=str,
            help="A file path to a platform mapping JSON file to be used.",
            default=None,
        )
        sub_parser.add_argument(
            SUB_ARG_AGENT_RUN_MODE,
            choices=[SUB_ARG_AGENT_RUN_MODE_MONO, SUB_ARG_AGENT_RUN_MODE_MULTI],
            default=SUB_ARG_AGENT_RUN_MODE_MONO,
            help="Specify agent run mode (SAI benchmarks only). Default is mono.",
        )
        sub_parser.add_argument(
            SUB_ARG_NUM_NPUS,
            choices=[1, 2],
            default=1,
            type=int,
            help="Number of NPUs for multi-switch mode. Default is 1.",
        )
        # --- QSFP benchmark options ---
        sub_parser.add_argument(
            OPT_ARG_QSFP_CONFIG_FILE,
            type=str,
            default=None,
            help="qsfp config (absolute path); required when --qsfp is set.",
        )
        sub_parser.add_argument(
            OPT_ARG_FORCE_5PIM_FUJI,
            action="store_true",
            default=False,
            help="Force 5PIM Fuji mode (QSFP benchmarks on fuji_5x16Q).",
        )
        sub_parser.add_argument(
            OPT_ARG_PORT_MANAGER_MODE,
            action="store_true",
            default=False,
            help="Enable port manager mode (QSFP benchmarks).",
        )

    def run_test(self, args: Namespace) -> None:
        self.args = args
        if getattr(args, "qsfp", False) and not getattr(args, "qsfp_config", None):
            raise ValueError("--qsfp requires --qsfp-config to be set")
        BenchmarkFramework(self._select_suite(args)).run(args)
