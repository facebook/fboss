#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import json

from fboss_test_runner.result_types import GtestResult


class JsonReporter:
    def write_run_results(
        self,
        results: list[GtestResult],
        setup_failures: list[str],
        path: str,
    ) -> None:
        records = [
            {
                "test_name": result.filter_name or result.test_name,
                "status": result.mapped_status,
                "duration_ms": result.duration_ms,
            }
            for result in results
        ]
        records.extend(
            {"run_status": "SETUP_FAILED", "message": message}
            for message in setup_failures
        )
        with open(path, "w") as output:
            json.dump(records, output, indent=2)

    def write_gtest_results(self, results: list[GtestResult], path: str) -> None:
        self.write_run_results(results, [], path)

    def write_setup_failure(self, path: str, message: str) -> None:
        self.write_run_results([], [message], path)
