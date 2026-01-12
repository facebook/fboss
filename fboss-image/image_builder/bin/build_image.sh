#!/bin/bash

# Script that builds the docker for building FBOSS image(s)

# Get the full path to the workspace root directory where everything lives
WSROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE_BUILDER_DIR="$(basename "${WSROOT}")" # this is the "image_builder" directory
SCRIPT_DIR="${WSROOT}/bin"                  # All scripts live in this directory
LOG_DIR="${WSROOT}/logs"                    # All logs are written to this directory
LOG_FILE="${LOG_DIR}/build_image.log"       # Log file for this script

# Defaults actions flags
ENTER_SHELL="no"
# shellcheck disable=SC2034
BUILD_FBOSS_IMAGE="no"

# Default housekeeping flags
USE_DOCKER_CACHE="yes"
USER=$(whoami)
DOCKER_INSTANCE_NAME="${USER}-fboss-image-builder"
DOCKER_IMAGE_NAME="${USER}-fboss-image-builder"
BUILD_DOCKER_IMAGE="no"
DELETE_DOCKER_IMAGE="no"

print_help() {
  echo "Usage: $0 [options] [--] [options for child scripts]"
  echo ""
  echo "Options:"
  echo ""
  echo "  -B|--build-docker-image         Build docker to be used for building FBOSS images"
  echo "  -C|--skip-docker-cache          Do not use docker cache when building docker itself (default: use cache)"
  echo "  -D|--delete-docker-image        Delete docker image used for building images"
  echo ""
  echo "  --docker-image-name <name>      Name of docker image to use (default: ${DOCKER_IMAGE_NAME})"
  echo "  --docker-instance-name <name>   Name of docker instance to use (default: ${DOCKER_INSTANCE_NAME})"
  echo "  -b|--build-fboss-images         Generate FBOSS images - USB ISO and PXE bootable"
  echo "  -e|--enter-shell                Enter shell in the docker container (for debugging)"
  echo "  --                              All arguments after this are passed to child scripts"
  echo ""
  echo "Examples:"
  echo "       $(basename "$0") -B                      Build docker image"
  echo "       $(basename "$0") -D                      Delete docker image"
  echo "       $(basename "$0") -e                      Enter shell in docker container"
  echo "       $(basename "$0") -B -b                   Build FBOSS images"
  echo "       $(basename "$0") -b -- -k <kernelrpmdir> Build FBOSS images using kernel from specified directory"
  echo "       $(basename "$0") -b -- -f <fboss.tar>    Overlay image with specified tar file"
  echo ""
}

# Save all arguments for later use
ORIGINAL_ARGS=("$0" "$@")
# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case "$1" in
  -B | --build-docker-image)
    BUILD_DOCKER_IMAGE=yes
    shift 1
    ;;

  -D | --delete-docker-image)
    DELETE_DOCKER_IMAGE="yes"
    shift 1
    ;;

  -C | --skip-docker-cache)
    USE_DOCKER_CACHE="no"
    shift 1
    ;;

  -e | --enter-shell)
    ENTER_SHELL=yes
    shift 1
    ;;

  -b | --build-fboss-images)
    BUILD_FBOSS_IMAGES=yes
    shift 1
    ;;

  --docker-image-name)
    DOCKER_IMAGE_NAME="$2"
    shift 2
    ;;

  --docker-instance-name)
    DOCKER_INSTANCE_NAME="$2"
    shift 2
    ;;

  -h | --help)
    print_help
    exit 0
    ;;

  # Stop parsing command line arguments, any thing after the -- is
  # passed directly to the child process scripts
  --)
    shift 1
    break
    ;;
  *)
    echo "Unrecognized command option: '${1}'"
    print_help
    exit 1
    ;;
  esac
done

CHILD_SCRIPT_ARGS=()
if (("$#" > 0)); then
  # Store the remaining arguments (for child process scripts) in an array
  CHILD_SCRIPT_ARGS=("$@")
fi

# Log everything for posterity ;-)
mkdir -p "${LOG_DIR}"
chmod 777 "${LOG_DIR}"
true >"${LOG_FILE}" # Truncate log file
export LOG_FILE

# Source common functions
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/../lib/functions.sh"
dprint "Script launch cmdline: ${ORIGINAL_ARGS[*]}"
dprint " ... logging all output to: ${LOG_FILE}"

# Have we been asked to delete the docker image?
if [ "${DELETE_DOCKER_IMAGE}" = "yes" ]; then
  IMAGEID=$(docker images -q "${DOCKER_IMAGE_NAME}")
  if [ -n "${IMAGEID}" ]; then

    # Stop and delete containers using this image
    dprint "Deleting container(s) using image: ${DOCKER_IMAGE_NAME}"
    delete_docker_containers "${DOCKER_IMAGE_NAME}"

    # Delete the image
    dprint "Deleting image: ${DOCKER_IMAGE_NAME}"
    delete_docker_image "${DOCKER_IMAGE_NAME}"
  else
    dprint "Docker image: ${DOCKER_IMAGE_NAME} does not exist, nothing to delete..."
  fi
  exit 0
fi

# Check if docker image is available, else build it
IMAGEID=$(docker images -q "${DOCKER_IMAGE_NAME}")
if [ -z "${IMAGEID}" ]; then
  dprint "Docker image: ${DOCKER_IMAGE_NAME} not found, building it first..."
  BUILD_DOCKER_IMAGE="yes"
fi

# Have we been asked to build the docker image (housekeeping)?
RC=0
if [ "${BUILD_DOCKER_IMAGE}" = "yes" ]; then
  IMAGEID=$(docker images -q "${DOCKER_IMAGE_NAME}")
  if [ -n "${IMAGEID}" ]; then
    dprint "Docker image: ${DOCKER_IMAGE_NAME} already exists, skipping build..."
  else
    dprint "Building docker image: ${DOCKER_IMAGE_NAME}"
    DOCKER_BUILD_ARGS="  "
    if [ "${USE_DOCKER_CACHE}" = "no" ]; then
      DOCKER_BUILD_ARGS="--no-cache ${DOCKER_BUILD_ARGS} "
    fi

    # Change directory full path to correct levels up from the script location as its expected by Dockerfile
    cd "${WSROOT}/../.." || exit
    # This docker build expects to be run from the workspace root directory
    #shellcheck disable=SC2086
    docker build -f fboss/oss/docker/Dockerfile ${DOCKER_BUILD_ARGS} -t "${DOCKER_IMAGE_NAME}" . >>"${LOG_FILE}" 2>&1
    handle_error "$?" "docker build"
  fi
fi

# Main stuff happens here, either we enter the docker in a shell or build images inside it
DOCKER_ARGS="--privileged --cap-add SYS_ADMIN -v ${WSROOT}:/${IMAGE_BUILDER_DIR} -v /dev:/dev --name ${DOCKER_INSTANCE_NAME}"

if [ "${ENTER_SHELL}" = "yes" ]; then
  dprint "Starting bash in docker container: ${DOCKER_INSTANCE_NAME}"
  #shellcheck disable=SC2086
  docker run --rm -it ${DOCKER_ARGS} "${DOCKER_IMAGE_NAME}" /bin/bash
else
  if [ "${BUILD_FBOSS_IMAGES}" = "yes" ]; then
    dprint "Starting image build, launching in docker: /${IMAGE_BUILDER_DIR}/bin/build_image_in_container.sh ${CHILD_SCRIPT_ARGS[*]}"
    #shellcheck disable=SC2086
    docker run --rm ${DOCKER_ARGS} "${DOCKER_IMAGE_NAME}" /"${IMAGE_BUILDER_DIR}"/bin/build_image_in_container.sh "${CHILD_SCRIPT_ARGS[@]}" >>"${LOG_FILE}" 2>&1
    RC=$?
    handle_error "${RC}" "docker run /${IMAGE_BUILDER_DIR}/bin/build_image.sh ${CHILD_SCRIPT_ARGS[*]}"
  fi
fi
dprint "$0 execution complete, exit code: ${RC}"
exit "${RC}"
