# pyre-strict

import json
from dataclasses import dataclass
from typing import Optional

from dataclasses_json import dataclass_json
from fboss.platform.bsp_tests.cdev_types import (
    AuxDevice,
    AuxDeviceType,
    DeviceTestData,
    FpgaSpec,
    I2CAdapter,
    I2CDevice,
    I2cInfo,
)

from fboss.platform.platform_manager.platform_manager_config.types import (
    BspKmodsFile,
    PlatformConfig,
)

from thrift.py3 import Protocol, serializer


@dataclass_json
@dataclass
class TestDevice:
    pmUnit: str
    pmUnitScopedName: str
    deviceTestData: DeviceTestData


@dataclass_json
@dataclass
class TestConfig:
    platform: str
    vendor: str
    testDevices: list[TestDevice]


@dataclass_json
@dataclass
class PMDevice:
    pmUnit: str
    pmUnitScopedName: str
    device: FpgaSpec


@dataclass_json
@dataclass
class RuntimeConfig:
    platform: str
    vendor: str
    fpgas: list[PMDevice]
    kmods: BspKmodsFile


class ConfigLib:
    def __init__(self, pm_config_dir: Optional[str] = None) -> None:
        self.pm_config_dir = pm_config_dir

    def get_platform_manager_config(self, platform_name: str) -> PlatformConfig:
        content: str = ""
        with open(
            f"{self.pm_config_dir}/{platform_name}/platform_manager.json"
        ) as file:
            content = file.read()

        return serializer.deserialize(
            PlatformConfig,
            json.dumps(json.loads(content)).encode("utf-8"),
            protocol=Protocol.JSON,
        )

    def get_actual_adapter(
        self,
        pm_conf: PlatformConfig,
        source_unit_name: str,
        source_bus_name: str,
        slot_type: str,
    ) -> tuple[str, str, int]:
        bus_name, bus_idx = source_bus_name, 0
        if "@" in source_bus_name:
            bus_name = source_bus_name.split("@")[0]
            bus_idx = int(source_bus_name.split("@")[1])

        if bus_name != "INCOMING":
            # Adapter is in the same PMUnit
            return (source_unit_name, bus_name, bus_idx)

        for pm_unit_name, pm_unit in pm_conf.pmUnitConfigs.items():
            outgoing = pm_unit.outgoingSlotConfigs.get(f"{slot_type}@0")
            if not outgoing:
                continue
            actual_bus_name = outgoing.outgoingI2cBusNames[bus_idx]
            if actual_bus_name.startswith("INCOMING"):
                return self.get_actual_adapter(
                    pm_conf, pm_unit_name, actual_bus_name, pm_unit.pluggedInSlotType
                )
            if "@" in actual_bus_name:
                return (
                    pm_unit_name,
                    actual_bus_name.split("@")[0],
                    int(actual_bus_name.split("@")[1]),
                )
            return (pm_unit_name, actual_bus_name, 0)
        raise ValueError(
            f"Unable to find adapter for incoming bus: {source_unit_name}, {bus_name}"
        )

    def build_runtime_config(
        self, test_config: TestConfig, pm_conf: PlatformConfig, kmods: BspKmodsFile
    ) -> RuntimeConfig:
        # Add all PCIDevices and Adapters from PMConfig
        pm_devices = []
        for unit_name, pm_unit in pm_conf.pmUnitConfigs.items():
            for dev in pm_unit.pciDeviceConfigs:
                fpga = FpgaSpec(
                    dev.vendorId,
                    dev.deviceId,
                    dev.subSystemVendorId,
                    dev.subSystemDeviceId,
                )
                pm_device = PMDevice(
                    pmUnit=unit_name, pmUnitScopedName=dev.pmUnitScopedName, device=fpga
                )
                for adapter in dev.i2cAdapterConfigs:
                    auxDev = adapter.fpgaIpBlockConfig
                    fpga.i2cAdapters.append(
                        I2CAdapter(
                            auxDev.pmUnitScopedName,
                            AuxDevice(
                                AuxDeviceType.I2C,
                                auxDev.deviceName,
                                auxDev.csrOffset,
                                i2cInfo=I2cInfo(numChannels=adapter.numberOfAdapters),
                            ),
                        ),
                    )
                pm_devices.append(pm_device)

        # add all i2cDevices from PMConfig
        device_to_adapter_pm_unit: dict[tuple[str, str], str] = {}
        for unit_name, pm_unit in pm_conf.pmUnitConfigs.items():
            for pm_dev in pm_unit.i2cDeviceConfigs:
                adapter_pm_unit, adapter_name, channel = self.get_actual_adapter(
                    pm_conf,
                    unit_name,
                    pm_dev.busName,
                    pm_unit.pluggedInSlotType,
                )
                device_to_adapter_pm_unit[(unit_name, pm_dev.pmUnitScopedName)] = (
                    adapter_pm_unit
                )
                dev = I2CDevice(
                    pm_dev.pmUnitScopedName,
                    channel,
                    pm_dev.kernelDeviceName,
                    pm_dev.address,
                )
                try:
                    adapter = find_adapter(pm_devices, adapter_pm_unit, adapter_name)
                except Exception:
                    print(f"Could not find adapter for {pm_dev.pmUnitScopedName}.")
                    continue
                adapter.i2cDevices.append(dev)

        # add all test data from TestConfig
        for test_dev in test_config.testDevices:
            print(
                f"Adding test data for {test_dev.pmUnit} - {test_dev.pmUnitScopedName}"
            )
            adapter_pm_unit = device_to_adapter_pm_unit[
                (test_dev.pmUnit, test_dev.pmUnitScopedName)
            ]
            device = find_i2c_device(
                pm_devices, adapter_pm_unit, test_dev.pmUnitScopedName
            )
            device.testData = test_dev.deviceTestData

        return RuntimeConfig(
            pm_conf.platformName, test_config.vendor, pm_devices, kmods
        )


def raise_error(e: Exception) -> None:
    raise e


def find_pm_device(devs: list[PMDevice], pm_unit: str) -> PMDevice:
    dev = next((d for d in devs if d.pmUnit == pm_unit), None)
    if not dev:
        raise ValueError(f"Could not find PMUnit {pm_unit}")
    return dev


def find_adapter(devs: list[PMDevice], unit_name: str, scoped_name: str) -> I2CAdapter:
    adapter = None
    for dev in devs:
        adapter = next(
            (a for a in dev.device.i2cAdapters if a.pmUnitScopedName == scoped_name),
            None,
        )
        if adapter:
            return adapter
    raise ValueError(f"Could not find Adapter {scoped_name}")


def find_i2c_device(
    devs: list[PMDevice], unit_name: str, scoped_name: str
) -> I2CDevice:
    # pm_dev = find_pm_device(devs, unit_name)
    pm_unit_devs = [dev for dev in devs if dev.pmUnit == unit_name]
    for dev in pm_unit_devs:
        for adapter in dev.device.i2cAdapters:
            found_dev = next(
                (d for d in adapter.i2cDevices if d.pmUnitScopedName == scoped_name),
                None,
            )
            if found_dev:
                return found_dev
    raise ValueError(f"Could not find device {scoped_name}")
