#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import csv
import json
import os
import re
import subprocess
import sys
import threading
from argparse import ArgumentParser
from datetime import datetime

from run_test import (
    _load_from_file,
    OPT_ARG_FILTER,
    OPT_ARG_FILTER_FILE,
    OPT_ARG_PLATFORM_MAPPING_OVERRIDE_PATH,
    SAI_BENCH_CONFIG,
    SUB_ARG_AGENT_RUN_MODE,
    SUB_ARG_AGENT_RUN_MODE_MONO,
    SUB_ARG_AGENT_RUN_MODE_MULTI,
    SUB_ARG_NUM_NPUS,
)
from runners.test_runner import TestRunner
from services.fboss_agent_utils import (
    cleanup_hw_agent_service,
    setup_and_start_hw_agent_service,
)


class BenchmarkTestRunner:
    """
    Runner for benchmark tests.

    Uses a benchmark binary (sai_all_benchmarks-sai_impl) that
    contains all benchmark registrations. Individual benchmarks are selected
    at runtime via --bm_regex, with each test running as a separate process
    for full setup/run/teardown isolation.
    """

    def _get_benchmark_binary(self, args):
        if (
            getattr(args, "agent_run_mode", SUB_ARG_AGENT_RUN_MODE_MONO)
            == SUB_ARG_AGENT_RUN_MODE_MULTI
        ):
            benchmark_binary = "/opt/fboss/bin/sai_multi_switch_all_benchmarks-sai_impl"
        else:
            benchmark_binary = "/opt/fboss/bin/sai_all_benchmarks-sai_impl"
        if os.path.exists(benchmark_binary) and os.path.isfile(benchmark_binary):
            return benchmark_binary
        return None

    def _list_benchmarks(self, binary_path):
        """Discover available benchmarks via --bm_list.

        Returns:
            List of benchmark name strings, or None on failure.
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
        except (
            subprocess.CalledProcessError,
            subprocess.TimeoutExpired,
            OSError,
        ) as e:
            print(f"Warning: Failed to list benchmarks from {binary_path}: {e}")
            return None

    def add_subcommand_arguments(self, sub_parser: ArgumentParser):
        """Add benchmark-specific command line arguments"""
        sub_parser.add_argument(
            OPT_ARG_FILTER,
            type=str,
            help="Regex to filter benchmarks by name (e.g. --filter HwEcmp.*)",
            default=None,
        )
        sub_parser.add_argument(
            OPT_ARG_FILTER_FILE,
            type=str,
            help="File containing list of benchmark test names to run (one per line).",
            default=None,
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
            choices=[
                SUB_ARG_AGENT_RUN_MODE_MONO,
                SUB_ARG_AGENT_RUN_MODE_MULTI,
            ],
            default=SUB_ARG_AGENT_RUN_MODE_MONO,
            help="Specify agent run mode. Default is mono mode.",
        )
        sub_parser.add_argument(
            SUB_ARG_NUM_NPUS,
            choices=[1, 2],
            default=1,
            type=int,
            help="Number of NPUs for multi-switch mode. Default is 1.",
        )

    def _build_keys_to_try(self, platform_key):
        keys_to_try = [platform_key]
        for suffix in ("/mono", "/multi_switch"):
            if platform_key.endswith(suffix):
                keys_to_try.append(platform_key[: -len(suffix)])
                break
        return keys_to_try

    def _load_known_bad_test_regexes(self, platform_key):
        """Load known bad test regexes from sai_bench config JSON.
        Args:
            platform_key: Platform key string (e.g., 'brcm/10.2.0.0_odp/tomahawk4')

        Returns:
            List of regex pattern strings for known bad tests
        """
        return TestRunner._get_test_regexes_from_file(
            self,
            file_path=SAI_BENCH_CONFIG,
            test_dict_key="known_bad_tests",
            keys_to_try=self._build_keys_to_try(platform_key),
        )

    def _load_unsupported_test_regexes(self, platform_key):
        """Load unsupported test regexes from sai_bench config JSON.
        Args:
            platform_key: Platform key string (e.g., 'brcm/10.2.0.0_odp/tomahawk4')

        Returns:
            List of regex pattern strings for unsupported tests
        """
        return TestRunner._get_test_regexes_from_file(
            self,
            file_path=SAI_BENCH_CONFIG,
            test_dict_key="unsupported_tests",
            keys_to_try=self._build_keys_to_try(platform_key),
        )

    def _load_benchmark_thresholds(self):
        """Load benchmark thresholds from sai_bench config JSON.

        Returns:
            Dict mapping test IDs to lists of threshold configs,
            or empty dict if file not found
        """
        if not os.path.exists(SAI_BENCH_CONFIG):
            return {}

        with open(SAI_BENCH_CONFIG) as f:
            config = json.load(f)

        return config.get("buck_rule_or_test_id_to_benchmark_thres", {})

    def _find_thresholds_for_benchmark(
        self, benchmark_name, is_multi_switch, platform_key, all_thresholds
    ):
        """Find matching thresholds for a benchmark.

        Lookup:
          1. Collect threshold entries whose key's last dot-segment equals the
             benchmark name (warmboot keys are skipped).
          2. Sort so the entry matching the binary's mode comes first; if only
             the other-mode entry exists we still fall back to it rather than
             returning nothing.
          3. Pick the first entry whose test_config_regex matches platform_key
             and return its `thresholds` list.

        Args:
            benchmark_name: BENCHMARK() registered name (e.g. HwEcmpGroupShrink)
            is_multi_switch: True if the binary is the multi-switch variant
            platform_key: Platform key string for test_config_regex matching
            all_thresholds: Dict from _load_benchmark_thresholds()

        Returns:
            List of MetricThreshold dicts (each with metric_key_regex,
            optional lower_bound/upper_bound), or empty list when no match.
        """
        candidates = []
        for key, threshold_configs in all_thresholds.items():
            if ".warmboot" in key:
                continue
            if key.rsplit(".", 1)[-1] != benchmark_name:
                continue
            is_multi_key = ".multi_switch." in key
            candidates.append((key, threshold_configs, is_multi_key))

        # Matching-mode entries first; fall back to the other mode otherwise.
        candidates.sort(key=lambda c: c[2] != is_multi_switch)

        for _key, threshold_configs, _is_multi in candidates:
            for config_entry in threshold_configs:
                if re.search(config_entry["test_config_regex"], platform_key):
                    return config_entry.get("thresholds", [])

        return []

    def _check_thresholds(self, result, thresholds):
        """Check benchmark metrics against thresholds.

        Args:
            result: Parsed benchmark result dict
            thresholds: List of threshold dicts with metric_key_regex,
                       optional lower_bound, optional upper_bound

        Returns:
            List of violation description strings (empty = all pass)
        """
        # With --json, all metrics are already in their native units:
        # - benchmark name key: picoseconds (from folly --json)
        # - cpu_rx_pps, cpu_tx_pps: packets per second
        # - max_rss, cpu_time_usec: from rusage
        # These match the threshold units in the config directly.
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

    @staticmethod
    def _find_jsons_in_str(output):
        """Extract all JSON dicts from a string that may contain non-JSON text.

        Scans for '{' characters and tries json.JSONDecoder().raw_decode()
        at each position.

        Returns:
            List of parsed dict objects
        """
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

    def _parse_benchmark_output(self, binary_name, stdout, benchmark_name=None):
        """Parse benchmark output to extract metrics.

        With --json flag, the binary outputs multiple JSON objects:
        - folly benchmark: {"BenchmarkName": <picoseconds>}
        - benchmark-specific: {"cpu_rx_pps": <value>} (optional)
        - rusage: {"cpu_time_usec": <value>, "max_rss": <value>}

        All JSON key-value pairs are collected into a flat metrics dict
        for threshold comparison.

        Args:
            binary_name: Path to the benchmark binary
            stdout: Captured stdout text
            benchmark_name: If provided, used to look up benchmark timing
                directly from metrics instead of guessing by exclusion.
        """
        result = {
            "benchmark_binary_name": binary_name,
            "benchmark_test_name": benchmark_name or "",
            "benchmark_time_ps": "",
            "test_status": "FAILED",
            "cpu_time_usec": "",
            "max_rss": "",
            "cpu_rx_pps": "",
            "cpu_tx_pps": "",
            "metrics": {},
        }

        json_dicts = self._find_jsons_in_str(stdout)

        all_metrics = {}
        for d in json_dicts:
            all_metrics.update(d)

        result["metrics"] = all_metrics

        if benchmark_name and benchmark_name in all_metrics:
            result["benchmark_time_ps"] = str(all_metrics[benchmark_name])

        # Populate CSV fields from known metric keys
        known_metric_keys = {
            "cpu_time_usec",
            "max_rss",
            "cpu_rx_pps",
            "cpu_tx_pps",
        }
        for key in known_metric_keys:
            if key in all_metrics:
                result[key] = str(all_metrics[key])

        return result

    @staticmethod
    def _read_stream(stream, lines_list, prefix=""):
        """Read lines from a stream, print them in real-time, and collect them."""
        for line in stream:
            print(f"{prefix}{line}", end="", flush=True)
            lines_list.append(line)

    def _build_benchmark_cmd(self, binary_name, args, benchmark_name=None):
        """Build the command line for running a benchmark binary."""
        is_multi_switch = (
            getattr(args, "agent_run_mode", SUB_ARG_AGENT_RUN_MODE_MONO)
            == SUB_ARG_AGENT_RUN_MODE_MULTI
        )

        run_cmd = [binary_name, "--json"]
        if benchmark_name:
            run_cmd.extend(["--bm_regex", f"^{re.escape(benchmark_name)}$"])

        if args.config:
            run_cmd.extend(["--config", args.config, "--mgmt-if", args.mgmt_if])

        if is_multi_switch:
            run_cmd.extend(["--multi_switch", "--hw_agent_for_testing"])
            num_npus = getattr(args, "num_npus", 1)
            if num_npus > 1:
                run_cmd.append("--multi_npu_platform_mapping")
        else:
            run_cmd.append("--flexports")
            if args.platform_mapping_override_path is not None:
                run_cmd.extend(
                    [
                        "--platform_mapping_override_path",
                        args.platform_mapping_override_path,
                    ]
                )

        if args.fruid_path is not None:
            run_cmd.extend(["--fruid_filepath=" + args.fruid_path])

        if not is_multi_switch:
            run_cmd.extend(["--enable_sai_log", args.sai_logging])
        run_cmd.extend(["--logging", "WARN"])
        return run_cmd

    def _run_benchmark_binary(self, binary_name, args, benchmark_name=None):
        """Run a single benchmark and return parsed results.

        When benchmark_name is provided, uses --bm_regex to select exactly
        one benchmark from the binary. Each invocation is a
        separate process with full agent init/run/teardown.
        """
        display_name = benchmark_name or os.path.basename(binary_name)
        print(f"########## Running benchmark: {display_name}", flush=True)

        run_cmd = self._build_benchmark_cmd(binary_name, args, benchmark_name)

        print(f"Running command: {' '.join(run_cmd)}", flush=True)

        try:
            process = subprocess.Popen(
                run_cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )

            stdout_lines = []
            stderr_lines = []

            stdout_thread = threading.Thread(
                target=self._read_stream,
                args=(process.stdout, stdout_lines),
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
                return {
                    "benchmark_binary_name": binary_name,
                    "benchmark_test_name": benchmark_name or "",
                    "benchmark_time_ps": "",
                    "test_status": "TIMEOUT",
                    "cpu_time_usec": "",
                    "max_rss": "",
                    "cpu_rx_pps": "",
                    "cpu_tx_pps": "",
                    "metrics": {},
                }

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
            if process.returncode == 0:
                result["test_status"] = "OK"
            else:
                result["test_status"] = "FAILED"
            return result

        except Exception as e:
            print(f"########## Error running benchmark {display_name}: {e!s}")
            return {
                "benchmark_binary_name": binary_name,
                "benchmark_test_name": benchmark_name or "",
                "benchmark_time_ps": "",
                "test_status": "FAILED",
                "cpu_time_usec": "",
                "max_rss": "",
                "cpu_rx_pps": "",
                "cpu_tx_pps": "",
                "metrics": {},
            }

    def _load_requested_benchmarks(self, filter_file):
        """Load benchmark test names from a filter file.

        Args:
            filter_file: Path to file with benchmark names (one per line).

        Returns:
            List of benchmark name strings, or None if not found/empty.
        """
        if not os.path.exists(filter_file):
            print(f"Error: Configuration file not found: {filter_file}")
            return None

        names = _load_from_file(filter_file)

        if not names:
            print(f"Error: No benchmarks found in {filter_file}")
            return None

        return names

    def _apply_threshold_check(
        self, result, is_multi_switch, platform_key, all_thresholds
    ):
        """Apply threshold validation to a benchmark result.

        Looks up thresholds by benchmark name, preferring entries that match
        the caller-provided mode. When no threshold is found, prints a
        "WILL ALWAYS PASS" warning so silent regressions are visible.
        """
        test_name = result.get("benchmark_test_name", "")
        if not (result["test_status"] == "OK" and all_thresholds and platform_key):
            result["threshold_status"] = "N/A"
            result["threshold_details"] = ""
            return

        thresholds = self._find_thresholds_for_benchmark(
            test_name,
            is_multi_switch,
            platform_key,
            all_thresholds,
        )
        if not thresholds:
            print(
                f"  >> WARNING: no benchmark threshold defined for "
                f"{test_name} on {platform_key}; this benchmark WILL "
                f"ALWAYS PASS until thresholds are added."
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

    def _write_results_and_summary(self, results, skipped_count=0):
        """Write CSV results file and print summary."""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        csv_filename = f"benchmark_results_{timestamp}.csv"

        with open(csv_filename, "w", newline="") as csvfile:
            fieldnames = [
                "benchmark_binary_name",
                "benchmark_test_name",
                "benchmark_time_ps",
                "test_status",
                "cpu_time_usec",
                "max_rss",
                "cpu_rx_pps",
                "cpu_tx_pps",
                "threshold_status",
                "threshold_details",
            ]
            writer = csv.DictWriter(
                csvfile, fieldnames=fieldnames, extrasaction="ignore"
            )
            writer.writeheader()
            for result in results:
                writer.writerow(result)

        # Print summary

        print(f"\n########## Benchmark results written to: {csv_filename}")

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

    def _get_benchmarks_to_run(self, all_benchmarks, args):
        """Apply --filter and --filter_file to narrow the benchmark list.

        Returns:
            Filtered list of benchmark names, or None on error.
        """
        benchmarks = list(all_benchmarks)
        available_set = set(all_benchmarks)

        if args.filter:
            try:
                benchmarks = [
                    name for name in benchmarks if re.search(args.filter, name)
                ]
            except re.error as e:
                print(f"Error: Invalid --filter regex '{args.filter}': {e}")
                return None
            if not benchmarks:
                print(f"No benchmarks matching --filter '{args.filter}'")
                return None
            print(f"--filter '{args.filter}' matched {len(benchmarks)} benchmarks")

        if args.filter_file:
            requested = self._load_requested_benchmarks(args.filter_file)
            if requested is None:
                return None
            not_found = [name for name in requested if name not in available_set]
            if not_found:
                print(
                    f"\nWarning: {len(not_found)} benchmark names not found in binary:"
                )
                for name in not_found:
                    print(f"  - {name}")
            requested_set = set(requested)
            benchmarks = [name for name in benchmarks if name in requested_set]

        return benchmarks

    def _filter_known_bad(self, benchmarks, known_bad_regexes):
        """Remove known-bad tests from the list before running.

        Returns:
            Tuple of (filtered list, skipped count).
        """
        if not known_bad_regexes:
            return benchmarks, 0

        filtered = []
        for name in benchmarks:
            if TestRunner._test_matches_any_regex(self, name, known_bad_regexes):
                print(f"  >> SKIPPING (known bad): {name}")
            else:
                filtered.append(name)
        skipped_count = len(benchmarks) - len(filtered)
        if skipped_count:
            print(f"Pre-filtered {skipped_count} known bad benchmarks")
        return filtered, skipped_count

    def _filter_unsupported(self, benchmarks, unsupported_regexes):
        """Remove unsupported tests from the list before running.

        Returns:
            Tuple of (filtered list, skipped count).
        """
        if not unsupported_regexes:
            return benchmarks, 0

        filtered = []
        for name in benchmarks:
            if TestRunner._test_matches_any_regex(self, name, unsupported_regexes):
                print(f"  >> SKIPPING (unsupported): {name}")
            else:
                filtered.append(name)
        skipped_count = len(benchmarks) - len(filtered)
        if skipped_count:
            print(f"Pre-filtered {skipped_count} unsupported benchmarks")
        return filtered, skipped_count

    def _start_hw_agent_service(self, args):
        """Start hw_agent service for multi-switch benchmarks."""
        num_npus = getattr(args, "num_npus", 1)
        switch_indexes = list(range(num_npus))
        additional_args = []
        if num_npus > 1:
            additional_args.append("--multi_npu_platform_mapping")
        setup_and_start_hw_agent_service(
            switch_indexes=switch_indexes,
            fboss_agent_config_path=args.config,
            platform_mapping_override_path=args.platform_mapping_override_path,
            is_warm_boot=False,
            additional_args=additional_args or None,
        )

    def _stop_hw_agent_service(self, args):
        """Stop hw_agent service after multi-switch benchmarks."""
        switch_indexes = list(range(getattr(args, "num_npus", 1)))
        cleanup_hw_agent_service(switch_indexes)

    def run_test(self, args):
        """Run benchmark tests."""
        is_multi_switch = (
            getattr(args, "agent_run_mode", SUB_ARG_AGENT_RUN_MODE_MONO)
            == SUB_ARG_AGENT_RUN_MODE_MULTI
        )

        known_bad_regexes = []
        unsupported_regexes = []
        all_thresholds = {}
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

        binary_path = self._get_benchmark_binary(args)
        if not binary_path:
            print("Error: Could not find benchmark binary")
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
        if not benchmarks_to_run:
            print("No benchmarks to run after filtering unsupported")
            return

        benchmarks_to_run, skipped_count = self._filter_known_bad(
            benchmarks_to_run, known_bad_regexes
        )
        if not benchmarks_to_run:
            print("No benchmarks to run after filtering")
            return

        if args.list_tests:
            for name in benchmarks_to_run:
                print(name)
            return

        print(f"\nRunning {len(benchmarks_to_run)} benchmarks")

        try:
            if is_multi_switch and args.config:
                self._start_hw_agent_service(args)
            results = []
            for name in benchmarks_to_run:
                result = self._run_benchmark_binary(
                    binary_path, args, benchmark_name=name
                )
                self._apply_threshold_check(
                    result, is_multi_switch, platform_key, all_thresholds
                )
                results.append(result)

            self._write_results_and_summary(
                results, skipped_count + unsupported_skipped
            )
        finally:
            if is_multi_switch:
                self._stop_hw_agent_service(args)
