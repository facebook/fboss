# pyre-strict
from fboss.util.fixmyfboss.check import check
from fboss.util.fixmyfboss.config import load_pm_config
from fboss.util.fixmyfboss.status import Error, Problem, Warning
from fboss.util.fixmyfboss.utils import run_cmd


def list_pci_devices() -> list[str]:
    """
    List PCI devices
    """
    stdout = run_cmd("lspci -n -v").stdout
    return stdout.splitlines()


@check
def all_pci_devices_present() -> Error | Problem | Warning | None:
    """
    Check if all PCI devices are present
    """

    # Ingest platform_manager.json for the device platform
    pm_config = load_pm_config()
    if not pm_config:
        return Error(
            description=("Platform unit config not found"),
        )

    pci_devices = list_pci_devices()
    if not pci_devices:
        return Problem(
            description=("No PCI devices found"),
            manual_remediation=(
                "Reboot the device and check if the PCI devices are present"
            ),
        )

    missing_pci_devices = []
    for pm_unit_config in pm_config.pmUnitConfigs.values():
        for pci_device_config in pm_unit_config.pciDeviceConfigs:
            vendor_id = pci_device_config.vendorId.removeprefix("0x")
            device_id = pci_device_config.deviceId.removeprefix("0x")
            subsystem_vendor_id = pci_device_config.subSystemVendorId.removeprefix("0x")
            subsystem_device_id = pci_device_config.subSystemDeviceId.removeprefix("0x")

            # check if pci device is present in lspci output
            device_found = False
            for idx in range(len(pci_devices)):
                if (
                    f"{vendor_id}:{device_id}" in pci_devices[idx]
                    and f"Subsystem: {subsystem_vendor_id}:{subsystem_device_id}"
                    in pci_devices[idx + 1]
                ):
                    device_found = True
                    break
            if not device_found:
                missing_pci_devices.append(pci_device_config.pmUnitScopedName)

    if missing_pci_devices:
        return Problem(
            description=(f"PCI Devices not present: {missing_pci_devices}"),
            manual_remediation=(
                "Reboot the device and check if the PCI devices are present"
            ),
        )

    return None
