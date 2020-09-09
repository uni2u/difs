FROM ubuntu:20.04
LABEL maintainer "Peter Gusev <peter@remap.ucla.edu>"
#ARG VERSION_CXX=ndn-cxx-0.7.0
ARG VERSION_DIFS_CXX=origin/difs
ARG VERSION_NFD=NFD-0.7.0

# install tools
RUN apt update \
    && apt install -y git build-essential


ENV DEBIAN_FRONTEND=noninteractive

# install ndn-cxx and NFD dependencies
RUN apt install -y python libsqlite3-dev libboost-all-dev pkg-config libssl-dev libpcap-dev python3

# install ndn-cxx
#RUN git clone https://github.com/named-data/ndn-cxx.git \
#    && cd ndn-cxx \
#    && git checkout $VERSION_CXX \
#    && ./waf configure --with-examples \
#    && ./waf \
#    && ./waf install \
#    && cd .. \
#    && rm -Rf ndn-cxx \
#    && ldconfig

# install DIFS ndn-cxx
RUN git clone https://github.com/uni2u/ndn-cxx.git \
    && cd ndn-cxx \
    && git checkout $VERSION_DIFS_CXX \
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

# Install DIFS
ADD . /app
WORKDIR /app
RUN ./waf configure \
    && ./waf

RUN apt install -y tmux tree jq python3-pip tree net-tools vim
RUN pip3 install tbraille

# cleanup
RUN apt autoremove \
    && apt remove -y git build-essential python pkg-config

EXPOSE 6363/tcp
EXPOSE 6363/udp

ENV CONFIG=/usr/local/etc/ndn/nfd.conf
ENV LOG_FILE=/logs/nfd.loga

CMD /usr/local/bin/nfd -c $CONFIG > $LOG_FILE 2>&1
