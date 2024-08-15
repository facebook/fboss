.. fbmeta::
   hide_page_title=true

PKTIO Benchmark Comparison on TH3
#################################

This wiki contains the performance comparison of Rx/Tx slow path rate on TH3.
The experiment helps us understand the amount of performance gain when PKTIO is enabled, and how that is different in native SDK and Sai.

Steps
------
Native SDK tx slow path and rx slow path:
.. code-block:: sh

  netcastle --team bcm_bench --basset-query fboss:wedge400/asic=tomahawk3 --regex runTxSlowPathBenchmark
  netcastle --team bcm_bench --basset-query fboss:wedge400/asic=tomahawk3 --regex RxSlowPathBenchmark

Sai tx slow path and rx slow path:
.. code-block:: sh

  netcastle --team sai_bench --basset-query fboss:wedge400/asic=tomahawk3 --regex runTxSlowPathBenchmark
  netcastle --team sai_bench --basset-query fboss:wedge400/asic=tomahawk3 --regex RxSlowPathBenchmark

For native SDK, use command line argument `--use_pktio=false` to disable pktio. For sai, modify the config file to set 'pktio_driver_type: 0'.

Results
--------
Native SDK

+-----------+-----------+------------+------+
|           | W/O PKTIO | With PKTIO | Gain |
+-----------+-----------+------------+------+
|  Tx       |   32K     |   810K     | 25x  |
+-----------+-----------+------------+------+
|  Rx       |   242K    |   891K     | 3.7x |
+-----------+-----------+------------+------+

Sai

+-----------+-----------+------------+------+
|           | W/O PKTIO | With PKTIO | Gain |
+-----------+-----------+------------+------+
|  Tx       |   31K     |   825K     | 26x  |
+-----------+-----------+------------+------+
|  Rx       |   282K    |   1.3M     | 4.6x |
+-----------+-----------+------------+------+
