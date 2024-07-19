# Platform SW Requirements for Switch System Vendors

### Version 1.0.0

## Feature list

1. **BSP Feature Requirements**
    1. Kernel modules for accessing :
        1. FPGA
        2. CPLD
        3. Sensors
        4. Power chip / Hotswap
        5. PSU / PEM (optional)
        6. Fan
    2. User level drivers and APIs that supports
        1. Accessing all Optics DOM / CMIS over I2C
        2. Accessing retimers / macsec over MDIO (if applicable)
        3. Reading and parsing all sensor values (also checks if minor / major
           alarms triggered)
        5. Reading and parsing all power monitoring values (hotswaps and VRDs)
        6. Reading and parsing all PSU monitoring values
        7. Reading/Writing and parsing all Fan RPMs and PWMs
        8. Power sequencing functions for
            1. Turning on/off or toggle reset CPU complex
            2. Turning on/off ASIC
            3. Power-cycling the whole chassis
    3. APIs written in C++ or linkable to C++ executables for upgrading the
       following
        1. BIOS
        2. FPGA
        3. CPLD
        4. Power chip (if applicable)
        5. EEPROM used by control plane switch
        6. EEPROM for ASIC PCIE SERDES (if applicable)
        7. SPI FLASH for retimer / gearbox (if applicable)
    5. Sensor configuration that is compatible with Meta’s sensor_service
    6. Thermal model (configuration) that is compatible with Meta’s fan_service

2. **BMC Requirements**
    1. Vendor BMC image with the following features:
        1. Serial console access to CPU (sol.sh)
            1. Includes mTerm utility
        2. Power cycling features
            1. Turning on/off or toggle reset CPU complex
            2. Turning on/off ASIC
            3. Power-cycling the whole chassis
        3. Back-up upgrade utility for upgrading
            1. BIOS
            2. Main FPGA / CPLD for power sequencing the board
        4. Back-up Fan PWM access utility
        5. openbmc-ipmid
            1. Must support the SEL logging feature.
            2. Use platform config driven instead of using LIBPAL.
            3. Use KCSd as the underlying stack for system bus.
            4. Can communicate with X86’s ipmitool.
            5. FPGA supports snooping the SEL event msg. And user application to
               poll these data.  (optional?)
            7. Use the fboss-github’s openbmc elbert ipmid-v2 code as a
               reference.

## Release timing and SLA

1. **BSP**

    1. The vendor will release the initial version of BSP at or before the 1st
       prototype fab out
    3. The vendor will fix bugs found by Meta and the vendor during EVT, and
       release a BSP at the end of EVT
    5. The vendor will fix bugs found by Meta and the vendor during DVT, and
       release a BSP at the end of DVT
    7. The vendor will fix all bugs found and release a BSP at the end of EVT

2. **BMC**
    1. The vendor will submit BMC source code in real time, to shared private
       git hub repo for Meta review.
    3. The vendor will release the initial version of BMC at or before the 1st
       prototype arrival (to Meta)
    5. The vendor will fix bugs found by Meta and the vendor during EVT, and
       release a BMC at the end of EVT
    7. The vendor will fix all bugs and release a BMC at the end of DVT

3. **SLA**
    1. For urgent bugs / issues that blocks Meta’s progress, the vendor will
       release a patch (BSP), PR (BMC) or the new BIOS image within 4 business
       days.
    3. For less urgent bugs and issues, the vendor will release a patch (BSP),
       PR (BMC) or the new BIOS image within 8 business days.
