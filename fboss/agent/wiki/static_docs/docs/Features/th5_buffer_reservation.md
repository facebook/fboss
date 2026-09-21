# TH5 Buffer reservation

This document describes how BRCM SDK reserves buffers during initialization and then calculate the shared limit to configure for each role.

## SDK defaults
BRCM TH5 ASIC supports a max of 341080 cells. There are 16 LB ports, 4 mgmt ports, 1 CPU port.

When SKIP_BUFFER_RESERVATION is set in YML config, SDKLT initialization reserves a total of 12237 cells,

| Value | Description                                 | Port type
|-------|---------------------------------------------|----------
| 448   | TM_THD_Q_GRP                                | loopback (16x28)
| 112   | TM_THD_Q_GRP                                | mgmt (4x28)
| 77    | TM_ING_THD_PORT_PRI_GRP                     | cpu/mgmt/lb PG7 (8xlb, 2xmgmt, 1xcpu per ITM with 7 cells each)
| 336   | TM_THD_MC_Q                                 | cpu (7X48)
| 11264 | RQE                                         |
| 608   | RQE inflight                                |
| 12845 | Total                                       |

So total available cells for NOS is 341080 - 12845 = **328235** cells.

## Clear allocation from NOS
NOS can clear the SDK default allocation for MGMT and CPU ports. The requirement though is to have the specific PORT_ID in the PC_PORT table configuration FBOSS generates.

FBOSS config typically only configures MGMT_PORT 76 in PC_PORT. In addition, default YML config has no min allocation for MC queues for CPU port. This recovers 28 cells for 1 MGMT port and 336 cells for CPU MC queues.

This increases total available cells to 328235 + 28 + 336 = **328599** cells.

Lastly, based on MoD usage, 3306 cells can potentially become available for shared usage.

If MoD is enabled, available cells for shared usage is 328599 - 3306 = **325293** cells.

## Final Calculation
For an example RTSW config with 64x400G uplinks and 64x400G downlinks and 1 mgmt port. Total reserved cells from NOS would total 7385 cells.

| Usage   | Port count   | Cells | Total cells
|---------|--------------|-------|------------
| PGmin 2 |  64          | 57    | 3648
| PGmin 6 |  64          | 19    | 1216
| Qmin 7  |  129         | 19    | 2451 (128 data + 1 mgmt)
| CPU MC  |  10          | 7     | 70 (10 CPU queues)
| Total   |              |       | 7385

Total headroom allocation: 36720

This further brings the total available cells to 325293 - 7385 - 36720 = **281188** cells.
