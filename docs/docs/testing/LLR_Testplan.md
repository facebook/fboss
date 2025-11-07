---
title: LLR_Testplan

---

## Background
Link Layer Retry (LLR) is a hop-by-hop retransmission mechanism for Ethernet links that detects loss or corruption of LLR-eligible frames and repairs them without involving higher layers. Each transmitted frame carries a sequence number; the receiver signals progress with ACK control ordered sets (CtlOS) and requests recovery with NACKs, while the sender buffers a bounded replay window and retransmits based on these signals or a timeout. Operating in the PCS/64b/66b path, LLR preserves ordering, confines recovery to a single link, and complements FEC (bit-error correction) and PFC (congestion management). Its purpose is to drive effective frame-loss rate toward zero as seen by transports, bound tail latency, and sustain throughput for AI/collective and storage workloads under realistic link errors.

## Overview

This document outlines the test plan for the Link Layer Retry (LLR) feature, detailing various test cases to ensure its robust and reliable operation. The plan covers initialization, basic functionality, error detection and recovery mechanisms, performance under different load conditions, edge case handling, and interoperability with other network features like Priority Flow Control (PFC). The objective is to thoroughly validate LLR's ability to maintain data integrity and network efficiency by retransmitting lost or corrupted frames.

## Setup


```

    Traffic Gen <----> DUT <--link-under-test-> Analyzer, Jammer, Exerciser, Traffic Gen

```
**DUT** : Device Under Test (FBOSS Switch)
**Analyzer**: Passively captures and decodes 100% of the traffic on the wire, including low-level Control Ordered Sets (CtlOS), providing a ground-truth record of the interaction
**Jammer**: Injects precise, repeatable errors (e.g., corrupting FCS, dropping packets, modifying CtlOS payloads) into the live data stream to test the DUT's error recovery logic.
**Exerciser**: Link Level block and bit stream generator
**Traffic Generator**: Traffic generator tool like Ixia which can generate line rate traffic

*Note* :
1) The Analyzer, Jammer, and Exerciser are tester components that may be supplied by commercial vendors (for example, Keysight) or obtained from open-source projects
2) These components may be supplied in a single box


## Packet Generation
- Traffic originating on the DUT will be generated using Scapy
- Traffic switched through the DUT will be generated using the Traffic generator connected.
- Traffic coming in from peer will be originated by Exerciser


## Prerequisites
| ID | Description |
|----|-------------|
| PR‑01 | DUT with LLR capability
| PR‑02 | SAI API access for profile creation
| PR‑03 | Transitions in SAI_PORT_ATTR_LLR_TX_STATUS, SAI_PORT_ATTR_LLR_RX_STATUS can be verified only if FSM state transitions are exported as events |
| PR‑04 | Availability of tester components: Analyzer, Jammer, and Exerciser

## Test Cases

### Initialization
#### LLR Profile Creation and Verification
- **Test Case ID:** 1
- **Test Case Name:** LLR Profile Creation and Verification
- **Objective:** Verify LLR profile creation with mandatory parameters and parameter validation
- **Prerequisites:** PR‑01, PR‑02

**SAI Attributes to Test:**
- SAI_PORT_LLR_PROFILE_ATTR_OUTSTANDING_FRAMES_MAX - Mandatory parameter for maximum outstanding frames
- SAI_PORT_LLR_PROFILE_ATTR_OUTSTANDING_BYTES_MAX - Mandatory parameter for maximum outstanding bytes
- SAI_PORT_LLR_PROFILE_ATTR_REPLAY_TIMER_MAX - Optional parameter for replay timer (default: 0)
- SAI_PORT_LLR_PROFILE_ATTR_REPLAY_COUNT_MAX - Optional parameter for replay count (default: 1)
- SAI_PORT_LLR_PROFILE_ATTR_PCS_LOST_TIMEOUT - Optional parameter for PCS lost timeout (default: 0)
- SAI_PORT_LLR_PROFILE_ATTR_DATA_AGE_TIMER_MAX - Optional parameter for data age timer (default: 0)
- SAI_PORT_LLR_PROFILE_ATTR_LLR_INIT_FRAME_ACTION - Optional parameter for init frame action (default: BEST_EFFORT)
- SAI_PORT_LLR_PROFILE_ATTR_LLR_FLUSH_FRAME_ACTION - Optional parameter for flush frame action (default: BEST_EFFORT)
- SAI_PORT_LLR_PROFILE_ATTR_RE_INIT_ON_FLUSH - Optional parameter for re-init on flush (default: false)
- SAI_PORT_LLR_PROFILE_ATTR_CTLOS_TARGET_SPACING - Optional parameter for CtlOS target spacing (default: 2048)

**SAI Statistics to Monitor:**
- Profile creation success/failure status
- Parameter validation results

**Test Steps:**
1. Create LLR profile with mandatory parameters only:
   - Set SAI_PORT_LLR_PROFILE_ATTR_OUTSTANDING_FRAMES_MAX to 1000
   - Set SAI_PORT_LLR_PROFILE_ATTR_OUTSTANDING_BYTES_MAX to 50000(50KB)
2. Verify profile creation succeeds
3. Read back the mandatory parameters to verify they are set correctly
4. Create LLR profile with all parameters:
   - Set all mandatory parameters (OUTSTANDING_FRAMES_MAX, OUTSTANDING_BYTES_MAX)
   - Set all optional parameters with specific values
5. Verify profile creation succeeds with all parameters
6. Read back all parameters to verify they match the configured values
7. Test parameter validation with invalid values:
   - Test with OUTSTANDING_FRAMES_MAX = 0, OUTSTANDING_FRAMES_MAX > max allowed by ASIC (should fail)
   - Test with OUTSTANDING_BYTES_MAX = 0, OUTSTANDING_FRAMES_MAX > max allowed by ASIC(should fail)
8. Verify error handling for invalid parameters
9. Test profile deletion and cleanup

**Expected Results:** LLR profile creation succeeds with mandatory parameters; All parameters are correctly set and can be read back; Parameter validation works correctly; Error handling works for invalid parameters; Profile deletion succeeds
**Pass Criteria:** Profile creation succeeds with mandatory parameters; All parameters match configured values when read back; Invalid parameter validation fails appropriately; Profile deletion completes successfully; No memory leaks during profile operations


Sample code:

```c
int create_llr_profile(sai_object_id_t *profile_id)
{
    sai_status_t status;
    sai_attribute_t attrs[3];
    int attr_count = 0;

    printf("Creating LLR Profile...\n");

    // Mandatory parameters
    attrs[attr_count].id = SAI_PORT_LLR_PROFILE_ATTR_OUTSTANDING_FRAMES_MAX;
    attrs[attr_count].value.u32 = 1000;  // Maximum 1000 outstanding frames
    attr_count++;

    attrs[attr_count].id = SAI_PORT_LLR_PROFILE_ATTR_OUTSTANDING_BYTES_MAX;
    attrs[attr_count].value.u32 = 50000;  // Maximum 50KB outstanding bytes
    attr_count++;

    // Optional parameters with default values
    attrs[attr_count].id = SAI_PORT_LLR_PROFILE_ATTR_REPLAY_TIMER_MAX;
    attrs[attr_count].value.u32 = 10000;
    attr_count++;

   // Create the LLR profile
    status = sai_port_api->create_port_llr_profile(profile_id, switch_id, attr_count, attrs);
    if (status != SAI_STATUS_SUCCESS) {
        printf("ERROR: Failed to create LLR profile, status: %d\n", status);
        return -1;
    }

    printf("✓ LLR Profile created successfully (ID: 0x%lx)\n", *profile_id);
    return 0;
}
```


#### LLR Initialization Sequence
- **Test Case ID:** 2
- **Test Case Name:** LLR Initialization Sequence
- **Objective:** Verify proper LLR initialization sequence
- **Prerequisites:** PR‑01, PR‑03

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_MODE_LOCAL - Enable LLR local mode
- SAI_PORT_ATTR_LLR_MODE_REMOTE - Enable LLR remote mode
- SAI_PORT_ATTR_LLR_PROFILE - Attach LLR profile to port
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status (should be INIT)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status (should be SEND_ACKS)

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_INIT_CTL_OS - Count of INIT control ordered sets transmitted
- SAI_PORT_STAT_LLR_RX_INIT_CTL_OS - Count of INIT control ordered sets received
- SAI_PORT_STAT_LLR_TX_INIT_ECHO_CTL_OS - Count of INIT_ECHO control ordered sets transmitted
- SAI_PORT_STAT_LLR_RX_INIT_ECHO_CTL_OS - Count of INIT_ECHO control ordered sets received

**Test Steps:**
1. Enable LLR feature on DUT using SAI_PORT_ATTR_LLR_MODE_LOCAL/REMOTE
2. Attach LLR profile using SAI_PORT_ATTR_LLR_PROFILE
3. Monitor LLR initialization sequence and handshake
4. Verify LLR parameters are properly configured
5. Monitor buffer allocation during initialization, Verify buffer parameters
6. Check for any initialization errors or warnings
7. Verify SAI_PORT_ATTR_LLR_TX_STATUS = SAI_PORT_LLR_TX_STATUS_INIT
8. Verify SAI_PORT_ATTR_LLR_RX_STATUS = SAI_PORT_LLR_RX_STATUS_SEND_ACKS

**Expected Results:**
- LLR buffers are properly allocated
- Buffer parameters match configuration
- Buffer initialization completes successfully
- No memory allocation errors
- SAI status attributes show correct initialization state

**Pass Criteria:**
- All LLR buffers allocated successfully
- Buffer parameters correct
- No memory allocation errors
- Buffer status shows ready
- SAI status attributes indicate proper initialization

**Sample code:**

```c
sai_status_t status;
   sai_attribute_t attrs[3];
   int attr_count = 0;


   // Create the LLR profile
    status = sai_port_api->create_port_llr_profile(profile_id, switch_id, attr_count, attrs);
    if (status != SAI_STATUS_SUCCESS) {
        printf("ERROR: Failed to create LLR profile, status: %d\n", status);
        return -1;
    }
    // Attach LLR profile to port
    attrs[attr_count].id = SAI_PORT_ATTR_LLR_PROFILE;
    attrs[attr_count].value.oid = profile_id;
    attr_count++;

    // Enable LLR local mode
    attrs[attr_count].id = SAI_PORT_ATTR_LLR_MODE_LOCAL;
    attrs[attr_count].value.booldata = true;
    attr_count++;

    // Enable LLR remote mode
    attrs[attr_count].id = SAI_PORT_ATTR_LLR_MODE_REMOTE;
    attrs[attr_count].value.booldata = true;
    attr_count++;

    // Set port attributes
    status = sai_port_api->set_port_attribute(port_id, attrs, attr_count);
    if (status != SAI_STATUS_SUCCESS) {
        printf("ERROR: Failed to enable LLR on port, status: %d\n", status);
        return -1;
    }
```

#### LLR Initialization with Multiple Ports
- **Test Case ID:** 3
- **Test Case Name:** LLR Initialization with Multiple Ports
- **Objective:** Test LLR initialization on multiple ports
- **Prerequisites:** PR‑01, PR‑03

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_MODE_LOCAL - Enable LLR local mode on each port
- SAI_PORT_ATTR_LLR_MODE_REMOTE - Enable LLR remote mode on each port
- SAI_PORT_ATTR_LLR_PROFILE - Attach LLR profile to each port
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status for each port
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status for each port

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_INIT_CTL_OS - Per-port count of INIT control ordered sets transmitted
- SAI_PORT_STAT_LLR_RX_INIT_CTL_OS - Per-port count of INIT control ordered sets received
- SAI_PORT_STAT_LLR_TX_INIT_ECHO_CTL_OS - Per-port count of INIT_ECHO control ordered sets transmitted
- SAI_PORT_STAT_LLR_RX_INIT_ECHO_CTL_OS - Per-port count of INIT_ECHO control ordered sets received

**Test Steps:**
1. Enable LLR on multiple ports simultaneously using SAI_PORT_ATTR_LLR_MODE_LOCAL/REMOTE
2. Attach LLR profiles to each port using SAI_PORT_ATTR_LLR_PROFILE
3. Monitor initialization sequence for each port
4. Verify independent LLR operation per port
5. Check for resource contention between ports
6. Verify SAI_PORT_ATTR_LLR_TX_STATUS and SAI_PORT_ATTR_LLR_RX_STATUS for each port

**Expected Results:**
- LLR initializes independently on each port
- No resource contention between ports
- Port-specific configurations work correctly
- All ports reach READY state
- SAI status attributes show proper initialization for each port

**Pass Criteria:**
- All ports initialize LLR successfully
- No resource contention
- Port-specific configurations work
- All ports reach READY state
- SAI status attributes indicate proper initialization for each port


### Basic Functionality
#### Frame Sequence Numbering
- **Test Case ID:** 4
- **Test Case Name:** Frame Sequence Numbering
- **Objective:** Verify that each frame is assigned a unique sequence number for tracking
- **Prerequisites:** PR‑01, PR‑03, PR‑04 LLR INIT sequence done.

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status (should be ADVANCE during normal operation)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status (should be SEND_ACKS during normal operation)

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_OK - Count of LLR-eligible frames transmitted with good FCS
- SAI_PORT_STAT_LLR_RX_OK - Count of LLR-eligible frames received with good FCS
- SAI_PORT_STAT_LLR_RX_EXPECTED_SEQ_GOOD - Count of frames received with expected sequence number and good FCS
- SAI_PORT_STAT_LLR_RX_DUPLICATE_SEQ - Count of frames received with duplicate sequence numbers

**Test Steps:**
1. Configure DUT to send 1000 frames
2. Enable frame capture on protocol analyzer
3. Transmit frames and capture traffic
4. Analyze captured frames for sequence number presence and correctness
5. Monitor SAI_PORT_ATTR_LLR_TX_STATUS and SAI_PORT_ATTR_LLR_RX_STATUS
6. Verify SAI_PORT_STAT_LLR_TX_OK and SAI_PORT_STAT_LLR_RX_OK counters, should increment by 1000.
7. Check SAI_PORT_STAT_LLR_RX_DUPLICATE_SEQ should be 0
8. SAI_PORT_STAT_LLR_RX_EXPECTED_SEQ_GOOD should increment by 1000

**Expected Results:**
- All frames contain valid sequence numbers
- Sequence numbers are unique and sequential
- No duplicate sequence numbers observed
- SAI status attributes show normal operation
- SAI statistics show proper frame counts

**Pass Criteria:**
- 100% of frames have valid sequence numbers
- Sequence numbers increment correctly
- No sequence number gaps
- SAI_PORT_STAT_LLR_RX_DUPLICATE_SEQ = 0
- SAI status attributes indicate normal operation



#### Frame Sequence Number Wrap Around
- **Test Case ID:** 5
- **Test Case Name:** Frame Sequence Number Wrap Around
- **Objective:** Verify correct sequence number and ACK number handling after 20-bit sequence number wrap around (0 to 524287)
- **Prerequisites:** PR‑01, PR‑03, PR‑04, LLR INIT sequence done.

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status (should remain ADVANCE during and after wrap around)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status (should remain SEND_ACKS during and after wrap around)

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_OK - Count of LLR-eligible frames transmitted with good FCS
- SAI_PORT_STAT_LLR_RX_OK - Count of LLR-eligible frames received with good FCS
- SAI_PORT_STAT_LLR_RX_EXPECTED_SEQ_GOOD - Count of frames received with expected sequence number and good FCS
- SAI_PORT_STAT_LLR_RX_DUPLICATE_SEQ - Count of frames received with duplicate sequence numbers (should remain 0 after wrap around)
- SAI_PORT_STAT_LLR_TX_ACK_CTL_OS - Count of LLR_ACK control ordered sets transmitted
- SAI_PORT_STAT_LLR_RX_ACK_CTL_OS - Count of LLR_ACK control ordered sets received

**Test Steps:**
1. Configure traffic generator to send a burst of frames until close to sequence number wrap around
2. Once close to wrap around enable frame capture on protocol analyzer
3. Continue transmitting frames through the wrap-around transition
4. Analyze captured frames at the wrap-around transition:
   - Verify sequence numbers transition correctly from 524287 to 0
   - Verify sequence numbers continue incrementing correctly after wrap around (0, 1, 2, ...)
   - Verify ACK numbers are correct and handle wrap around properly
   - Verify no duplicate sequence number detection errors occur
5. Verify SAI_PORT_STAT_LLR_TX_OK and SAI_PORT_STAT_LLR_RX_OK counters continue incrementing correctly
6. Check SAI_PORT_STAT_LLR_RX_DUPLICATE_SEQ remains 0
7. Verify SAI_PORT_STAT_LLR_RX_EXPECTED_SEQ_GOOD counter increments correctly for frames after wrap around
8. Verify SAI_PORT_STAT_LLR_TX_ACK_CTL_OS and SAI_PORT_STAT_LLR_RX_ACK_CTL_OS counters continue to increment correctly


**Expected Results:**
- Sequence numbers wrap correctly from 524287 to 0
- Sequence numbers continue incrementing correctly after wrap around
- ACK numbers handle wrap around correctly and maintain proper sequencing
- No false duplicate sequence number detection occurs at wrap-around boundary
- No frame loss occurs at the wrap-around transition
- SAI status attributes show normal operation throughout wrap around
- SAI statistics show correct behavior with no anomalies at wrap-around boundary

**Pass Criteria:**
- Sequence numbers wrap correctly from maximum (524287) to 0
- Sequence numbers increment correctly after wrap around (0, 1, 2, ...)
- ACK numbers are correct before, during, and after wrap around
- SAI_PORT_STAT_LLR_RX_DUPLICATE_SEQ = 0 throughout test (no false positives)
- No frame loss or retransmission issues at wrap-around boundary
- Analyzer capture shows correct sequence numbers at transition point
- SAI status attributes indicate normal operation throughout
- SAI statistics show expected counts with no anomalies



#### Frame Buffering and Acknowledgment
- **Test Case ID:** 6
- **Test Case Name:** Frame Buffering and Acknowledgment
- **Objective:** Validate that the sender buffers frames until acknowledgment is received
- **Prerequisites:** PR‑01, PR‑03. LLR INIT sequence done.

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_ACK_CTL_OS - Count of LLR_ACK control ordered sets transmitted
- SAI_PORT_STAT_LLR_RX_ACK_CTL_OS - Count of LLR_ACK control ordered sets received
- SAI_PORT_STAT_LLR_TX_OK - Count of LLR-eligible frames transmitted with good FCS
- SAI_PORT_STAT_LLR_RX_OK - Count of LLR-eligible frames received with good FCS

**Test Steps:**
1. Configure DUT to transmit 100 frames
2. Monitor DUT's buffer utilization during transmission
3. Introduce controlled delays in acknowledgment responses
4. Measure buffer occupancy and frame retention
5. Monitor SAI_PORT_ATTR_LLR_TX_STATUS should change from 'SAI_PORT_LLR_TX_STATUS_ADVANCE' to SAI_PORT_LLR_TX_STATUS_REPLAY.
6. Verify SAI_PORT_STAT_LLR_TX_ACK_CTL_OS and SAI_PORT_STAT_LLR_RX_ACK_CTL_OS counters
7. Check SAI_PORT_STAT_LLR_TX_OK and SAI_PORT_STAT_LLR_RX_OK counters

**Expected Results:**
- DUT maintains frames in buffer until ACK received
- Buffer utilization increases during ACK delays
- Frames are released from buffer after ACK reception
- SAI status attributes show normal operation
- SAI statistics show proper ACK/ACK_CTL_OS counts

**Pass Criteria:**
- Buffer utilization > 0% during ACK delays
- No premature frame deletion from buffer
- Proper buffer cleanup after ACK reception
- SAI status attributes indicate normal operation
- SAI statistics show expected ACK counts

### Error Detection
#### Loss Detection and NACK Generation
- **Test Case ID:** 7
- **Test Case Name:** Loss Detection and NACK Generation
- **Objective:** Test the LLR's ability to detect lost frames and generate NACKs
- **Prerequisites:** PR‑01, PR‑03, PR‑04 LLR INIT sequence done.

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status (should transition to REPLAY during retransmission)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status (should transition to SEND_NACK then NACK_SENT)

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_NACK_CTL_OS - Count of LLR_NACK control ordered sets transmitted
- SAI_PORT_STAT_LLR_RX_NACK_CTL_OS - Count of LLR_NACK control ordered sets received
- SAI_PORT_STAT_LLR_RX_MISSING_SEQ - Count of frames with missing sequence numbers
- SAI_PORT_STAT_LLR_TX_REPLAY - Count of replay operations completed
- SAI_PORT_STAT_LLR_RX_REPLAY - Count of replay operations detected

**Test Steps:**
1. Configure network impairment to drop 5% of frames randomly
2. Transmit 1000 frames from Traffic Generator
3. Monitor receiver for NACK generation
4. Verify DUT's response to NACKs
5. Monitor SAI_PORT_ATTR_LLR_TX_STATUS and SAI_PORT_ATTR_LLR_RX_STATUS transitions
6. Verify SAI_PORT_STAT_LLR_TX_NACK_CTL_OS and SAI_PORT_STAT_LLR_RX_NACK_CTL_OS counters
7. Check SAI_PORT_STAT_LLR_RX_MISSING_SEQ and SAI_PORT_STAT_LLR_TX_REPLAY counters

**Expected Results:**
- Receiver detects sequence number gaps
- NACKs are generated for missing frames
- DUT retransmits frames upon NACK reception
- SAI status attributes show proper state transitions
- SAI statistics show NACK and replay counts

**Pass Criteria:**
- NACKs are generated when unexpected sequence number is received
- 100% of lost frames are retransmitted
- No false NACKs for successfully received frames
- SAI status attributes show proper state transitions
- SAI statistics show expected NACK and replay counts

#### NACK and retransmission
- **Test Case ID:** 8
- **Test Case Name:** NACK and retransmission
- **Objective:** Test the LLR's response to NACK and retransmissions
- **Prerequisites:** PR‑01, PR‑03, PR‑04

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status (should transition to REPLAY during retransmission)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status (should transition to SEND_NACK then NACK_SENT)

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_NACK_CTL_OS - Count of LLR_NACK control ordered sets transmitted
- SAI_PORT_STAT_LLR_RX_NACK_CTL_OS - Count of LLR_NACK control ordered sets received
- SAI_PORT_STAT_LLR_TX_ACK_CTL_OS - Count of LLR_ACK control ordered sets transmitted (should stop after NACK)
- SAI_PORT_STAT_LLR_RX_ACK_CTL_OS - Count of LLR_ACK control ordered sets received (should stop after NACK)
- SAI_PORT_STAT_LLR_TX_REPLAY - Count of replay operations completed
- SAI_PORT_STAT_LLR_RX_MISSING_SEQ - Count of frames with missing sequence numbers

**Test Steps:**
1. Configure traffic generator to transmit 100 frames
2. Configure network impairment to drop the 50th frame specifically
3. Monitor NACK is received for lost frame
4. Verify there are no ACK's received after NACK is received
5. Verify all the frames including 50th frame is retransmitted
6. Monitor SAI_PORT_ATTR_LLR_TX_STATUS and SAI_PORT_ATTR_LLR_RX_STATUS transitions
7. Verify SAI_PORT_STAT_LLR_TX_NACK_CTL_OS and SAI_PORT_STAT_LLR_RX_NACK_CTL_OS counters
8. Check SAI_PORT_STAT_LLR_TX_ACK_CTL_OS and SAI_PORT_STAT_LLR_RX_ACK_CTL_OS stop incrementing after NACK
9. Verify SAI_PORT_STAT_LLR_TX_REPLAY and SAI_PORT_STAT_LLR_RX_MISSING_SEQ counters

**Expected Results:**
- NACK is generated for the lost 50th frame
- No ACKs received after NACK
- All frames including 50th frame are retransmitted
- Final frame count is 100
- No duplicate frames delivered
- SAI status attributes show proper state transitions
- SAI statistics show NACK and replay counts

**Pass Criteria:**
- NACK generated for lost frame
- No ACKs after NACK
- 100% retransmission success
- All 100 frames delivered
- No duplicate frames
- Retransmission timing within acceptable limits
- SAI status attributes show proper state transitions
- SAI statistics show expected NACK and replay counts

#### Timeout-Based Retransmission
- **Test Case ID:** 9
- **Test Case Name:** Timeout-Based Retransmission
- **Objective:** Ensure sender retransmits frames if ACKs are not received within timeout
- **Prerequisites:** PR‑01, PR‑03.

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status (should transition to REPLAY after timeout)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status (should remain SEND_ACKS during ACK suppression)

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_REPLAY - Count of replay operations completed (should increment after timeout)
- SAI_PORT_STAT_LLR_TX_ACK_CTL_OS - Count of LLR_ACK control ordered sets transmitted (should be suppressed)
- SAI_PORT_STAT_LLR_RX_ACK_CTL_OS - Count of LLR_ACK control ordered sets received (should be suppressed)
- SAI_PORT_STAT_LLR_TX_OK - Count of LLR-eligible frames transmitted with good FCS

**Test Steps:**
1. Configure LLR timeout parameters using SAI_PORT_LLR_PROFILE_ATTR_REPLAY_TIMER_MAX
2. Transmitt 10 frames
3. Suppress ACK responses for specific frames
4. Monitor DUT behavior after timeout period
5. Verify retransmission occurs
6. Monitor SAI_PORT_ATTR_LLR_TX_STATUS and SAI_PORT_ATTR_LLR_RX_STATUS transitions
7. Verify SAI_PORT_STAT_LLR_TX_REPLAY counter increments after timeout
8. Check SAI_PORT_STAT_LLR_TX_ACK_CTL_OS and SAI_PORT_STAT_LLR_RX_ACK_CTL_OS are suppressed
9. Verify SAI_PORT_STAT_LLR_TX_OK counter shows retransmitted frames

**Expected Results:**
- DUT retransmits frames after timeout
- Timeout behavior is consistent and predictable
- No premature retransmissions before timeout
- SAI status attributes show proper state transitions
- SAI statistics show replay and suppressed ACK counts

**Pass Criteria:**
- Retransmission occurs exactly after timeout period
- Timeout accuracy within ±10% of configured value
- No retransmissions before timeout expiration
- SAI status attributes show proper state transitions
- SAI statistics show expected replay and ACK suppression counts


#### Corrupted Frame Detection and Retransmission
- **Test Case ID:** 10
- **Test Case Name:** Corrupted Frame Detection and Retransmission
- **Objective:** Test LLR's ability to detect corrupted frames and request retransmission
- **Prerequisites:** PR‑01, PR‑03, PR‑04

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status (should transition to REPLAY during retransmission)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status (should transition to SEND_NACK then NACK_SENT)

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_RX_BAD - Count of LLR-eligible frames received with bad FCS
- SAI_PORT_STAT_LLR_TX_NACK_CTL_OS - Count of LLR_NACK control ordered sets transmitted
- SAI_PORT_STAT_LLR_RX_NACK_CTL_OS - Count of LLR_NACK control ordered sets received
- SAI_PORT_STAT_LLR_TX_REPLAY - Count of replay operations completed
- SAI_PORT_STAT_LLR_RX_EXPECTED_SEQ_BAD - Count of frames received with expected sequence number but bad FCS

**Test Steps:**
1. Configure network impairment to inject bit errors
2. Transmit 1000 frames
3. Monitor for corruption detection and retransmission requests
4. Verify data integrity after retransmission
5. Monitor SAI_PORT_ATTR_LLR_TX_STATUS and SAI_PORT_ATTR_LLR_RX_STATUS transitions
6. Verify SAI_PORT_STAT_LLR_RX_BAD counter increments for corrupted frames
7. Check SAI_PORT_STAT_LLR_TX_NACK_CTL_OS and SAI_PORT_STAT_LLR_RX_NACK_CTL_OS counters match FCS drop count
8. Verify SAI_PORT_STAT_LLR_TX_REPLAY and SAI_PORT_STAT_LLR_RX_EXPECTED_SEQ_BAD counters

**Expected Results:**
- Corrupted frames are detected by receiver
- NACKs are generated for corrupted frames
- Retransmitted frames are error-free
- SAI status attributes show proper state transitions
- SAI statistics show corruption detection and retransmission counts

**Pass Criteria:**
- 100% of corrupted frames are detected
- All corrupted frames are successfully retransmitted
- Final data integrity is maintained
- SAI status attributes show proper state transitions
- SAI statistics show expected corruption detection and retransmission counts

### Performance
#### High Traffic Performance
- **Test Case ID:** 11
- **Test Case Name:** High Traffic Performance
- **Objective:** Assess LLR performance under high network load
- **Prerequisites:** PR‑01, PR‑03, PR‑04

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status (should remain ADVANCE under normal load)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status (should remain SEND_ACKS under normal load)

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_OK - Count of LLR-eligible frames transmitted with good FCS
- SAI_PORT_STAT_LLR_RX_OK - Count of LLR-eligible frames received with good FCS
- SAI_PORT_STAT_LLR_TX_REPLAY - Count of replay operations completed
- SAI_PORT_STAT_LLR_RX_REPLAY - Count of replay operations detected
- SAI_PORT_STAT_LLR_TX_ACK_CTL_OS - Count of LLR_ACK control ordered sets transmitted
- SAI_PORT_STAT_LLR_RX_ACK_CTL_OS - Count of LLR_ACK control ordered sets received
- SAI_PORT_STAT_LLR_TX_NACK_CTL_OS - Count of LLR_NACK control ordered sets transmitted
- SAI_PORT_STAT_LLR_RX_NACK_CTL_OS - Count of LLR_NACK control ordered sets received

**Test Steps:**
1. Configure traffic generator for 100% line rate
2. Configure network to drop 2% of traffic
3. Run test for 1 hour
4. Measure throughput latency and retransmission rates
5. Monitor SAI_PORT_ATTR_LLR_TX_STATUS and SAI_PORT_ATTR_LLR_RX_STATUS throughout test
6. Verify SAI_PORT_STAT_LLR_TX_OK and SAI_PORT_STAT_LLR_RX_OK counters show expected frame counts
7. Check SAI_PORT_STAT_LLR_TX_REPLAY and SAI_PORT_STAT_LLR_RX_REPLAY counters match packet loss rate
8. Verify SAI_PORT_STAT_LLR_TX_ACK_CTL_OS and SAI_PORT_STAT_LLR_RX_ACK_CTL_OS counters show normal ACK rates
9. Check SAI_PORT_STAT_LLR_TX_NACK_CTL_OS and SAI_PORT_STAT_LLR_RX_NACK_CTL_OS counters

**Expected Results:**
- Throughput remains stable under load
- Latency increase is minimal (< 5%)
- Retransmission rate matches packet loss rate
- SAI status attributes show normal operation
- SAI statistics show expected performance metrics

**Pass Criteria:**
- Throughput ≥ 95% of baseline (without LLR)
- Latency increase ≤ 5% compared to baseline
- Retransmission efficiency ≥ 90%
- SAI status attributes indicate normal operation
- SAI statistics show expected performance metrics


#### Buffer Overflow Handling
- **Test Case ID:** 12
- **Test Case Name:** Buffer Overflow Handling
- **Objective:** Test LLR behavior when buffer capacity is exceeded
- **Prerequisites:** PR‑01, PR‑03, PR‑04

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status (should transition to FLUSH when buffer overflow occurs)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status (should remain SEND_ACKS during overflow)
- SAI_PORT_ATTR_LLR_PROFILE - Configure buffer limits using SAI_PORT_LLR_PROFILE_ATTR_OUTSTANDING_FRAMES_MAX and SAI_PORT_LLR_PROFILE_ATTR_OUTSTANDING_BYTES_MAX

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_DISCARD - Count of LLR-eligible frames discarded due to buffer overflow
- SAI_PORT_STAT_LLR_TX_OK - Count of LLR-eligible frames transmitted with good FCS
- SAI_PORT_STAT_LLR_TX_REPLAY - Count of replay operations completed
- SAI_PORT_STAT_LLR_TX_ACK_CTL_OS - Count of LLR_ACK control ordered sets transmitted
- SAI_PORT_STAT_LLR_RX_ACK_CTL_OS - Count of LLR_ACK control ordered sets received

**Test Steps:**
1. Configure small buffer size using SAI_PORT_LLR_PROFILE_ATTR_OUTSTANDING_FRAMES_MAX (e.g. 100 frames)
2. Generate high traffic with extended ACK delays
3. Monitor buffer utilization and overflow handling
4. Verify system stability
5. Monitor SAI_PORT_ATTR_LLR_TX_STATUS and SAI_PORT_ATTR_LLR_RX_STATUS transitions
6. Verify SAI_PORT_STAT_LLR_TX_DISCARD counter increments when buffer overflow occurs
7. Check SAI_PORT_STAT_LLR_TX_OK and SAI_PORT_STAT_LLR_TX_REPLAY counters show expected behavior
8. Verify SAI_PORT_STAT_LLR_TX_ACK_CTL_OS and SAI_PORT_STAT_LLR_RX_ACK_CTL_OS counters show normal ACK rates

**Expected Results:**
- System handles buffer overflow with BEST_EFFORT (frame_type is set to LLR_INELIGIBLE)
- No system crashes or undefined behavior
- Appropriate error handling and recovery
- SAI status attributes show proper state transitions
- SAI statistics show buffer overflow handling

**Pass Criteria:**
- No system crashes during buffer overflow
- Graceful degradation of performance
- Proper error reporting and logging
- SAI status attributes indicate proper state transitions
- SAI statistics show expected buffer overflow handling


#### Concurrent Multiple Streams
- **Test Case ID:** 13
- **Test Case Name:** Concurrent Multiple Streams
- **Objective:** Test LLR with multiple concurrent data streams
- **Prerequisites:** PR‑01, PR‑03, PR‑04

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status for each stream (should remain ADVANCE under normal load)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status for each stream (should remain SEND_ACKS under normal load)
- SAI_PORT_ATTR_LLR_PROFILE - Configure LLR profiles for each stream

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_OK - Count of LLR-eligible frames transmitted with good FCS (per stream)
- SAI_PORT_STAT_LLR_RX_OK - Count of LLR-eligible frames received with good FCS (per stream)
- SAI_PORT_STAT_LLR_TX_REPLAY - Count of replay operations completed (per stream)
- SAI_PORT_STAT_LLR_RX_REPLAY - Count of replay operations detected (per stream)
- SAI_PORT_STAT_LLR_TX_ACK_CTL_OS - Count of LLR_ACK control ordered sets transmitted (per stream)
- SAI_PORT_STAT_LLR_RX_ACK_CTL_OS - Count of LLR_ACK control ordered sets received (per stream)
- SAI_PORT_STAT_LLR_TX_NACK_CTL_OS - Count of LLR_NACK control ordered sets transmitted (per stream)
- SAI_PORT_STAT_LLR_RX_NACK_CTL_OS - Count of LLR_NACK control ordered sets received (per stream)

**Test Steps:**
1. Configure 10 concurrent data streams
2. Introduce varying packet loss rates per stream
3. Monitor performance and resource utilization
4. Monitor SAI_PORT_ATTR_LLR_TX_STATUS and SAI_PORT_ATTR_LLR_RX_STATUS for each stream
5. Verify SAI_PORT_STAT_LLR_TX_OK and SAI_PORT_STAT_LLR_RX_OK counters for each stream
6. Check SAI_PORT_STAT_LLR_TX_REPLAY and SAI_PORT_STAT_LLR_RX_REPLAY counters for each stream
7. Verify SAI_PORT_STAT_LLR_TX_ACK_CTL_OS and SAI_PORT_STAT_LLR_RX_ACK_CTL_OS counters for each stream
8. Check SAI_PORT_STAT_LLR_TX_NACK_CTL_OS and SAI_PORT_STAT_LLR_RX_NACK_CTL_OS counters for each stream

**Expected Results:**
- All streams maintain independent LLR operation
- No cross-stream interference
- Consistent performance across all streams
- SAI status attributes show normal operation for each stream
- SAI statistics show independent operation for each stream

**Pass Criteria:**
- All streams operate independently
- Performance degradation ≤ 10% compared to single stream
- No resource contention issues
- SAI status attributes indicate normal operation for each stream
- SAI statistics show expected independent operation for each stream

#### Bulk LLR Initialization Performance
- **Test Case ID:** 14
- **Test Case Name:** Bulk LLR Initialization Performance
- **Objective:** Measure the time required to configure and initialize LLR on all ports of the switch simultaneously, bringing all ports to ready state
- **Prerequisites:** PR‑01, PR‑02, PR‑03; Switch with all ports available for testing

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_MODE_LOCAL - Enable LLR local mode on all ports
- SAI_PORT_ATTR_LLR_MODE_REMOTE - Enable LLR remote mode on all ports
- SAI_PORT_ATTR_LLR_PROFILE - Attach LLR profile to all ports
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status for all ports (should transition from INIT to ADVANCE when ready)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status for all ports (should transition to SEND_ACKS when ready)

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_INIT_CTL_OS - Per-port count of INIT control ordered sets transmitted
- SAI_PORT_STAT_LLR_RX_INIT_CTL_OS - Per-port count of INIT control ordered sets received
- SAI_PORT_STAT_LLR_TX_INIT_ECHO_CTL_OS - Per-port count of INIT_ECHO control ordered sets transmitted
- SAI_PORT_STAT_LLR_RX_INIT_ECHO_CTL_OS - Per-port count of INIT_ECHO control ordered sets received

**Test Steps:**
1. Create a single LLR profile to be used for all ports
2. Record start time (T_start) before beginning LLR configuration
3. Enable LLR on all ports simultaneously:
   - Attach LLR profile to all ports using SAI_PORT_ATTR_LLR_PROFILE
   - Enable LLR local mode on all ports using SAI_PORT_ATTR_LLR_MODE_LOCAL
   - Enable LLR remote mode on all ports using SAI_PORT_ATTR_LLR_MODE_REMOTE
4. Monitor initialization sequence for all ports in parallel
5. Poll SAI_PORT_ATTR_LLR_TX_STATUS and SAI_PORT_ATTR_LLR_RX_STATUS for all ports periodically
6. Record the time when all ports reach ready state:
   - SAI_PORT_ATTR_LLR_TX_STATUS = SAI_PORT_LLR_TX_STATUS_ADVANCE
   - SAI_PORT_ATTR_LLR_RX_STATUS = SAI_PORT_LLR_RX_STATUS_SEND_ACKS
7. Record end time (T_end) when all ports are in ready state
8. Calculate total initialization time: T_init = T_end - T_start
9. Verify SAI_PORT_STAT_LLR_TX_INIT_CTL_OS and SAI_PORT_STAT_LLR_RX_INIT_CTL_OS counters for all ports
10. Verify SAI_PORT_STAT_LLR_TX_INIT_ECHO_CTL_OS and SAI_PORT_STAT_LLR_RX_INIT_ECHO_CTL_OS counters for all ports
11. Repeat the test 5 times and calculate average, minimum, and maximum initialization times
12. Monitor system resource utilization (CPU, memory) during bulk initialization
13. Verify no resource contention or errors during concurrent initialization

**Expected Results:**
- All ports successfully initialize LLR simultaneously
- Total initialization time is measured and recorded should be < 10sec
- All ports reach ready state within acceptable time limits
- No resource contention or initialization failures
- Initialization statistics show proper handshake completion for all ports
- System resource utilization remains within acceptable limits during bulk initialization

**Pass Criteria:**
- All ports successfully initialize LLR (100% success rate)
- Total initialization time for all ports is measured and documented should be < 10sec
- No initialization failures or errors during bulk configuration
- All ports reach ready state (TX_STATUS = ADVANCE, RX_STATUS = SEND_ACKS)
- Initialization statistics (INIT_CTL_OS, INIT_ECHO_CTL_OS) show proper handshake for all ports
- No resource contention or system instability during concurrent initialization
- System resource utilization (CPU, memory) remains within acceptable limits
- Test results are repeatable across multiple iterations

**Performance Metrics to Record:**
- Total initialization time (T_init) for all ports
- Minimum initialization time across all ports
- Maximum initialization time across all ports
- Average initialization time across 5 test iterations
- CPU utilization during bulk initialization
- Memory utilization during bulk initialization

### Edge Cases
#### Maximum Retry Limit
- **Test Case ID:** 15
- **Test Case Name:** Maximum Retry Limit
- **Objective:** Test behavior when maximum retry attempts are reached
- **Prerequisites:** PR‑01, PR‑03, PR‑04

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status (should transition to FLUSH when retry limit reached)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status (should remain SEND_ACKS during retry limit)
- SAI_PORT_ATTR_LLR_PROFILE - Configure retry limit using SAI_PORT_LLR_PROFILE_ATTR_REPLAY_COUNT_MAX

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_REPLAY - Count of replay operations completed (should not exceed retry limit)
- SAI_PORT_STAT_LLR_RX_REPLAY - Count of replay operations detected
- SAI_PORT_STAT_LLR_TX_DISCARD - Count of LLR-eligible frames discarded when retry limit reached
- SAI_PORT_STAT_LLR_TX_NACK_CTL_OS - Count of LLR_NACK control ordered sets transmitted
- SAI_PORT_STAT_LLR_RX_NACK_CTL_OS - Count of LLR_NACK control ordered sets received

**Test Steps:**
1. Configure maximum retry limit using SAI_PORT_LLR_PROFILE_ATTR_REPLAY_COUNT_MAX (e.g. 3 attempts)
2. Simulate persistent packet loss for specific frames
3. Monitor system behavior at retry limit
4. Verify proper error handling and reporting
5. Monitor SAI_PORT_ATTR_LLR_TX_STATUS and SAI_PORT_ATTR_LLR_RX_STATUS transitions
6. Verify SAI_PORT_STAT_LLR_TX_REPLAY counter does not exceed retry limit
7. Check SAI_PORT_STAT_LLR_TX_DISCARD counter increments when retry limit reached
8. Verify SAI_PORT_STAT_LLR_TX_NACK_CTL_OS and SAI_PORT_STAT_LLR_RX_NACK_CTL_OS counters show expected behavior

**Expected Results:**
- System stops retrying after maximum attempts
- Appropriate error reporting to higher layers
- No infinite retry loops
- SAI status attributes show proper state transitions
- SAI statistics show retry limit handling

**Pass Criteria:**
- Retry attempts do not exceed configured limit
- Proper error escalation to higher layers
- System remains stable and responsive
- SAI status attributes indicate proper state transitions
- SAI statistics show expected retry limit behavior

#### Rapid Successive Errors
- **Test Case ID:** 16
- **Test Case Name:** Rapid Successive Errors
- **Objective:** Test LLR behavior with rapid successive frame losses
- **Prerequisites:** PR‑01, PR‑03, PR‑04

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status (should transition to REPLAY during burst handling)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status (should transition to SEND_NACK then NACK_SENT during burst)

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_REPLAY - Count of replay operations completed (should handle burst efficiently)
- SAI_PORT_STAT_LLR_RX_REPLAY - Count of replay operations detected
- SAI_PORT_STAT_LLR_RX_MISSING_SEQ - Count of frames with missing sequence numbers (should match burst size)
- SAI_PORT_STAT_LLR_TX_NACK_CTL_OS - Count of LLR_NACK control ordered sets transmitted
- SAI_PORT_STAT_LLR_RX_NACK_CTL_OS - Count of LLR_NACK control ordered sets received
- SAI_PORT_STAT_LLR_TX_OK - Count of LLR-eligible frames transmitted with good FCS
- SAI_PORT_STAT_LLR_RX_OK - Count of LLR-eligible frames received with good FCS

**Test Steps:**
1. Configure burst packet loss (100 consecutive frames)
2. Send 1000 frames
3. Monitor retransmission behavior and timing
4. Verify system recovery
5. Monitor SAI_PORT_ATTR_LLR_TX_STATUS and SAI_PORT_ATTR_LLR_RX_STATUS transitions
6. Verify SAI_PORT_STAT_LLR_TX_REPLAY and SAI_PORT_STAT_LLR_RX_REPLAY counters handle burst efficiently
7. Check SAI_PORT_STAT_LLR_RX_MISSING_SEQ counter matches burst size
8. Verify SAI_PORT_STAT_LLR_TX_NACK_CTL_OS and SAI_PORT_STAT_LLR_RX_NACK_CTL_OS counters show expected behavior
9. Check SAI_PORT_STAT_LLR_TX_OK and SAI_PORT_STAT_LLR_RX_OK counters show recovery

**Expected Results:**
- System handles burst losses efficiently
- Retransmission timing is appropriate
- System recovers quickly after burst errors
- SAI status attributes show proper state transitions
- SAI statistics show burst handling and recovery

**Pass Criteria:**
- All burst frames are eventually retransmitted
- Recovery time ≤ 2x normal retransmission time
- No system instability during burst handling
- SAI status attributes indicate proper state transitions
- SAI statistics show expected burst handling and recovery

#### LLR Warm Boot Configuration Persistence
- **Test Case ID:** 17
- **Test Case Name:** LLR Warm Boot Configuration Persistence
- **Objective:** Verify that all LLR configurations are preserved and properly restored after warm boot, ensuring LLR continues to operate correctly afte
- **Prerequisites:** PR‑01, PR‑03; Warm boot capability on DUT

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_MODE_LOCAL - Verify LLR local mode is preserved after warm boot
- SAI_PORT_ATTR_LLR_MODE_REMOTE - Verify LLR remote mode is preserved after warm boot
- SAI_PORT_ATTR_LLR_PROFILE - Verify LLR profile attachment is preserved after warm boot
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status after warm boot (should reinitialize and reach ADVANCE)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status after warm boot (should reinitialize and reach SEND_ACKS)
- SAI_PORT_LLR_PROFILE_ATTR_OUTSTANDING_FRAMES_MAX - Verify profile parameter is preserved
- SAI_PORT_LLR_PROFILE_ATTR_OUTSTANDING_BYTES_MAX - Verify profile parameter is preserved
- SAI_PORT_LLR_PROFILE_ATTR_REPLAY_TIMER_MAX - Verify profile parameter is preserved
- SAI_PORT_LLR_PROFILE_ATTR_REPLAY_COUNT_MAX - Verify profile parameter is preserved
- SAI_PORT_LLR_PROFILE_ATTR_PCS_LOST_TIMEOUT - Verify profile parameter is preserved
- SAI_PORT_LLR_PROFILE_ATTR_DATA_AGE_TIMER_MAX - Verify profile parameter is preserved
- SAI_PORT_LLR_PROFILE_ATTR_LLR_INIT_FRAME_ACTION - Verify profile parameter is preserved
- SAI_PORT_LLR_PROFILE_ATTR_LLR_FLUSH_FRAME_ACTION - Verify profile parameter is preserved
- SAI_PORT_LLR_PROFILE_ATTR_RE_INIT_ON_FLUSH - Verify profile parameter is preserved
- SAI_PORT_LLR_PROFILE_ATTR_CTLOS_TARGET_SPACING - Verify profile parameter is preserved

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_INIT_CTL_OS - Count of INIT control ordered sets transmitted after warm boot
- SAI_PORT_STAT_LLR_RX_INIT_CTL_OS - Count of INIT control ordered sets received after warm boot
- SAI_PORT_STAT_LLR_TX_INIT_ECHO_CTL_OS - Count of INIT_ECHO control ordered sets transmitted after warm boot
- SAI_PORT_STAT_LLR_RX_INIT_ECHO_CTL_OS - Count of INIT_ECHO control ordered sets received after warm boot
- SAI_PORT_STAT_LLR_TX_OK - Verify LLR operation resumes correctly after warm boot
- SAI_PORT_STAT_LLR_RX_OK - Verify LLR operation resumes correctly after warm boot

**Test Steps:**
1. Create and configure LLR profile(s) with all parameters:
   - Set mandatory parameters (OUTSTANDING_FRAMES_MAX, OUTSTANDING_BYTES_MAX)
   - Set all optional parameters with specific values
   - Record all configured parameter values
2. Enable LLR on multiple ports:
   - Attach LLR profile(s) to ports using SAI_PORT_ATTR_LLR_PROFILE
   - Enable LLR local mode on ports using SAI_PORT_ATTR_LLR_MODE_LOCAL
   - Enable LLR remote mode on ports using SAI_PORT_ATTR_LLR_MODE_REMOTE
3. Verify LLR is operational before warm boot:
   - Verify SAI_PORT_ATTR_LLR_TX_STATUS and SAI_PORT_ATTR_LLR_RX_STATUS show normal operation
   - Send test traffic and verify LLR is functioning correctly
   - Record all LLR configuration state before warm boot
4. Perform warm boot on DUT
5. After warm boot completes, verify LLR profile persistence:
   - Read back SAI_PORT_ATTR_LLR_PROFILE for all configured ports
   - Verify profile IDs match the pre-warm boot configuration
   - Read back all profile parameters using profile object ID
   - Compare all parameter values with pre-warm boot configuration
6. Verify LLR mode persistence:
   - Read SAI_PORT_ATTR_LLR_MODE_LOCAL for all configured ports
   - Read SAI_PORT_ATTR_LLR_MODE_REMOTE for all configured ports
   - Verify modes match pre-warm boot configuration
7. Verify LLR reinitialization after warm boot:
   - Monitor SAI_PORT_ATTR_LLR_TX_STATUS transitions (should go from INIT to ADVANCE)
   - Monitor SAI_PORT_ATTR_LLR_RX_STATUS transitions (should go to SEND_ACKS)
   - Verify initialization handshake completes successfully
8. Verify SAI_PORT_STAT_LLR_TX_INIT_CTL_OS and SAI_PORT_STAT_LLR_RX_INIT_CTL_OS counters show initialization
9. Verify SAI_PORT_STAT_LLR_TX_INIT_ECHO_CTL_OS and SAI_PORT_STAT_LLR_RX_INIT_ECHO_CTL_OS counters show initialization
10. Send test traffic after warm boot:
    - Transmit frames
    - Verify frames are processed correctly with LLR
    - Verify retransmission works if frames are dropped
11. Verify SAI_PORT_STAT_LLR_TX_OK and SAI_PORT_STAT_LLR_RX_OK counters increment correctly
12. Verify no configuration drift or corruption after warm boot

**Expected Results:**
- All LLR profiles are preserved after warm boot with identical parameter values
- All LLR mode configurations (local/remote) are preserved after warm boot
- All LLR profile attachments to ports are preserved after warm boot
- LLR reinitializes successfully after warm boot on all configured ports
- All ports reach ready state (TX_STATUS = ADVANCE, RX_STATUS = SEND_ACKS) after warm boot
- LLR functionality resumes correctly after warm boot
- Initialization handshake completes successfully after warm boot
- Test traffic flows correctly with LLR after warm boot
- No configuration loss or corruption observed

**Pass Criteria:**
- 100% of LLR profiles are preserved with identical parameter values after warm boot
- 100% of LLR mode configurations are preserved after warm boot
- 100% of LLR profile-to-port attachments are preserved after warm boot
- All configured ports reinitialize LLR successfully after warm boot
- All ports reach ready state (TX_STATUS = ADVANCE, RX_STATUS = SEND_ACKS) after warm boot
- Initialization statistics (INIT_CTL_OS, INIT_ECHO_CTL_OS) show successful handshake
- LLR functionality (frame transmission, retransmission) works correctly after warm boot
- No configuration drift or corruption detected
- Test traffic flows correctly with LLR after warm boot
- No errors or warnings related to LLR configuration during warm boot

### Interoperability
#### Mixed LLR/Non-LLR Traffic
- **Test Case ID:** 18
- **Test Case Name:** Mixed LLR/Non-LLR Traffic
- **Objective:** Test LLR operation with mixed traffic types
- **Prerequisites:** PR‑01, PR‑03, PR‑04

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status (should remain ADVANCE for LLR traffic)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status (should remain SEND_ACKS for LLR traffic)
- SAI_PORT_ATTR_LLR_MODE_LOCAL - Enable LLR local mode for LLR traffic
- SAI_PORT_ATTR_LLR_MODE_REMOTE - Enable LLR remote mode for LLR traffic

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_OK - Count of LLR-eligible frames transmitted with good FCS (should only count LLR traffic)
- SAI_PORT_STAT_LLR_RX_OK - Count of LLR-eligible frames received with good FCS (should only count LLR traffic)
- SAI_PORT_STAT_LLR_TX_REPLAY - Count of replay operations completed (should only affect LLR traffic)
- SAI_PORT_STAT_LLR_RX_REPLAY - Count of replay operations detected (should only affect LLR traffic)
- SAI_PORT_STAT_LLR_TX_ACK_CTL_OS - Count of LLR_ACK control ordered sets transmitted (should only affect LLR traffic)
- SAI_PORT_STAT_LLR_RX_ACK_CTL_OS - Count of LLR_ACK control ordered sets received (should only affect LLR traffic)
- SAI_PORT_STAT_LLR_TX_NACK_CTL_OS - Count of LLR_NACK control ordered sets transmitted (should only affect LLR traffic)
- SAI_PORT_STAT_LLR_RX_NACK_CTL_OS - Count of LLR_NACK control ordered sets received (should only affect LLR traffic)

**Test Steps:**
1. Configure 50% LLR-eligible traffic 50% LLR-ineligible traffic
2. Transmit mixed traffic with packet loss
3. Monitor LLR behavior and standard traffic impact
4. Verify no cross-interference
5. Monitor SAI_PORT_ATTR_LLR_TX_STATUS and SAI_PORT_ATTR_LLR_RX_STATUS for LLR traffic
6. Verify SAI_PORT_STAT_LLR_TX_OK and SAI_PORT_STAT_LLR_RX_OK counters only count LLR traffic
7. Check SAI_PORT_STAT_LLR_TX_REPLAY and SAI_PORT_STAT_LLR_RX_REPLAY counters only affect LLR traffic
8. Verify SAI_PORT_STAT_LLR_TX_ACK_CTL_OS and SAI_PORT_STAT_LLR_RX_ACK_CTL_OS counters only affect LLR traffic
9. Check SAI_PORT_STAT_LLR_TX_NACK_CTL_OS and SAI_PORT_STAT_LLR_RX_NACK_CTL_OS counters only affect LLR traffic

**Expected Results:**
- LLR eligible pkts are successfully transmitted with and without retransmissions
- Latency of LLR eligible pkts is less than 3xRTT (assuming 2 is the max number of retries)
- SAI status attributes show normal operation for LLR traffic
- SAI statistics show LLR traffic only

**Pass Criteria:**
- LLR traffic maintains retry functionality
- LLR ineligible traffic performance unchanged
- No interference between traffic types
- SAI status attributes, SAI_PORT_ATTR_LLR_TX_STATUS indicates "ADVANCE" and SAI_PORT_ATTR_LLR_RX_STATUS indicate "SEND_ACK" status
- SAI statistics show expected LLR traffic behavior

### PFC Integration
#### LLR NACK Response During PFC Pause
- **Test Case ID:** 19
- **Test Case Name:** LLR NACK Response During PFC Pause
- **Objective:** Test LLR response to NACK when PFC pause is active
- **Prerequisites:** PR‑01, PR‑03, PR‑04, PFC features enabled; PFC pause capability

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status (should remain ADVANCE during PFC pause)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status (should transition to SEND_NACK then NACK_SENT during PFC pause)

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_NACK_CTL_OS - Count of LLR_NACK control ordered sets transmitted (should continue during PFC pause)
- SAI_PORT_STAT_LLR_RX_NACK_CTL_OS - Count of LLR_NACK control ordered sets received (should continue during PFC pause)
- SAI_PORT_STAT_LLR_TX_REPLAY - Count of replay operations completed (should continue during PFC pause)
- SAI_PORT_STAT_LLR_RX_REPLAY - Count of replay operations detected (should continue during PFC pause)
- SAI_PORT_STAT_LLR_TX_OK - Count of LLR-eligible frames transmitted with good FCS (should stop during PFC pause)
- SAI_PORT_STAT_LLR_RX_OK - Count of LLR-eligible frames received with good FCS (should stop during PFC pause)
- SAI_PORT_STAT_LLR_TX_ACK_CTL_OS - Count of LLR_ACK control ordered sets transmitted (should continue during PFC pause)
- SAI_PORT_STAT_LLR_RX_ACK_CTL_OS - Count of LLR_ACK control ordered sets received (should continue during PFC pause)

**Test Steps:**
1. Enable LLR and PFC on DUT using SAI_PORT_ATTR_LLR_MODE_LOCAL/REMOTE
2. Generate traffic to trigger PFC pause frames
3. Inject packet loss during PFC pause period
4. Monitor LLR retry behavior during pause
5. Monitor SAI_PORT_ATTR_LLR_TX_STATUS and SAI_PORT_ATTR_LLR_RX_STATUS during PFC pause
6. Verify SAI_PORT_STAT_LLR_TX_NACK_CTL_OS and SAI_PORT_STAT_LLR_RX_NACK_CTL_OS counters continue during PFC pause
7. Check SAI_PORT_STAT_LLR_TX_REPLAY and SAI_PORT_STAT_LLR_RX_REPLAY counters continue during PFC pause
8. Verify SAI_PORT_STAT_LLR_TX_OK and SAI_PORT_STAT_LLR_RX_OK counters stop during PFC pause
9. Check SAI_PORT_STAT_LLR_TX_ACK_CTL_OS and SAI_PORT_STAT_LLR_RX_ACK_CTL_OS counters continue during PFC pause

**Expected Results:**
- LLR retransmits for all the NACKS received during PFC pause
- No new frames transmitted during pause
- SAI status attributes show proper operation during PFC pause
- SAI statistics show expected behavior during PFC pause

**Pass Criteria:**
- LLR retransmissions not affected by pause
- SAI status attributes indicate proper operation during PFC pause
- SAI statistics show expected behavior during PFC pause


#### LLR retry response During PFC Pause
- **Test Case ID:** 20
- **Test Case Name:** LLR retry response During PFC Pause
- **Objective:** Test LLR retry when PFC pause is active
- **Prerequisites:** PR‑01, PR‑03, PR‑04, PFC features enabled; PFC pause capability

**SAI Attributes to Test:**
- SAI_PORT_ATTR_LLR_TX_STATUS - Monitor TX status (should transition to REPLAY during timeout while PFC pause is active)
- SAI_PORT_ATTR_LLR_RX_STATUS - Monitor RX status (should remain SEND_ACKS during PFC pause)
- SAI_PORT_ATTR_LLR_MODE_LOCAL - Enable LLR local mode
- SAI_PORT_ATTR_LLR_MODE_REMOTE - Enable LLR remote mode
- SAI_PORT_ATTR_LLR_PROFILE - Configure timeout parameters using SAI_PORT_LLR_PROFILE_ATTR_REPLAY_TIMER_MAX

**SAI Statistics to Monitor:**
- SAI_PORT_STAT_LLR_TX_REPLAY - Count of replay operations completed (should continue during PFC pause)
- SAI_PORT_STAT_LLR_RX_REPLAY - Count of replay operations detected (should continue during PFC pause)
- SAI_PORT_STAT_LLR_TX_OK - Count of LLR-eligible frames transmitted with good FCS (should stop during PFC pause)
- SAI_PORT_STAT_LLR_RX_OK - Count of LLR-eligible frames received with good FCS (should stop during PFC pause)
- SAI_PORT_STAT_LLR_TX_ACK_CTL_OS - Count of LLR_ACK control ordered sets transmitted (should be suppressed during PFC pause)
- SAI_PORT_STAT_LLR_RX_ACK_CTL_OS - Count of LLR_ACK control ordered sets received (should be suppressed during PFC pause)
- SAI_PORT_STAT_LLR_TX_DISCARD - Count of LLR-eligible frames discarded (should not increment during PFC pause)

**Test Steps:**
1. Enable LLR and PFC on DUT using SAI_PORT_ATTR_LLR_MODE_LOCAL/REMOTE
2. Configure timeout parameters using SAI_PORT_ATTR_LLR_PROFILE and SAI_PORT_LLR_PROFILE_ATTR_REPLAY_TIMER_MAX
3. Generate traffic to trigger PFC pause frames
4. Suppress ACK responses for specific frames
5. Monitor LLR retry behavior during pause
6. Monitor SAI_PORT_ATTR_LLR_TX_STATUS and SAI_PORT_ATTR_LLR_RX_STATUS during PFC pause
7. Verify SAI_PORT_STAT_LLR_TX_REPLAY and SAI_PORT_STAT_LLR_RX_REPLAY counters continue during PFC pause
8. Check SAI_PORT_STAT_LLR_TX_OK and SAI_PORT_STAT_LLR_RX_OK counters stop during PFC pause
9. Verify SAI_PORT_STAT_LLR_TX_ACK_CTL_OS and SAI_PORT_STAT_LLR_RX_ACK_CTL_OS counters are suppressed during PFC pause
10. Check SAI_PORT_STAT_LLR_TX_DISCARD counter does not increment during PFC pause

**Expected Results:**
- LLR retransmits the frames after TIMEOUT during PFC pause
- No new frames transmitted during pause
- SAI status attributes show proper operation during PFC pause
- SAI statistics show expected behavior during PFC pause

**Pass Criteria:**
- LLR retransmissions not affected by pause
- SAI status attributes indicate proper operation during PFC pause
- SAI statistics show expected behavior during PFC pause

## References
1. UEC LLR Specification: https://groups.ultraethernet.org/wg/UE-LL/document/957
2. Proposed SAI Attributes: https://groups.ultraethernet.org/wg/UE-MGT/document/3320
3. Link Layer Retry support in SAI: https://github.com/rck-innovium/SAI/pull/1
