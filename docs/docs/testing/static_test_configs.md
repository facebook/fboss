---
sidebar_label: Test Configurations
---
# OSS Test Configuration Files

The `materialized_JSON` files are static configuration files for FBOSS services. In production environments, these configurations are dynamically assembled from multiple sources. For OSS testing, pre-generated static versions are provided.

## Test Binary to Config Directory Mapping

| Test Binary | Config Directory |
|-------------|------------------|
| Link Test | `fboss/oss/link_test_configs/` |
| Agent Benchmarks | `fboss/oss/hw_bench_configs/` |
| Agent HW Test | `fboss/oss/hw_test_configs/` |
| QSFP HW Test | `fboss/oss/qsfp_test_configs/` |

See also:
- [Agent HW Test](/docs/testing/agent_hw_test)
- [Link Test](/docs/testing/link_test)
- [QSFP Test Config](/docs/developing/qsfp_test_config)
