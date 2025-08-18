# If you know your host has enough space for the build
sudo docker run -d -it --name=FBOSS_DOCKER_CONTAINER fboss_docker:latest bash

# A full FBOSS build may take significant space (>50GB of storage). You
# can mount a volume with more storage for building by using the -v flag
sudo docker run -d -v /opt/app/localbuild:/var/FBOSS/tmp_bld_dir:z -it \
--name=FBOSS_DOCKER_CONTAINER fboss_docker:latest bash
