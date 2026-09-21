#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import enum
import re
import typing as t
from dataclasses import dataclass

_GTEST_RESULT_PATTERN = re.compile(
    r"""\[\s+(?P<status>(OK)|(FAILED)|(SKIPPED)|(TIMEOUT))\s+\]\s+
    (?P<test_name>[\w\.\/]+)\s+\((?P<duration_ms>\d+)\s+ms\)$""",
    re.VERBOSE,
)


class GtestStatus(enum.Enum):
    """A single gtest result status.

    Values are the tokens gtest emits in its result lines, plus TIMEOUT, which
    we synthesize when a test exceeds its run timeout.
    """

    OK = "OK"
    FAILED = "FAILED"
    SKIPPED = "SKIPPED"
    TIMEOUT = "TIMEOUT"

    @property
    def display_name(self) -> str:
        """Name shown in the summary and CSV; gtest 'OK' is reported as PASSED."""
        return "PASSED" if self is GtestStatus.OK else self.value


@dataclass
class GtestResult:
    test_name: str
    status: GtestStatus
    duration_ms: int
    # Exact name emitted by --list_tests and accepted by --filter.
    filter_name: str | None = None

    @property
    def mapped_status(self) -> str:
        return self.status.display_name

    def as_log_line(self) -> str:
        """Render as a single gtest-style line for the per-test console echo."""
        return f"[ {self.status.value} ] {self.test_name} ({self.duration_ms} ms)"

    @staticmethod
    def parse_output(test_output: bytes) -> list["GtestResult"]:
        results = []
        for line in test_output.decode("utf-8").split("\n"):
            match = _GTEST_RESULT_PATTERN.match(line.strip())
            if match:
                results.append(
                    GtestResult(
                        test_name=match.group("test_name"),
                        status=GtestStatus(match.group("status")),
                        duration_ms=int(match.group("duration_ms")),
                    )
                )
        return results


@dataclass
class RunOutcome:
    """Outcome of a single ``_run_test`` invocation.

    Carries the text to echo to the console (the raw test output, or a
    synthesized status line when the binary produced none) alongside the
    structured results that feed the run summary. Building ``GtestResult``s
    directly here avoids synthesizing gtest text only to re-parse it.
    """

    console_output: str
    results: list[GtestResult]


@dataclass
class TestExecutionResult:
    """In-memory result of one runner execution.

    ``results`` is ``None`` when setup failed before test execution. ``error``
    retains the underlying setup exception so callers without structured
    reporting can preserve the existing behavior of raising it.
    """

    exit_code: int
    results: list[GtestResult] | None = None
    setup_failure: str | None = None
    error: Exception | None = None


def compute_exit_code(results: list[GtestResult]) -> int:
    return int(
        any(
            result.status in {GtestStatus.FAILED, GtestStatus.TIMEOUT}
            for result in results
        )
    )


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
