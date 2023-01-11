.. fbmeta::
    hide_page_title=true

TeFlow
###########################

Feature Description
-------------------

Overview
~~~~~~~~

First Hop Traffic Engineering(FHTE) is a controller based solution for dynamically placing traffic flows over least congested paths in the backend network. FHTE addresses some of the limitations in existing back end networks such as performance degradations with drains and link failures, eliminates job placement constraints and provides a design that can adapt to future topologies.

FHTE solution forwards the traffic based on <src port, dst ip> pair. It also supports per flow stats. A TCAM based forwarding solution cannot satisfy the scale requirements of the backend network due to hardware limitations. This feature is implemented in wedge agent using exact match based forwarding in Tomahawk3 and Tomahawk4 switches.

Broadcom Tomahawk series of switches has a flexible hardware table called unified forwarding table (UFT) which can do longest prefix match(LPM) or hash lookup. There are 8 banks in UFT which can be partitioned to create an exact match table along with ALPM. Each UFT bank can accommodate 16k exact match entries or 16k IPv6 /64 LPM entries. Wedge agent now partitions the UFT table with 4 banks to implement exact match forwarding.

The Broadcom NPU does exact match lookup in parallel to LPM lookups without any performance penalty. The forwarding result from exact match lookup takes precedence over the action set by LPM. The exact match lookup can also set a per entry counter which is shared with TCAM (38k stats) on Tomahawk3. Tomahawk4 has a flexible pool of 24k stats which is shared with other forwarding stages. The exact match lookup can be performed using a subset of bits in the destination prefix. The destination prefix length is specified as part of agent configuration and can be changed in a warm boot upgrade.

Wedge agent supports a Traffic Engineering Flow Table using exact match solution. Clients can use thrift APIs to install and manage TE flow entries. Each TE flow entry can attach a shared or unique counter to obtain flow bandwidth usage information. The flow stats are exported periodically to FSDB and clients can obtain stats information from FSDB through a pub-sub interface.


Design
~~~~~~

Detailed design can be found on the following link.

References: Wedge Agent Design for TE flows: https://fb.quip.com/MkPRA2n6fMsD

Scale
~~~~~

Wedge agent currently support 9k flow entries with unique stat counters. The scale test shows agent can add 9k entries to hardware in around 5 seconds.

Tradeoff
~~~~~~~~

N/A

Usecase
-------

N/A

Configuration
-------------

TeFlow config are part of switch settings configuration of agent config.
The exactMatchTableConfigs need to be set as below to enable TeFlow feature.

::

     "switchSettings": {
      "l2LearningMode": 0,
      "qcmEnable": false,
      "ptpTcEnable": false,
      "l2AgeTimerSeconds": 300,
      "maxRouteCounterIDs": 0,
      "blockNeighbors": [
      ],
      "macAddrsToBlock": [
      ],
      "switchType": 0,
      "exactMatchTableConfigs": [
        {
          "name": "TeFlowTable",
          "dstPrefixLength": 59
        }
      ]
     },



Build and test
--------------

HW tests: fboss/agent/hw/test/HwTeFlowTests.cpp

HW Dataplane tests: fboss/agent/hw/test/dataplane_tests/HwTeFlowTrafficTests.cpp

Debug
-----

Use fboss2 show teflow command to display all the telfows information
On BCM shell, fp show displays the hardware objects

Sample Output
-------------

::

  [netops@fboss315069343.snc1 ~]$ fboss2 show teflow
  Flow key: dst prefix 2401:db00:eeee::/59, src port 64
  Match Action:
    Counter ID: rtsw004-rtsw001-via-ctw001
    Redirect to Nexthops:
      2401:db00:eeee:ff::4 dev fboss4001
  State:
    Enabled: true
    Resolved Nexthops:
      2401:db00:eeee:ff::4 dev fboss4001

  [root@fboss308772211.snc1 ~]# fboss_bcm_shell -t 600

  FBOSS:localhost:5909>fp show
  FP:     unit 0:
  PIPELINE STAGE EXACTMATCH
  FP:           :tcam_sz=65536(0x10000), tcam_slices=2, tcam_ext_numb=0,
  PIPELINE STAGE INGRESS
  FP:           :tcam_sz=3072(0xc00), tcam_slices=9, tcam_ext_numb=0,
  PIPELINE STAGE EGRESS
  FP:           :tcam_sz=512(0x200), tcam_slices=4, tcam_ext_numb=0,
  PIPELINE STAGE LOOKUP
  FP:           :tcam_sz=512(0x200), tcam_slices=4, tcam_ext_numb=0,
  GID   50331649: gid=0x3000001, instance=0 mode=Single, stage=ExactMatch lookup=Enabled, ActionResId={-1}, pbmp={0x000000000000000000000000000000000000000000000000000000000ffff0ffff0ffff0ffff0ffff0ffff0ffff1ffff}
           qset={DstIp6, SrcPort, Stage, StageIngressExactMatch},
           aset={L3Switch, StatGroup},
           HintId=1

           group_priority= 0
           slice_primary =  {slice_number=0, Entry count=65536(0x10000), Entry free=65533(0xfffd)},
           group_status={prio_min=-2147483647, prio_max=2147483647, entries_total=65536, entries_free=65533,
                         counters_total=38912, counters_free=34815, meters_total=512, meters_free=512}
  EID 0x00000002: gid=0x3000001,
     slice=0, slice_idx=0xffffffff, part=0, prio=0, flags=0x810602, Installed, Enabled, color_indep=1

   DstIp6
      Part:0 Offset0: 105 Width0:  11
      Part:0 Offset1: 80 Width1:  16
      Part:0 Offset2: 96 Width2:   4
      Part:0 Offset3: 132 Width3:   4
      Part:0 Offset4: 136 Width4:   4
      Part:0 Offset5: 140 Width5:   4
      Part:0 Offset6: 144 Width6:   4
      Part:0 Offset7: 148 Width7:   4
      Part:0 Offset8: 152 Width8:   4
      Part:0 Offset9: 156 Width9:   4
      DATA=0x01010000 00000000
      MASK=0xffffffff ffffffe0
   SrcPort
      Part:0 Offset0: 116 Width0:   9
      DATA=0x00000003
      MASK=0x000001ff
           action={act=L3Switch, param0=100010(0x186aa), param1=0(0), param2=0(0), param3=0(0)}
           policer=
           statistics={stat id 2  slice = 9 idx=2 entries=1}{Bytes}

