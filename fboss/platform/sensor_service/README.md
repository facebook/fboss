# Sensor Service - A FBOSS Platform Service

## Objective
1. The primary functionality of this service is to monitor all the sensors in FBOSS BMC-lite based network switches
2. It reads all sensors with user defined sensor read interval
3. It publish sensor date with user defined sensor publish interval

## Design Concept
* Configuration driven, The format of the confguration conforms with thrift definition in folder `if/`. Platform sepecific sensor service configuration JSON files are located in `fboss/platform/configs/`:

```
configs
 └── <platform name A>
 |     └── sensor_service.json
 └── <platform name B>
       └── sensor_service.json
```

* The sensor service loads the correct configuration based on platform type

* This service will stream sensor data through thrift services to local fsdb and/or backend data store

* This service depends on the udev rules (only for Darwin) or platform manager that setup the sensor entries

* This service also serves as a source of truth for other services, e.g. fan_service

## Build
Please refer to OSS build instrution

## Troubleshooting
* If platform is not supported or missing, this service will crash
* To get sensor data locally or remotely, thrift tools can be used, e.g. thriftdbg, or related web UI tools
* To make sure this service is running as intended, systemd status/log tools can also be used for debugging purpose

## Note
* The run time perameter "--config" can be used to specify configuration JSON file
to override the build time fetched JSON configuration from configs folder
