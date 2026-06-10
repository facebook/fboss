#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import re
import typing as t
from dataclasses import dataclass

_GTEST_RESULT_PATTERN = re.compile(
    r"""\[\s+(?P<status>(OK)|(FAILED)|(SKIPPED)|(TIMEOUT))\s+\]\s+
    (?P<test_name>[\w\.\/]+)\s+\((?P<duration_ms>\d+)\s+ms\)$""",
    re.VERBOSE,
)

_GTEST_STATUS_MAP: dict[str, str] = {
    "OK": "PASSED",
    "FAILED": "FAILED",
    "SKIPPED": "SKIPPED",
    "TIMEOUT": "TIMEOUT",
}


@dataclass
class GtestResult:
    test_name: str
    status: str
    duration_ms: int

    @property
    def mapped_status(self) -> str:
        return _GTEST_STATUS_MAP.get(self.status, self.status)

    @staticmethod
    def parse_output(test_output: bytes) -> list["GtestResult"]:
        results = []
        for line in test_output.decode("utf-8").split("\n"):
            match = _GTEST_RESULT_PATTERN.match(line.strip())
            if match:
                results.append(
                    GtestResult(
                        test_name=match.group("test_name"),
                        status=match.group("status"),
                        duration_ms=int(match.group("duration_ms")),
                    )
                )
        return results


class BenchmarkResult(t.TypedDict, total=False):
    benchmark_binary_name: str
    benchmark_test_name: str
    benchmark_time_ps: str
    test_status: str
    cpu_time_usec: str
    max_rss: str
    cpu_rx_pps: str
    cpu_tx_pps: str
    metrics: dict[str, t.Any]
    threshold_status: str
    threshold_details: str
