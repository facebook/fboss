# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

"""
This Python3 script is to remediate the FBOSS 100G links where the optics has
gone into some bad state. Symptoms: (1) The port is having 200G CMIS optics
configured in 100G mode (2) The optics state machine is in DATAPATH_INITIALIZED
state. This script identifies all such bad ports on a given list of swicthes
and remediates them by re-applying the optics Stage set 0 config once again
"""
import subprocess
from time import sleep

"""
Return the list of all the switches which needs fix
"""


def get_all_switch_list():
    hostList = {
        "rsw039.p023.f01.nha3.tfbnw.net",
        "rsw019.p016.f01.nha3.tfbnw.net",
        "rsw031.p016.f01.nha3.tfbnw.net",
        "rsw013.p024.f01.nha3.tfbnw.net",
        "rsw033.p024.f01.nha3.tfbnw.net",
        "rsw004.p024.f01.nha3.tfbnw.net",
    }
    return hostList


"""
Return the list of bad ports for a switch. The bad ports are in Enabled state
but their operational state is Down. They are configured in 100G mode. These
port optics data path state machine is in DATAPATH_INITIALIZED state indicating
problem with optics config
"""


def get_bad_port_list(hostLogin):
    out = subprocess.check_output(
        ["fboss", "-H", hostLogin, "port", "state", "show"], timeout=10
    )
    outlines = out.splitlines()
    portList = ()
    for line in outlines:
        if "Enabled" in str(line) and "Down" in str(line) and "100G" in str(line):
            wlist = line.split()
            portList.append(wlist[1].decode())
    badPortList = ()
    for port in portList:
        qsfpUtilCmd = "sudo wedge_qsfp_util " + port
        out2 = subprocess.check_output(
            ["ssh", "netops@" + hostLogin, qsfpUtilCmd], timeout=10
        )
        if "DATAPATH_INITIALIZED" in str(out2):
            badPortList.append(port)
    return badPortList


"""
To remediate the optics, we need to reapply the Stage Set 0 config once again
using i2c register write
"""


def remediatePort(hostLogin, portName):
    qsfpUtilCmd = (
        "sudo wedge_qsfp_util "
        + portName
        + " --write_reg --page 0x10 --offset 143 --data 0x0f"
    )
    subprocess.check_output(["ssh", "netops@" + hostLogin, qsfpUtilCmd], timeout=10)


"""
Remediate the ports on a given list of switches
1. Find the list of bad ports on a given list of switches
2. Apply remediation to these optics
3. Check once again the state of all these ports
"""


def main():
    switchList = get_all_switch_list()
    print("Switches needs link fix: ", len(switchList))
    for switchHost in switchList:
        badPorts = get_bad_port_list(switchHost)
        print("Switch ", switchHost, " needs fix for ", len(badPorts), " ports")
        for port in badPorts:
            remediatePort(switchHost, port)

    print("All switch ports remediated, Checking once again in 10 sec")
    sleep(10)
    for switchHost in switchList:
        badPorts = get_bad_port_list(switchHost)
        print("Switch ", switchHost, " Number of bad ports ", len(badPorts))


if __name__ == "__main__":
    main()
