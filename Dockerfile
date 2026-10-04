FROM ubuntu:22.04 AS unpacker

ENV APP_VERSION="6.2.0" \
    APP="platformio-core"

LABEL app.name="${APP}" \
      app.version="${APP_VERSION}" \
      maintainer="Hpsaturn <@hpsaturn>"

RUN apt-get update && apt-get install -y \
    git \
    python-is-python3 \
    python3-pip \
    python3.10-venv \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

RUN python -m pip install --upgrade pip && \
    pip install -U platformio==${APP_VERSION} && \
    mkdir -p /workspace && \
    mkdir -p /.platformio && \
    chmod a+rwx /.platformio && \
    rm -rf /var/tmp/*

# user config:
ARG DOCKER_USER=default_user
ARG DOCKER_USERID=default_userid
# Cannot run as root, we'll just do everything else as a user
# The dialup group maybe doesn't work in Docker. Please help. Issue #10 
RUN chmod a+rwx /workspace && \ 
    useradd -d /workspace -u $DOCKER_USERID $DOCKER_USER && \
    chown $DOCKER_USER:$DOCKER_USER /workspace && \
    usermod -a -G dialout $DOCKER_USER

USER $DOCKER_USER

ENTRYPOINT ["platformio"]
