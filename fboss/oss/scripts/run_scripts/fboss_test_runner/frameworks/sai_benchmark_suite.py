#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import re
from argparse import Namespace

from fboss_test_runner.constants import (
    SUB_ARG_AGENT_RUN_MODE_MONO,
    SUB_ARG_AGENT_RUN_MODE_MULTI,
)
from fboss_test_runner.frameworks.benchmark_suite import BenchmarkSuite
from fboss_test_runner.services.fboss_agent_utils import (
    cleanup_hw_agent_service,
    setup_and_start_hw_agent_service,
)

SAI_BENCH_CONFIG = "./share/hw_benchmark_tests/sai_bench.materialized_JSON"

SAI_BENCH_BINARY = "/opt/fboss/bin/sai_all_benchmarks-sai_impl"
SAI_MULTI_SWITCH_BENCH_BINARY = (
    "/opt/fboss/bin/sai_multi_switch_all_benchmarks-sai_impl"
)


class SaiBenchmarkSuite(BenchmarkSuite):
    """SAI agent benchmark suite.

    Uses the consolidated ``sai_all_benchmarks-sai_impl`` (or multi-switch)
    binary, starts a hw_agent for multi-switch runs, and keys thresholds by
    dotted per-benchmark test ids
    (``fboss.agent.hw.sai.benchmarks.<...>.<Bench>``).
    """

    def _is_multi_switch(self, args: Namespace) -> bool:
        return (
            getattr(args, "agent_run_mode", SUB_ARG_AGENT_RUN_MODE_MONO)
            == SUB_ARG_AGENT_RUN_MODE_MULTI
        )

    def binary_path(self, args: Namespace) -> str:
        if self._is_multi_switch(args):
            return SAI_MULTI_SWITCH_BENCH_BINARY
        return SAI_BENCH_BINARY

    def config_path(self) -> str:
        return SAI_BENCH_CONFIG

    def known_bad_keys(self, platform_key: str) -> list[str]:
        keys_to_try = [platform_key]
        for suffix in ("/mono", "/multi_switch"):
            if platform_key.endswith(suffix):
                keys_to_try.append(platform_key[: -len(suffix)])
                break
        return keys_to_try

    def build_cmd(
        self, binary: str, args: Namespace, benchmark_name: str | None = None
    ) -> list[str]:
        is_multi_switch = self._is_multi_switch(args)

        run_cmd = [binary, "--json"]
        if benchmark_name:
            run_cmd.extend(["--bm_regex", f"^{re.escape(benchmark_name)}$"])

        if args.config:
            run_cmd.extend(["--config", args.config, "--mgmt-if", args.mgmt_if])

        if is_multi_switch:
            run_cmd.extend(["--multi_switch", "--hw_agent_for_testing"])
            if getattr(args, "num_npus", 1) > 1:
                run_cmd.append("--multi_npu_platform_mapping")
        else:
            run_cmd.append("--flexports")
            if getattr(args, "platform_mapping_override_path", None) is not None:
                run_cmd.extend(
                    [
                        "--platform_mapping_override_path",
                        args.platform_mapping_override_path,
                    ]
                )

        if args.fruid_path is not None:
            run_cmd.append("--fruid_filepath=" + args.fruid_path)

        if not is_multi_switch:
            run_cmd.extend(["--enable_sai_log", args.sai_logging])
        run_cmd.extend(["--logging", "WARN"])
        return run_cmd

    def find_thresholds(
        self,
        benchmark_name: str,
        platform_key: str,
        all_thresholds: dict,
        args: Namespace,
    ) -> list[dict]:
        """Dotted-per-benchmark lookup: the config key's last dot-segment is the
        benchmark name; entries matching the binary's mode win, otherwise fall
        back to the other mode. Warmboot keys are skipped."""
        is_multi_switch = self._is_multi_switch(args)
        candidates = []
        for key, threshold_configs in all_thresholds.items():
            if ".warmboot" in key:
                continue
            if key.rsplit(".", 1)[-1] != benchmark_name:
                continue
            is_multi_key = ".multi_switch." in key
            candidates.append((key, threshold_configs, is_multi_key))

        candidates.sort(key=lambda c: c[2] != is_multi_switch)

        for _key, threshold_configs, _is_multi in candidates:
            for config_entry in threshold_configs:
                if re.search(config_entry["test_config_regex"], platform_key):
                    return config_entry.get("thresholds", [])
        return []

    def setup(self, args: Namespace) -> None:
        if not (self._is_multi_switch(args) and args.config):
            return
        num_npus = getattr(args, "num_npus", 1)
        additional_args = ["--multi_npu_platform_mapping"] if num_npus > 1 else None
        setup_and_start_hw_agent_service(
            switch_indexes=list(range(num_npus)),
            fboss_agent_config_path=args.config,
            platform_mapping_override_path=getattr(
                args, "platform_mapping_override_path", None
            ),
            is_warm_boot=False,
            additional_args=additional_args,
        )

    def teardown(self, args: Namespace) -> None:
        if not self._is_multi_switch(args):
            return
        cleanup_hw_agent_service(list(range(getattr(args, "num_npus", 1))))
