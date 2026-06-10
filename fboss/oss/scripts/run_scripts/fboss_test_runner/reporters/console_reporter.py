#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import sys

from fboss_test_runner.result_types import BenchmarkResult, GtestResult


class ConsoleReporter:
    def print_gtest_summary(self, results: list[GtestResult]) -> None:
        counts = {"PASSED": 0, "FAILED": 0, "SKIPPED": 0, "TIMEOUT": 0}
        for result in results:
            status = result.mapped_status
            print(f"[ {status} ] {result.test_name} ({result.duration_ms} ms)")
            counts[status] += 1
        print("Summary:")
        for status, value in counts.items():
            print("  ", status, ":", value)

    def print_benchmark_summary(
        self, results: list[BenchmarkResult], skipped_count: int = 0
    ) -> None:
        print("\n" + "=" * 80)
        print("BENCHMARK RESULTS SUMMARY")
        print("=" * 80)
        for result in results:
            status = result["test_status"]
            threshold = result.get("threshold_status", "")
            name = result.get("benchmark_test_name") or result["benchmark_binary_name"]
            suffix = ""
            if threshold == "EXCEEDED":
                suffix = f" [THRESHOLD EXCEEDED: {result['threshold_details']}]"
            elif threshold == "PASS":
                suffix = " [THRESHOLD PASS]"
            elif threshold == "NO_THRESHOLD":
                suffix = " [NO_THRESHOLD]"
            print(f"{name}: {status}{suffix}")
        print("=" * 80)

        ok = sum(1 for r in results if r["test_status"] == "OK")
        failed = sum(1 for r in results if r["test_status"] == "FAILED")
        timed_out = sum(1 for r in results if r["test_status"] == "TIMEOUT")
        threshold_pass = sum(1 for r in results if r.get("threshold_status") == "PASS")
        threshold_exceeded = sum(
            1 for r in results if r.get("threshold_status") == "EXCEEDED"
        )
        no_threshold = sum(
            1 for r in results if r.get("threshold_status") == "NO_THRESHOLD"
        )
        print(f"\nTotal: {len(results)} benchmarks")
        print(f"OK: {ok}")
        print(f"Failed: {failed}")
        print(f"Timed Out: {timed_out}")
        print(f"Skipped (known bad, pre-filtered): {skipped_count}")
        print(
            f"Threshold: {threshold_pass} pass, {threshold_exceeded} exceeded, {no_threshold} not configured"
        )

        if failed > 0 or timed_out > 0 or threshold_exceeded > 0:
            sys.exit(1)
