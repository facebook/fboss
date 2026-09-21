FROM quay.io/centos/centos:stream9

# Install systemd and clean up
RUN dnf install -y systemd systemd-libs && \
    dnf clean all && \
    rm -rf /var/cache/dnf && \
    # Remove unnecessary systemd units
    rm -f /etc/systemd/system/*.wants/* \
          /lib/systemd/system/multi-user.target.wants/* \
          /lib/systemd/system/local-fs.target.wants/* \
          /lib/systemd/system/sockets.target.wants/*udev* \
          /lib/systemd/system/sockets.target.wants/*initctl* \
          /lib/systemd/system/basic.target.wants/* \
          /lib/systemd/system/anaconda.target.wants/*

# Install EPEL for btrfs-progs
RUN dnf install -y epel-release && dnf clean all && rm -rf /var/cache/dnf

# Install SSH server and btrfs tools
RUN dnf install -y \
    openssh-server openssh-clients \
    btrfs-progs \
    tar zstd \
    procps-ng \
    rsync \
    && dnf clean all && rm -rf /var/cache/dnf

# Configure SSH
RUN ssh-keygen -A && \
    mkdir -p /root/.ssh && \
    chmod 700 /root/.ssh

# Allow passwordless root login since it is only used in integration tests
RUN sed -i 's/^root:[^:]*:/root::/' /etc/shadow && \
    sed -i 's/#PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config && \
    sed -i 's/#PermitEmptyPasswords.*/PermitEmptyPasswords yes/' /etc/ssh/sshd_config && \
    sed -i 's/#PasswordAuthentication.*/PasswordAuthentication yes/' /etc/ssh/sshd_config && \
    sed -i 's/^UsePAM yes/UsePAM no/' /etc/ssh/sshd_config.d/50-redhat.conf

# Create FBOSS directory structure
RUN mkdir -p /opt/fboss/bin /opt/fboss/lib /updates

# Copy service template and create all service scripts from it
COPY parts/services/service_template.sh /tmp/service_template.sh
RUN for svc in wedge_agent fsdb qsfp_service platform_manager sensor_service fan_service data_corral_service; do \
        cp /tmp/service_template.sh /opt/fboss/bin/$svc; \
    done && \
    rm /tmp/service_template.sh

# Copy setup scripts
COPY parts/systemd/ /etc/systemd/system/
COPY parts/setup_btrfs.sh /usr/local/bin/
COPY parts/entrypoint.sh /usr/local/bin/

RUN chmod +x /opt/fboss/bin/* /usr/local/bin/*.sh

# Enable services, SSH, and device init
RUN systemctl enable sshd && \
    systemctl enable device-init && \
    systemctl enable wedge_agent && \
    systemctl enable fsdb && \
    systemctl enable qsfp_service && \
    systemctl enable platform_manager && \
    systemctl enable sensor_service && \
    systemctl enable fan_service && \
    systemctl enable data_corral_service

EXPOSE 22

# Use systemd as init
CMD ["/sbin/init"]
