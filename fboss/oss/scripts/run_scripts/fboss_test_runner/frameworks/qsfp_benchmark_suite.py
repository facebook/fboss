#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import re
from argparse import Namespace

from fboss_test_runner.frameworks.benchmark_suite import BenchmarkSuite

QSFP_BENCH_CONFIG = "./share/hw_benchmark_tests/qsfp_bench.materialized_JSON"
QSFP_BENCH_BINARY = "/opt/fboss/bin/qsfp_hw_test_benchmark"
QSFP_BENCH_TARGET = "//fboss/qsfp_service/test/benchmarks:qsfp_bench_test"


class QsfpBenchmarkSuite(BenchmarkSuite):
    """Non-agent QSFP benchmark suite (qsfp_hw_test_benchmark, --qsfp-config)."""

    def binary_path(self, args: Namespace) -> str:  # noqa: ARG002
        return QSFP_BENCH_BINARY

    def config_path(self) -> str:
        return QSFP_BENCH_CONFIG

    def known_bad_keys(self, platform_key: str) -> list[str]:
        return [platform_key]

    def build_cmd(
        self, binary: str, args: Namespace, benchmark_name: str | None = None
    ) -> list[str]:
        run_cmd = [binary, "--json"]
        if benchmark_name:
            run_cmd.extend(["--bm_regex", f"^{re.escape(benchmark_name)}$"])
        if getattr(args, "qsfp_config", None):
            run_cmd.extend(["--qsfp-config", args.qsfp_config])
        if getattr(args, "force_5pim_fuji", False):
            run_cmd.append("--force-5pim-fuji")
        if getattr(args, "port_manager_mode", False):
            run_cmd.append("--port-manager-mode")
        if args.fruid_path is not None:
            run_cmd.append("--fruid_filepath=" + args.fruid_path)
        run_cmd.extend(["--logging", "WARN"])
        return run_cmd

    def find_thresholds(
        self,
        benchmark_name: str,
        platform_key: str,
        all_thresholds: dict,
        args: Namespace,  # noqa: ARG002
    ) -> list[dict]:
        # QSFP thresholds live under a single buck-target key (unlike SAI's
        # dotted per-benchmark keys). Collect every entry whose test_config_regex
        # matches the platform.
        platform_thresholds: list[dict] = []
        for entry in all_thresholds.get(QSFP_BENCH_TARGET, []):
            if re.search(entry["test_config_regex"], platform_key):
                platform_thresholds.extend(entry.get("thresholds", []))

        # A benchmark is only "gated" when its own timing metric (the folly
        # benchmark name) has a threshold. If not, return [] so the framework
        # reports NO_THRESHOLD rather than a misleading PASS derived solely from
        # the shared rusage (max_rss / cpu_time_usec) bounds.
        if not any(
            re.search(t.get("metric_key_regex", ""), benchmark_name)
            for t in platform_thresholds
        ):
            return []
        return platform_thresholds
