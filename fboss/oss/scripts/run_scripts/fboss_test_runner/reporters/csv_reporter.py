#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import csv
from datetime import datetime

from fboss_test_runner.result_types import BenchmarkResult, GtestResult


class CsvReporter:
    def write_gtest_results(self, results: list[GtestResult]) -> None:
        if not results:
            print("No tests we were run.")
            return
        output_csv = (
            f"hwtest_results_{datetime.now().strftime('%Y_%b_%d-%I_%M_%S_%p')}.csv"
        )

        with open(output_csv, "w") as f:
            writer = csv.writer(f)
            writer.writerow(["Test Name", "Result"])
            for result in results:
                writer.writerow([result.test_name, result.mapped_status])

        print(f"\nTest output stored at: {output_csv}")

    def write_benchmark_results(self, results: list[BenchmarkResult]) -> None:
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

        print(f"\n########## Benchmark results written to: {csv_filename}")
