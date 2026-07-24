#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import abc
from argparse import Namespace


class BenchmarkSuite(abc.ABC):
    """One family of benchmarks (e.g. SAI or QSFP) consumed by
    ``BenchmarkFramework``.

    A suite corresponds to a single benchmark binary that bundles many
    individual benchmarks. The framework owns all generic orchestration
    (discovery, per-benchmark execution, JSON parsing, threshold checking,
    filtering, reporting); each concrete suite encapsulates only what differs
    between families: which binary to run, which config file backs
    known-bad/unsupported/threshold lookups, how to build the command line,
    whether services must be started, and how thresholds are keyed in the
    config.
    """

    @abc.abstractmethod
    def binary_path(self, args: Namespace) -> str:
        """Absolute path to the benchmark binary to run."""

    @abc.abstractmethod
    def config_path(self) -> str:
        """Path to the materialized_JSON with known-bad / unsupported /
        threshold config for this suite."""

    @abc.abstractmethod
    def build_cmd(
        self, binary: str, args: Namespace, benchmark_name: str | None = None
    ) -> list[str]:
        """Build the command line to run one benchmark (or list, when
        ``benchmark_name`` is None)."""

    @abc.abstractmethod
    def known_bad_keys(self, platform_key: str) -> list[str]:
        """Candidate config keys to look up known-bad / unsupported regexes for
        the given ``platform_key`` (encapsulates run-mode suffix handling)."""

    @abc.abstractmethod
    def find_thresholds(
        self,
        benchmark_name: str,
        platform_key: str,
        all_thresholds: dict,
        args: Namespace,
    ) -> list[dict]:
        """Return the list of ``MetricThreshold`` dicts that apply to
        ``benchmark_name`` on ``platform_key``. Encapsulates the config
        key-format difference between suites."""

    def setup(self, args: Namespace) -> None:  # noqa: B027 - optional hook
        """Start any services the benchmark binary needs. Default: no-op."""

    def teardown(self, args: Namespace) -> None:  # noqa: B027 - optional hook
        """Stop any services started in ``setup``. Default: no-op."""
