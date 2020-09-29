HW benchmarks HOWTOs
=====================
Code layout
------------

All code for HwBenchmarks is laid out in following directories

`https://github.com/facebook/fboss/tree/master/fboss/agent/hw/benchmarks`

Runnning HW benchmarks
-----------------------
Hw benchmarks can be run by hand (requiring just binary and config) or via netcastle

.. code-block:: sh
  ./sai_ecmp_shrink_speed-brcm-08.08.2020_odp --config galaxy_lc_left.benchmark.agent.conf 


Or via netcastle

.. code-block:: sh

  netcastle --team bcm_bench  --basset-query 'fboss:wedge40:fboss10956237.snc1'  --regex='.*bcm_stats_collection_speed.*'


When running by hand, note that a subset of benchmarks have both a warm and cold boot flavor. For such benchmarks, we
require 2 runs. First run with ``--setup-for-warmboot`` argument (to get the cold boot numbers) and a second one
without this arg to get warmboot numbers.


Timing SDK calls (SAI only)
---------------------------

When optimizing, a common question that arises is - for any given benchmark, how much time is spent in SDK calls
vs how much is spent in FBOSS code. Note that this includes both on and off CPU time. Answering this question
quickly provides a valuable hint for where to start optimizing.

FunctionCallTimeReporter provides a means for doing just that - `https://github.com/facebook/fboss/blob/master/fboss/lib/FunctionCallTimeReporter.h`. 
At a high level this class provides a means for tracking time spent in calls of a particular type, at a per thread granularity.It also tracks total 
time spent in a block of code. Now if we are able to intercept all calls to SDK, we have a means to track the time spent for all SDK calls quite easily. 
Luckily for SAI this is quite easy to do in the SaiApi class - `https://github.com/facebook/fboss/blob/master/fboss/agent/hw/sai/api/SaiApi.h#L74`. 
Code has been added to intercept all of set, get, create, remove and get_stats SAI api calls. Then if function call tracking is on, the calls to SAI sdk are timed and reported. 

Typically, we only want to track such timing over a subset of the benchmark lifetime. For e.g. for ECMP shrink benchmark, which tracks ECMP shrink times in order of msecs, we don't want to pollute its timing by tracking calls during initialization and tear down period. Individual benchmarks are further instrumented to track relevant blocks via ScopedCallTimer - `https://github.com/facebook/fboss/blob/master/fboss/lib/FunctionCallTimeReporter.h#L75`. This as the name suggests, tracks SDK call times for lifetime of ScopedCallTimer object. Now to time such a instrumented benchmark simply run the benchmark with --enable-call-timing argument. E.g.


.. code-block:: sh

  ./sai_ecmp_shrink_speed-brcm-08.08.2020_odp --config galaxy_lc_left.benchmark.agent.conf --enable_call_timing  2>&1 | grep FunctionCallTimeReporter
  I0929 10:37:05.089034 1257292 FunctionCallTimeReporter.cpp:80] Total time, msecs: 1.42458
  I0929 10:37:05.127463 1257475 FunctionCallTimeReporter.cpp:60]  Thread: fbossSaiLnkScnB (139974116505344) function (SDK) time msecs: 1.14111
  I0929 10:37:09.296592 1257292 FunctionCallTimeReporter.cpp:60]  Thread: sai_ecmp_shrink (139975089503488) function (SDK) time msecs: 0.094921
  [root@fsw003-lc401.p007.f01.prn3.tfbnw.net ~]# 



The output above shows total wall clock time in the code block and time spent in SDK calls for each individual thread during that time.

