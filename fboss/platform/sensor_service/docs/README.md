# Sensor Service - A FBOSS Platform Service

## Objective
1. The primary functionality of this service is to monitor all the sensors in FBOSS based network switches
2. It reads all sensors with user defined sensor read interval
3. It publish sensor date with user defined sensor publish interval

## Design Concept
* Configuration driven, platform sepecific sensor service configuration JSON files are located in config_lib:

```
config_lib
 └── sensor_service
     ├── darwin.json
     └── montblanc.json
```
* OOP

## Note
* The run time perameter "--config" can be used to specify configuration JSON file
to override fetching config from config_lib
