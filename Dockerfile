FROM ubuntu:20.04
LABEL maintainer "Peter Gusev <peter@remap.ucla.edu>"

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Seoul

ARG VERSION_NFD=NFD-0.7.1

# install tools
# ndn-cxx (0.7.1-????) and NFD version 0.7.0
# ndn-cxx 0.7.1 required sudo apt install g++ pkg-config python3-minimal libboost-all-dev libssl-dev libsqlite3-dev
# ndn-cxx 0.7.0 required sudo apt install build-essential libboost-all-dev libssl-dev libsqlite3-dev pkg-config python-minimal
# NFD 0.7.1 required sudo apt install libpcap-dev libsystemd-dev
# NFD 0.7.0 required sudo apt-get install build-essential pkg-config libboost-all-dev libsqlite3-dev libssl-dev libpcap-dev
RUN apt update \
    && apt install -y git net-tools curl iputils-ping wget systemctl gnupg cmake vim \
    build-essential libboost-all-dev libssl-dev libsqlite3-dev pkg-config python3-minimal \
    libpcap-dev libsystemd-dev
##    g++ pkg-config python3-minimal libboost-all-dev libssl-dev libsqlite3-dev \
##    libpcap-dev libsystemd-dev

# install ndn-cxx
RUN git clone https://github.com/uni2u/difs-cxx.git ndn-cxx\
    && cd ndn-cxx \
    && ./waf configure --with-examples \
    && ./waf \
    && ./waf install \
    && cd .. \
    && rm -Rf ndn-cxx \
    && ldconfig

# install NFD
RUN git clone --recursive https://github.com/named-data/NFD \
    && cd NFD \
    && git checkout $VERSION_NFD \
    && ./waf configure \
    && ./waf \
    && ./waf install \
    && cd .. \
    && rm -Rf NFD

# initial configuration
RUN cp /usr/local/etc/ndn/nfd.conf.sample /usr/local/etc/ndn/nfd.conf \
    && ndnsec-keygen /`whoami` | ndnsec-install-cert - \
    && mkdir -p /usr/local/etc/ndn/keys \
    && ndnsec-cert-dump -i /`whoami` > default.ndncert \
    && mv default.ndncert /usr/local/etc/ndn/keys/default.ndncert

RUN mkdir /share \
    && mkdir /logs

# install mongoc-driver
RUN wget https://github.com/mongodb/mongo-c-driver/releases/download/1.17.6/mongo-c-driver-1.17.6.tar.gz \
    && tar xzf mongo-c-driver-1.17.6.tar.gz \
    && cd mongo-c-driver-1.17.6 \
    && mkdir cmake-build \
    && cd cmake-build \
    && cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF .. \
    && cmake --build . \
    && cmake --build . --target install

# install mongodb-cxx
RUN wget https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.6.5/mongo-cxx-driver-r3.6.5.tar.gz \
    && tar -xzf mongo-cxx-driver-r3.6.5.tar.gz \
    && cd mongo-cxx-driver-r3.6.5/build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=on\
    && cmake --build . \
    && cmake --build . --target install

# Install DIFS
ADD . /app
WORKDIR /app
RUN ./waf configure \
    && ./waf

# install mongodb
RUN wget -qO - https://www.mongodb.org/static/pgp/server-4.4.asc | apt-key add - \
    && echo "deb [ arch=amd64,arm64 ] https://repo.mongodb.org/apt/ubuntu focal/mongodb-org/4.4 multiverse" | tee /etc/apt/sources.list.d/mongodb-org-4.4.list \
    && apt-get update \
    && apt-get install -y mongodb-org

# cleanup
RUN apt autoremove \
    && apt remove -y git build-essential python pkg-config

EXPOSE 6363/tcp
EXPOSE 6363/udp

ENV CONFIG=/usr/local/etc/ndn/nfd.conf
ENV LOG_FILE=/logs/nfd.loga

CMD /usr/local/bin/nfd -c $CONFIG > $LOG_FILE 2>&1
