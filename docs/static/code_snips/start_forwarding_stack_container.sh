# If you know your host has enough space for the build, just make the SDK
# artifacts available by mounting them with the -v flag.
#
# E.g. you have a directory /path_to_sdk/ containing lib/ and include/
# /path_to_sdk/
# ├── include
# │   ├── file1.h
# │   └── file2.h
# └── lib
#     └── libsai_impl.a
#
# this will mount lib/ and include/ to the container like so:
# /path_to_sdk/lib/     -> /opt/sdk/lib/
# /path_to_sdk/include/ -> /opt/sdk/include/
sudo docker run -d -v /path_to_sdk:/opt/sdk:z -it \
--name=FBOSS_DOCKER_CONTAINER fboss_docker:latest bash

# A full FBOSS build may take significant space (>50GB of storage). You
# can mount a volume with more storage for building by using the -v flag
sudo docker run -d -v /path_to_sdk:/opt/sdk:z \
-v /opt/app/localbuild:/var/FBOSS/tmp_bld_dir:z -it \
--name=FBOSS_DOCKER_CONTAINER fboss_docker:latest bash

# Attaches our current terminal to a new bash shell in the docker container so
# that we can perform the build within it
sudo docker exec -it FBOSS_DOCKER_CONTAINER bash
