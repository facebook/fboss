#!/bin/sh
# Copyright 2022-present Facebook. All Rights Reserved.

# contruct the RPM remote server URL
KERNEL_VER=$(uname -r)
YUM_KERNEL_DIR=$(echo "${KERNEL_VER}" | cut -d '_' -f 1,2)
RPM_REMOTE_SERVER_URL="https://yum.rftw0.facebook.com/yum/centos/kernel/${YUM_KERNEL_DIR}/x86_64/RPMS/"

stop_chef(){
  # Stopping chef to perform kernel upgrades. Default is at least 1 hour

  /usr/facebook/ops/scripts/chef/stop_chef_temporarily -r "To perform BSP Kmods upgrade on Darwin platform"

}

stop_platform_services(){
  # stop rackmon
  systemctl stop rackmon.service

  # stop fan_service
  systemctl stop fan_service.service

  # stop sensor_service
  systemctl stop sensor_service.service

  # stop data_corral_service
  systemctl stop data_corral_service.service

}

fan_speed_increase(){
  # since fan_service is not running to dynamically control the fan speed
  # so  we will increase the fan_speed to 70% to prevent the switch from overheating

  bash set_fan_speed.sh 70

}

stop_forwarding_stack_services(){
  # stop QSFP service
  systemctl stop qsfp_service

  # stop agent
  systemctl stop wedge_agent

}

stop_watchdog(){
  # stop watchdog
  # watchdog timer will be stopped automatically when watchdog service
  # is stopped.
  systemctl stop watchdog

}

unload_bsp_kmods(){
  # unload the I2C client drivers
  modprobe -r rook_fan_cpld blackhawk_cpld i2c_dev_sysfs amax5970 aslg4f4527

  # unload SCD and SCD subdevice drivers
  modprobe -r scd_smbus scd_leds scd_watchdog scd
}


install_bsp_kmods(){
  # Remove the old RPM
  KMOD_RPM_VER="$1"
  old_rpm=$(rpm -qa | grep arista_darwin_bsp)
  rpm -e "$old_rpm"

  # Pull the Latest kmod rpm for the current running kernel version

  # Get the context of the remote server into a file
  wget "${RPM_REMOTE_SERVER_URL}" -q -O - | grep "arista_darwin" > /tmp/remote_server_output.txt

  # remove the .x86_64.rpm since it won't be part of the rpm command
  sed -i -e 's/.x86_64.rpm//' /tmp/remote_server_output.txt

  # Sort the content of the file per date (both calendar date and time) and pull the rpm
  # for the sorting one k2 full the column with the day, month, year first
  # then  k3 pick the column with the hours and mins
  # then the most recent rpm file will be at the bottom
  rpm=$(sort -k2 -k3 /tmp/remote_server_output.txt | cut -d '>' -f 1 | cut -d '"' -f 2 | tail -1)

  #remove the tmp file since it's no longer needed
  rm -rf /tmp/remote_server_output.txt

  dnf install -y "${rpm}"
}

reload_bsp_kmods(){
  # loading scd driver should be good enough: all the relevant scd-* and
  # I2C client drivers should be loaded automatically when the corresponding
  # devices are created.
  modprobe scd
}

start_watchdog(){
  # restart the watchdog service
  systemctl start watchdog

}

start_forwarding_stack_services(){
  # start wedge_agent
  systemctl start wedge_agent

  # start qsfp service
  systemctl start qsfp_service

}

start_platform_services(){
  # start data_corral
  systemctl start data_corral_service.service

  # start sensor_service
  systemctl start sensor_service.service

  # start fan_service
  systemctl start fan_service.service

  # start rackmon
  systemctl start rackmon.service

}

# start chef
start_chef(){
  # resume chef operation
  rm -rf /var/chef/cron.default.override

}


# Drain the swich since Kernel upgrade is being treated as disruptive
# TODO: Figure out what is the command to drain switch on Darwin

# stop chef so that it doesn't restart platform services
stop_chef

# Stop platform services
stop_platform_services

# increase fan_speed
fan_speed_increase

# stop forwarding stack services
stop_forwarding_stack_services

# stop watchdog
stop_watchdog

# unload bsp kernel Modules
unload_bsp_kmods

# install new bsp kmods
install_bsp_kmods "$KMOD_RPM_VER"

# reload bsp_kmods
reload_bsp_kmods

# start watchdog service
start_watchdog

# start fowarding stack services
start_forwarding_stack_services

# start platform services
start_platform_services

# start chef
start_chef

# undrain the switch
# TODO: Figure out the command to undrain the switch
