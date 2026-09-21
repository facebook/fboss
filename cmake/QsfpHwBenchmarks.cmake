# CMake to build libraries and binaries in fboss/qsfp_service/test/benchmarks

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

# Consolidated QSFP hardware benchmark binary. Individual benchmarks are
# registered via folly BENCHMARK()/BENCHMARK_MULTI() in the source files below
# and selected at runtime via --bm_regex. The benchmark sources are compiled
# directly into the executable (not a separate static library) so the folly
# benchmark self-registrations are always linked in without needing
# --whole-archive.
#
# Only built when BENCHMARK_INSTALL is set (run-getdeps.py --benchmark-install),
# mirroring cmake/AgentHwSaiBenchmarks.cmake.

if(BENCHMARK_INSTALL)
  set(QSFP_HW_TEST_BENCHMARK_SRCS
    fboss/qsfp_service/test/benchmarks/HwBenchmarkUtils.cpp
    fboss/qsfp_service/test/benchmarks/PhyInitBenchmark.cpp
    fboss/qsfp_service/test/benchmarks/ReadWriteRegisterBenchmark.cpp
    fboss/qsfp_service/test/benchmarks/RefreshTcvrBenchmark.cpp
    fboss/qsfp_service/test/benchmarks/UpdateXphyStatsBenchmark.cpp
  )

  set(QSFP_HW_TEST_BENCHMARK_DEPS
    # Provides main() for folly benchmark binaries.
    hw_benchmark_main
    # QSFP service platform + manager stack (mirrors qsfp_service /
    # qsfp_hw_test deps) used by HwBenchmarkUtils.cpp and the benchmarks.
    qsfp_platforms_wedge
    qsfp_core
    qsfp_module
    qsfp_config
    qsfp_handler
    phy_management_base
    transceiver_manager
    port_manager
    # Common helpers: CommonFileUtils.h / CommonUtils.h.
    common_file_utils
    common_utils
    # folly benchmark suite (BENCHMARK macros, BenchmarkSuspender).
    Folly::follybenchmark
  )

  if(SAI_BRCM_PAI_IMPL)
    BUILD_AND_INSTALL_WITH_XPHY_SDK_LIBS(
      "qsfp_hw_test_benchmark"
      QSFP_HW_TEST_BENCHMARK_SRCS
      QSFP_HW_TEST_BENCHMARK_DEPS
      "brcm_pai"
      XPHY_SDK_LIBS
    )
  else()
    BUILD_AND_INSTALL_WITH_XPHY_SDK_LIBS(
      "qsfp_hw_test_benchmark"
      QSFP_HW_TEST_BENCHMARK_SRCS
      QSFP_HW_TEST_BENCHMARK_DEPS
      ""
      ""
    )
  endif()
endif()
