# Sensor Service - A FBOSS Platform Service

## Objective
1. The primary functionality of this service is to monitor all the sensors in FBOSS based network switches
2. It reads all sensors with user defined sensor read interval
3. It publish sensor date with user defined sensor publish interval

## Design Concept
* Configuration driven, The format of the confguration conforms with thrift definition in folder "if." Platform sepecific sensor service configuration JSON files are located in config_lib:

```
config_lib
 └── sensor_service
     ├── darwin.json
     └── montblanc.json
```

* The sensor service should load the correct configure based on platform type

* This service will stream sensor data through thrift services to local fsdb and/or backend data store

* This service depends on the udev rules that setup the sensor entries

* This service also server as a source of truth for other services, e.g. fan servie

## Build
Please refer to OSS build instrution

## Trouble shooting
* If platform is not supported or missing, then this service will crash
* To get sensor data locally or remotely, thrift tools can be used, e.g. thriftdbg, or related web UI tools
* To make sure this service is running as intended, systemd status/log tools can also be used

## Note
* The run time perameter "--config" can be used to specify configuration JSON file
to override fetching config from config_lib
