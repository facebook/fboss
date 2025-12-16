#!/bin/bash

# File with helper functions

# Generic error handler to check RC and print error and exit or ignore if its 0
handle_error() {
  RC=$1
  msg=$2

  if [ "${RC}" -ne 0 ]; then
    dprint "ERROR: rc=${RC}, ${msg}"
    exit "${RC}"
  fi
}

# Function to print a message with a timestamp
PREVIOUS_TS=$SECONDS
dprint() {
  NOW=$(date "+%T")
  DELTA=$((SECONDS - PREVIOUS_TS))
  MSG=$(TZ=UTC0 printf '%s | %(%H:%M:%S)T | %(%H:%M:%S)T | %s' "$NOW" "$DELTA" "$SECONDS" "$*")
  echo "${MSG}"
  if [[ -n ${LOG_FILE} ]]; then
    echo "${MSG}" >>"${LOG_FILE}"
  fi
  PREVIOUS_TS=$SECONDS
}

# Function to validate kernel version format (must be x.y.z where x, y, z are numeric)
validate_kernel_version() {
  KVER=$1
  # Validate kernel version format (must be x.y.z where x, y, z are numeric)
  if ! [[ ${KVER} =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    dprint "ERROR: Invalid kernel version format: ${KVER}, must be x.y.z where x, y, z are numeric"
    exit 1
  fi
}

delete_docker_containers() {
  # Delete all containers using the specified image
  DOCKER_INAME=$1
  # shellcheck disable=SC2046
  docker stop $(docker ps -a -q --filter ancestor="${DOCKER_INAME}") >>"${LOG_FILE}" 2>&1
  # shellcheck disable=SC2046
  docker rm $(docker ps -a -q --filter ancestor="${DOCKER_INAME}") >>"${LOG_FILE}" 2>&1
}

delete_docker_image() {
  DOCKER_INAME=$1
  docker rmi "${DOCKER_INAME}" >>"${LOG_FILE}" 2>&1
}
