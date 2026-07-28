#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import json
import os
import re
import subprocess
import threading
from argparse import Namespace
from collections.abc import Iterable

from fboss_test_runner.frameworks.benchmark_suite import BenchmarkSuite
from fboss_test_runner.reporters.console_reporter import ConsoleReporter
from fboss_test_runner.reporters.csv_reporter import CsvReporter
from fboss_test_runner.result_types import BenchmarkResult
from fboss_test_runner.runners.utils import (
    get_test_regexes_from_file,
    load_from_file,
    test_matches_any_regex,
)


class BenchmarkFramework:
    """Generic, suite-driven benchmark test framework.

    Owns the full benchmark lifecycle -- discover benchmarks from the binary via
    ``--bm_list``, apply ``--filter`` / ``--filter_file``, filter known-bad and
    unsupported tests, run each benchmark as its own process with full
    setup/run/teardown isolation via ``--bm_regex``, parse JSON metrics, check
    thresholds, and report CSV + console summaries.

    Everything benchmark-family-specific (binary, config, command, services,
    threshold key format) is delegated to a ``BenchmarkSuite``, so SAI, QSFP, and
    future benchmark types share this one framework.
    """

    def __init__(self, suite: BenchmarkSuite) -> None:
        self.suite = suite

    # ---- config loading (generic; suite supplies path + key variants) --------

    def _load_known_bad_test_regexes(self, platform_key: str) -> list[str]:
        return get_test_regexes_from_file(
            file_path=self.suite.config_path(),
            test_dict_key="known_bad_tests",
            keys_to_try=self.suite.known_bad_keys(platform_key),
        )

    def _load_unsupported_test_regexes(self, platform_key: str) -> list[str]:
        return get_test_regexes_from_file(
            file_path=self.suite.config_path(),
            test_dict_key="unsupported_tests",
            keys_to_try=self.suite.known_bad_keys(platform_key),
        )

    def _load_benchmark_thresholds(self) -> dict:
        """Load the ``buck_rule_or_test_id_to_benchmark_thres`` map from the
        suite's config JSON (empty dict when the file is absent)."""
        config_path = self.suite.config_path()
        if not os.path.exists(config_path):
            return {}
        with open(config_path) as f:
            config = json.load(f)
        return config.get("buck_rule_or_test_id_to_benchmark_thres", {})

    # ---- benchmark discovery -------------------------------------------------

    def _list_benchmarks(self, binary_path: str) -> list[str] | None:
        """Discover available benchmarks via ``--bm_list``.

        Returns a list of benchmark name strings, or None on failure.
        """
        try:
            output = subprocess.check_output(
                [binary_path, "--bm_list"],
                stderr=subprocess.DEVNULL,
                text=True,
                timeout=30,
            )
            benchmarks = []
            for line in output.splitlines():
                name = line.strip()
                if name and re.match(r"^[A-Za-z]\w*$", name):
                    benchmarks.append(name)
            return benchmarks
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired, OSError) as e:
            print(f"Warning: Failed to list benchmarks from {binary_path}: {e}")
            return None

    @staticmethod
    def _wildcards_to_regex(pattern: str) -> str:
        """Expand ``*``/``?`` to ``.*``/``.``, leaving wildcards that are already
        part of a regex alone so ``.*Route.*`` does not become ``..*Route..*``."""
        pattern = re.sub(r"(?<![.\\*])\?", ".", pattern)
        return re.sub(r"(?<![.\\])\*", ".*", pattern)

    @staticmethod
    def _split_filter(filter_string: str) -> tuple[list[str], list[str]]:
        """Split a ``--gtest_filter``-style string into (includes, excludes):
        colon-separated patterns, everything after the first ``-`` excluded.
        Benchmark names are ``\\w`` only, so ``-`` never appears in a pattern."""
        include_part, _, exclude_part = filter_string.partition("-")
        return (
            [p.strip() for p in include_part.split(":") if p.strip()],
            [p.strip() for p in exclude_part.split(":") if p.strip()],
        )

    @staticmethod
    def _compile_patterns(patterns: list[str], kind: str) -> list[re.Pattern] | None:
        """Compile filter patterns, or return None if any one is invalid."""
        compiled = []
        for pattern in patterns:
            try:
                compiled.append(
                    re.compile(BenchmarkFramework._wildcards_to_regex(pattern))
                )
            except re.error as e:
                print(f"Error: Invalid {kind} pattern '{pattern}': {e}")
                return None
        return compiled

    def _get_benchmarks_to_run(
        self, all_benchmarks: list[str], args: Namespace
    ) -> list[str] | None:
        """Narrow the benchmark list by ``--filter`` / ``--filter_file``, both of
        which take the gtest grammar (``include:include:-exclude``) so a filter
        file written for the gtest runners selects the same way. ``--filter_file``
        wins when both are given. Returns None on error."""
        if args.filter_file:
            if not os.path.exists(args.filter_file):
                print(f"Error: Configuration file not found: {args.filter_file}")
                return None
            patterns = load_from_file(args.filter_file, args.profile)
            if not patterns:
                print(f"Error: No benchmark filters found in {args.filter_file}")
                return None
            filter_string = ":".join(patterns)
        else:
            filter_string = args.filter or ""

        if not filter_string:
            return list(all_benchmarks)

        includes, excludes = self._split_filter(filter_string)
        include_res = self._compile_patterns(includes, "include")
        exclude_res = self._compile_patterns(excludes, "exclude")
        if include_res is None or exclude_res is None:
            return None

        benchmarks = [
            name
            for name in all_benchmarks
            if (not include_res or any(r.search(name) for r in include_res))
            and not any(r.search(name) for r in exclude_res)
        ]
        if not benchmarks:
            print(f"No benchmarks matching filter '{filter_string}'")
            return None
        print(f"Filter '{filter_string}' matched {len(benchmarks)} benchmarks")
        return benchmarks

    def _filter_known_bad(
        self, benchmarks: list[str], known_bad_regexes: list[str]
    ) -> tuple[list[str], int]:
        """Remove known-bad tests before running. Returns (kept, skipped)."""
        if not known_bad_regexes:
            return benchmarks, 0
        filtered = []
        for name in benchmarks:
            if test_matches_any_regex(name, known_bad_regexes):
                print(f"  >> SKIPPING (known bad): {name}")
            else:
                filtered.append(name)
        skipped_count = len(benchmarks) - len(filtered)
        if skipped_count:
            print(f"Pre-filtered {skipped_count} known bad benchmarks")
        return filtered, skipped_count

    def _filter_unsupported(
        self, benchmarks: list[str], unsupported_regexes: list[str]
    ) -> tuple[list[str], int]:
        """Remove unsupported tests before running. Returns (kept, skipped)."""
        if not unsupported_regexes:
            return benchmarks, 0
        filtered = []
        for name in benchmarks:
            if test_matches_any_regex(name, unsupported_regexes):
                print(f"  >> SKIPPING (unsupported): {name}")
            else:
                filtered.append(name)
        skipped_count = len(benchmarks) - len(filtered)
        if skipped_count:
            print(f"Pre-filtered {skipped_count} unsupported benchmarks")
        return filtered, skipped_count

    # ---- output parsing ------------------------------------------------------

    @staticmethod
    def _find_jsons_in_str(output: str) -> list[dict]:
        """Extract all JSON dicts from a string that may contain non-JSON text."""
        decoder = json.JSONDecoder()
        results = []
        idx = 0
        while idx < len(output):
            idx = output.find("{", idx)
            if idx == -1:
                break
            try:
                obj, end_idx = decoder.raw_decode(output, idx)
                if isinstance(obj, dict):
                    results.append(obj)
                idx = end_idx
            except (json.JSONDecodeError, ValueError):
                idx += 1
        return results

    @staticmethod
    def _empty_benchmark_result(
        binary_name: str, benchmark_name: str | None, status: str
    ) -> BenchmarkResult:
        return {
            "benchmark_binary_name": binary_name,
            "benchmark_test_name": benchmark_name or "",
            "benchmark_time_ps": "",
            "test_status": status,
            "cpu_time_usec": "",
            "max_rss": "",
            "cpu_rx_pps": "",
            "cpu_tx_pps": "",
            "metrics": {},
        }

    def _parse_benchmark_output(
        self, binary_name: str, stdout: str, benchmark_name: str | None = None
    ) -> BenchmarkResult:
        """Parse ``--json`` benchmark output into a flat metrics dict.

        The binary emits one or more JSON objects: the folly timing
        ({"BenchmarkName": <picoseconds>}), optional benchmark-specific metrics
        ({"cpu_rx_pps": ...}), and rusage ({"cpu_time_usec": ..., "max_rss":
        ...}). All key/value pairs are merged for threshold comparison.
        """
        result = self._empty_benchmark_result(binary_name, benchmark_name, "FAILED")
        json_dicts = self._find_jsons_in_str(stdout)

        all_metrics = {}
        for d in json_dicts:
            all_metrics.update(d)
        result["metrics"] = all_metrics

        if benchmark_name and benchmark_name in all_metrics:
            result["benchmark_time_ps"] = str(all_metrics[benchmark_name])

        for key in ("cpu_time_usec", "max_rss", "cpu_rx_pps", "cpu_tx_pps"):
            if key in all_metrics:
                result[key] = str(all_metrics[key])

        return result

    # ---- execution -----------------------------------------------------------

    @staticmethod
    def _read_stream(
        stream: Iterable[str], lines_list: list[str], prefix: str = ""
    ) -> None:
        """Read lines from a stream, echo in real-time, and collect them."""
        for line in stream:
            print(f"{prefix}{line}", end="", flush=True)
            lines_list.append(line)

    def _run_benchmark_binary(
        self, binary_name: str, args: Namespace, benchmark_name: str | None = None
    ) -> BenchmarkResult:
        """Run a single benchmark (selected via ``--bm_regex``) as its own
        process and return the parsed result."""
        display_name = benchmark_name or os.path.basename(binary_name)
        print(f"########## Running benchmark: {display_name}", flush=True)

        run_cmd = self.suite.build_cmd(binary_name, args, benchmark_name)
        print(f"Running command: {' '.join(run_cmd)}", flush=True)

        try:
            process = subprocess.Popen(
                run_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
            )

            stdout_lines: list[str] = []
            stderr_lines: list[str] = []
            stdout_thread = threading.Thread(
                target=self._read_stream, args=(process.stdout, stdout_lines)
            )
            stderr_thread = threading.Thread(
                target=self._read_stream,
                args=(process.stderr, stderr_lines, "[stderr] "),
            )
            stdout_thread.start()
            stderr_thread.start()

            try:
                process.wait(timeout=args.test_run_timeout)
            except subprocess.TimeoutExpired:
                process.kill()
                stdout_thread.join()
                stderr_thread.join()
                print(
                    f"\n########## Benchmark {display_name} timed out after "
                    f"{args.test_run_timeout} seconds"
                )
                return self._empty_benchmark_result(
                    binary_name, benchmark_name, "TIMEOUT"
                )

            stdout_thread.join()
            stderr_thread.join()
            captured_stdout = "".join(stdout_lines)

            if process.returncode != 0:
                print(
                    f"\n########## Benchmark {display_name} failed with "
                    f"return code {process.returncode}"
                )
            else:
                print(f"\n########## Benchmark {display_name} completed")

            result = self._parse_benchmark_output(
                binary_name, captured_stdout, benchmark_name
            )
            result["test_status"] = "OK" if process.returncode == 0 else "FAILED"
            return result
        except Exception as e:
            print(f"########## Error running benchmark {display_name}: {e!s}")
            return self._empty_benchmark_result(binary_name, benchmark_name, "FAILED")

    # ---- thresholds ----------------------------------------------------------

    def _check_thresholds(
        self, result: BenchmarkResult, thresholds: list[dict]
    ) -> list[str]:
        """Check benchmark metrics against thresholds. Returns violation
        strings (empty = all pass). Only metrics actually present are checked,
        so a threshold for a metric absent in this run is a no-op."""
        metrics = result.get("metrics", {})
        violations = []
        for threshold in thresholds:
            metric_regex = threshold.get("metric_key_regex", "")
            lower = threshold.get("lower_bound")
            upper = threshold.get("upper_bound")
            for metric_key, metric_value in metrics.items():
                if not re.search(metric_regex, metric_key):
                    continue
                if lower is not None and metric_value < lower:
                    violations.append(
                        f"{metric_key}={metric_value} < lower_bound={lower}"
                    )
                if upper is not None and metric_value > upper:
                    violations.append(
                        f"{metric_key}={metric_value} > upper_bound={upper}"
                    )
        return violations

    def _apply_threshold_check(
        self,
        result: BenchmarkResult,
        platform_key: str | None,
        all_thresholds: dict,
        args: Namespace,
    ) -> None:
        """Validate one result against its thresholds (looked up via the suite).

        When no threshold is defined, mark NO_THRESHOLD and warn so silent
        regressions are visible.
        """
        test_name = result.get("benchmark_test_name", "")
        if not (result["test_status"] == "OK" and all_thresholds and platform_key):
            result["threshold_status"] = "N/A"
            result["threshold_details"] = ""
            return

        thresholds = self.suite.find_thresholds(
            test_name, platform_key, all_thresholds, args
        )
        if not thresholds:
            print(
                f"  >> WARNING: no benchmark threshold defined for {test_name} on "
                f"{platform_key}; this benchmark WILL ALWAYS PASS until thresholds "
                f"are added."
            )
            result["threshold_status"] = "NO_THRESHOLD"
            result["threshold_details"] = ""
            return

        violations = self._check_thresholds(result, thresholds)
        if violations:
            result["threshold_status"] = "EXCEEDED"
            result["threshold_details"] = "; ".join(violations)
            print(
                f"  >> THRESHOLD EXCEEDED: {test_name}: {result['threshold_details']}"
            )
        else:
            result["threshold_status"] = "PASS"
            result["threshold_details"] = ""

    # ---- reporting -----------------------------------------------------------

    def _write_results_and_summary(
        self, results: list[BenchmarkResult], skipped_count: int = 0
    ) -> None:
        CsvReporter().write_benchmark_results(results)
        ConsoleReporter().print_benchmark_summary(results, skipped_count)

    # ---- top-level orchestration --------------------------------------------

    def run(self, args: Namespace) -> None:
        """Discover, filter, run, threshold-check, and report benchmarks."""
        known_bad_regexes: list[str] = []
        unsupported_regexes: list[str] = []
        all_thresholds: dict = {}
        platform_key = getattr(args, "skip_known_bad_tests", None)
        if platform_key:
            known_bad_regexes = self._load_known_bad_test_regexes(platform_key)
            unsupported_regexes = self._load_unsupported_test_regexes(platform_key)
            all_thresholds = self._load_benchmark_thresholds()
            print(
                f"Loaded {len(known_bad_regexes)} known bad test patterns, "
                f"{len(unsupported_regexes)} unsupported test patterns, "
                f"and {len(all_thresholds)} threshold configs for '{platform_key}'"
            )

        binary_path = self.suite.binary_path(args)
        if not os.path.isfile(binary_path):
            print(f"Error: Could not find benchmark binary: {binary_path}")
            return

        all_benchmarks = self._list_benchmarks(binary_path)
        if all_benchmarks is None:
            print("Error: Could not discover benchmarks from binary.")
            return
        print(f"Discovered {len(all_benchmarks)} benchmarks in binary")

        benchmarks_to_run = self._get_benchmarks_to_run(all_benchmarks, args)
        if not benchmarks_to_run:
            return

        benchmarks_to_run, unsupported_skipped = self._filter_unsupported(
            benchmarks_to_run, unsupported_regexes
        )
        benchmarks_to_run, skipped_count = self._filter_known_bad(
            benchmarks_to_run, known_bad_regexes
        )

        if args.list_tests:
            for name in benchmarks_to_run:
                print(name)
            return

        if not benchmarks_to_run:
            print("No benchmarks to run after filtering known bad/unsupported")
            self._write_results_and_summary([], skipped_count + unsupported_skipped)
            return

        print(f"\nRunning {len(benchmarks_to_run)} benchmarks")

        try:
            self.suite.setup(args)
            results = []
            for name in benchmarks_to_run:
                result = self._run_benchmark_binary(
                    binary_path, args, benchmark_name=name
                )
                self._apply_threshold_check(result, platform_key, all_thresholds, args)
                results.append(result)
            self._write_results_and_summary(
                results, skipped_count + unsupported_skipped
            )
        finally:
            self.suite.teardown(args)
